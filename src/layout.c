#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>

#include "layout.h"

#include "calc.h"
#include "cfg.h"
#include "displ.h"
#include "head.h"
#include "info.h"
#include "lid.h"
#include "list.h"
#include "listeners.h"
#include "log.h"
#include "mode.h"
#include "process.h"
#include "server.h"
#include "wlr-output-management-unstable-v1.h"

struct Head *head_changing_mode = NULL;

void desire_enabled(struct Head *head) {

	// lid closed
	head->desired.enabled = !lid_is_closed(head->name);

	// ignore lid closed when there is only the laptop display, for smoother sleeping
	head->desired.enabled |= slist_length(heads) == 1;

	// explicitly disabled
	head->desired.enabled &= slist_find_equal(cfg->disabled_name_desc, head_matches_name_desc, head) == NULL;
}

void desire_mode(struct Head *head) {
	if (!head->desired.enabled)
		return;

	// attempt to find a mode
	struct Mode *mode = head_find_mode(head);

	if (mode) {
		head->desired.mode = mode;
	} else {

		if (!head->warned_no_mode) {
			log_warn("\nNo mode for %s, disabling.", head->name);
			print_head(WARNING, NONE, head);
			head->warned_no_mode = true;
		}
		head->desired.enabled = false;
	}
}

void desire_scale(struct Head *head) {
	if (!head->desired.enabled)
		return;

	// user scale first
	struct UserScale *user_scale;
	for (struct SList *i = cfg->user_scales; i; i = i->nex) {
		user_scale = (struct UserScale*)i->val;
		if (head_matches_name_desc(user_scale->name_desc, head)) {
			head->desired.scale = wl_fixed_from_double(user_scale->scale);
			return;
		}
	}

	// auto or 1
	if (cfg->auto_scale == ON) {
		head->desired.scale = calc_auto_scale(head);
	} else {
		head->desired.scale = wl_fixed_from_int(1);
	}
}

void desire(void) {

	for (struct SList *i = heads; i; i = i->nex) {
		struct Head *head = (struct Head*)i->val;

		memcpy(&head->desired, &head->current, sizeof(struct HeadState));

		desire_enabled(head);
		desire_mode(head);
		desire_scale(head);

		calc_scaled_dimensions(head);
	}

	struct SList *heads_ordered = calc_head_order(cfg->order_name_desc, heads);

	calc_head_positions(heads_ordered);

	slist_free(&heads_ordered);
}

void apply(void) {
	struct SList *heads_changing = NULL;

	// determine whether changes are needed before initiating output configuration
	struct SList *i = heads;
	while ((i = slist_find(i, head_current_not_desired))) {
		slist_append(&heads_changing, i->val);
		i = i->nex;
	}
	if (!heads_changing)
		return;

	// passed into our configuration listener
	struct zwlr_output_configuration_v1 *zwlr_config = zwlr_output_manager_v1_create_configuration(displ->output_manager, displ->serial);
	zwlr_output_configuration_v1_add_listener(zwlr_config, output_configuration_listener(), displ);

	if ((head_changing_mode = slist_find_val(heads, head_current_mode_not_desired))) {

		print_head(INFO, DELTA, head_changing_mode);

		// mode change in its own operation; mode change desire is always enabled
		head_changing_mode->zwlr_config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head_changing_mode->zwlr_head);
		zwlr_output_configuration_head_v1_set_mode(head_changing_mode->zwlr_config_head, head_changing_mode->desired.mode->zwlr_mode);

	} else {

		print_heads(INFO, DELTA, heads);

		// all changes except mode
		for (i = heads_changing; i; i = i->nex) {
			struct Head *head = (struct Head*)i->val;

			if (head->desired.enabled) {
				head->zwlr_config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
				zwlr_output_configuration_head_v1_set_scale(head->zwlr_config_head, head->desired.scale);
				zwlr_output_configuration_head_v1_set_position(head->zwlr_config_head, head->desired.x, head->desired.y);
			} else {
				zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);
			}
		}
	}

	zwlr_output_configuration_v1_apply(zwlr_config);

	displ->config_state = OUTSTANDING;

	slist_free(&heads_changing);
}

void handle_failure(void) {

	if (head_changing_mode) {

		// mode setting failure, try again
		log_error("  %s:", head_changing_mode->name);
		print_mode(ERROR, head_changing_mode->desired.mode);
		slist_append(&head_changing_mode->modes_failed, head_changing_mode->desired.mode);

		// current mode may be misreported
		head_changing_mode->current.mode = NULL;

	} else {

		// any other failures are fatal
		exit_fail();
	}
}

void layout(void) {

	print_heads(INFO, ARRIVED, heads_arrived);
	slist_free(&heads_arrived);

	print_heads(INFO, DEPARTED, heads_departed);
	slist_free_vals(&heads_departed, head_free);

	switch (displ->config_state) {
		case SUCCEEDED:
			log_info("\nChanges successful");
			displ->config_state = IDLE;
			break;

		case OUTSTANDING:
			// wait
			return;

		case FAILED:
			log_error("\nChanges failed");
			handle_failure();
			displ->config_state = IDLE;
			break;

		case CANCELLED:
			log_warn("\nChanges cancelled, retrying");
			displ->config_state = IDLE;
			return;

		case IDLE:
		default:
			break;
	}

	desire();
	apply();
}


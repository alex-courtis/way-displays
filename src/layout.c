#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>

#include "layout.h"

#include "cfg.h"
#include "displ.h"
#include "global.h"
#include "head.h"
#include "info.h"
#include "lid.h"
#include "slist.h"
#include "listeners.h"
#include "log.h"
#include "mode.h"
#include "process.h"
#include "wlr-output-management-unstable-v1.h"

void position_heads(struct SList *heads) {
	struct Head *head;
	int32_t tallest = 0, widest = 0, x = 0, y = 0;

	// find tallest/widest
	for (struct SList *i = heads; i; i = i->nex) {
		head = i->val;
		if (!head || !head->desired.mode || !head->desired.enabled) {
			continue;
		}
		if (head->scaled.height > tallest) {
			tallest = head->scaled.height;
		}
		if (head->scaled.width > widest) {
			widest = head->scaled.width;
		}
	}

	// arrange each in the predefined order
	for (struct SList *i = heads; i; i = i->nex) {
		head = i->val;
		if (!head || !head->desired.mode || !head->desired.enabled) {
			continue;
		}

		switch (cfg->arrange) {
			case COL:
				// position
				head->desired.y = y;
				y += head->scaled.height;

				// align
				switch (cfg->align) {
					case RIGHT:
						head->desired.x = widest - head->scaled.width;
						break;
					case MIDDLE:
						head->desired.x = (widest - head->scaled.width) / 2.0 + 0.5;
						break;
					case LEFT:
					default:
						head->desired.x = 0;
						break;
				}
				break;
			case ROW:
			default:
				// position
				head->desired.x = x;
				x += head->scaled.width;

				// align
				switch (cfg->align) {
					case BOTTOM:
						head->desired.y = tallest - head->scaled.height;
						break;
					case MIDDLE:
						head->desired.y = (tallest - head->scaled.height) / 2.0 + 0.5;
						break;
					case TOP:
					default:
						head->desired.y = 0;
						break;
				}
				break;
		}
	}
}

struct SList *order_heads(struct SList *order_name_desc, struct SList *heads) {
	if (!heads)
		return NULL;

	unsigned long n_order = slist_length(order_name_desc);
	unsigned long i;
	struct SList *sorting = slist_shallow_clone(heads);

	// array of order to list of heads matched
	struct SList **order_heads = calloc(n_order, sizeof(struct SList*));

	// exact match
	i = 0;
	for (struct SList *o = order_name_desc; o; o = o->nex) {
		slist_move(&order_heads[i], &sorting, head_matches_name_desc_exact, o->val);
		i++;
	}

	// regex
	i = 0;
	for (struct SList *o = order_name_desc; o; o = o->nex) {
		slist_move(&order_heads[i], &sorting, head_matches_name_desc_regex, o->val);
		i++;
	}

	// fuzzy
	i = 0;
	for (struct SList *o = order_name_desc; o; o = o->nex) {
		slist_move(&order_heads[i], &sorting, head_matches_name_desc_fuzzy, o->val);
		i++;
	}

	// marshal the ordered
	struct SList *sorted = NULL;
	for (i = 0; i < n_order; i++) {
		struct SList *order_list = (struct SList*)order_heads[i];
		for (struct SList *h = order_list; h; h = h->nex) {
			slist_append(&sorted, h->val);
		}
		slist_free(&order_list);
	}

	// remaing in discovered order
	for (struct SList *h = sorting; h; h = h->nex) {
		slist_append(&sorted, h->val);
	}

	slist_free(&sorting);
	free(order_heads);

	return sorted;
}

void desire_enabled(struct Head *head) {

	// lid closed
	head->desired.enabled = !lid_is_closed(head->name);

	// ignore lid closed when there is only the laptop display, for smoother sleeping
	head->desired.enabled |= slist_length(heads) == 1;

	// explicitly disabled
	head->desired.enabled &= slist_find_equal(cfg->disabled_name_desc, head_name_desc_matches_head, head) == NULL;
}

void desire_mode(struct Head *head) {
	if (!head->desired.enabled) {
		return;
	}

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
	if (!head->desired.enabled) {
		return;
	}

	// all scaling disabled
	if (cfg->scaling == OFF) {
		head->desired.scale = wl_fixed_from_int(1);
		return;
	}

	// user scale first
	struct UserScale *user_scale;
	for (struct SList *i = cfg->user_scales; i; i = i->nex) {
		user_scale = (struct UserScale*)i->val;
		if (head_matches_name_desc(head, user_scale->name_desc)) {
			head->desired.scale = wl_fixed_from_double(user_scale->scale);
			return;
		}
	}

	// auto or 1
	if (cfg->auto_scale == ON) {
		head->desired.scale = head_auto_scale(head);
	} else {
		head->desired.scale = wl_fixed_from_int(1);
	}
}

void desire_adaptive_sync(struct Head *head) {
	if (!head->desired.enabled) {
		return;
	}

	if (head->adaptive_sync_failed) {
		return;
	}

	if (slist_find_equal(cfg->adaptive_sync_off_name_desc, head_name_desc_matches_head, head)) {
		head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	} else {
		head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	}
}

void desire(void) {

	for (struct SList *i = heads; i; i = i->nex) {
		struct Head *head = (struct Head*)i->val;

		memcpy(&head->desired, &head->current, sizeof(struct HeadState));

		desire_enabled(head);
		desire_mode(head);
		desire_scale(head);
		desire_adaptive_sync(head);

		head_scaled_dimensions(head);
	}

	struct SList *heads_ordered = order_heads(cfg->order_name_desc, heads);

	position_heads(heads_ordered);

	slist_free(&heads_ordered);
}

void apply(void) {
	struct SList *heads_changing = NULL;
	head_changing_mode = NULL;
	head_changing_adaptive_sync = NULL;

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

	} else if ((head_changing_adaptive_sync = slist_find_val(heads, head_current_adaptive_sync_not_desired))) {

		print_head(INFO, DELTA, head_changing_adaptive_sync);

		// adaptive sync change in its own operation; adaptive sync change desire is always enabled
		head_changing_adaptive_sync->zwlr_config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head_changing_adaptive_sync->zwlr_head);
		zwlr_output_configuration_head_v1_set_adaptive_sync(head_changing_adaptive_sync->zwlr_config_head, head_changing_adaptive_sync->desired.adaptive_sync);

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

void handle_success(void) {
	if (head_changing_mode) {

		// succesful mode change is not always reported
		head_changing_mode->current.mode = head_changing_mode->desired.mode;

		head_changing_mode = NULL;

	} else if (head_changing_adaptive_sync) {

		struct Head *head = head_changing_adaptive_sync;
		head_changing_adaptive_sync = NULL;

		// sway reports adaptive sync failure as success
		if (head_current_adaptive_sync_not_desired(head)) {
			log_info("\n%s: Cannot enable VRR, display or compositor may not support it.", head->name);
			head->adaptive_sync_failed = true;
			return;
		}
	}

	log_info("\nChanges successful");
}

void handle_failure(void) {

	if (head_changing_mode) {
		log_error("\nChanges failed");

		// mode setting failure, try again
		log_error("  %s:", head_changing_mode->name);
		print_mode(ERROR, head_changing_mode->desired.mode);
		slist_append(&head_changing_mode->modes_failed, head_changing_mode->desired.mode);

		// current mode may be misreported
		head_changing_mode->current.mode = NULL;

		head_changing_mode = NULL;

	} else if (head_changing_adaptive_sync && head_current_adaptive_sync_not_desired(head_changing_adaptive_sync)) {

		// river reports adaptive sync failure as failure
		log_info("\n%s: Cannot enable VRR, display or compositor may not support it.", head_changing_adaptive_sync->name);
		head_changing_adaptive_sync->adaptive_sync_failed = true;

		head_changing_adaptive_sync = NULL;

	} else {
		log_error("\nChanges failed");

		// any other failures are fatal
		wd_exit_message(EXIT_FAILURE);
	}
}

void layout(void) {

	print_heads(INFO, ARRIVED, heads_arrived);
	slist_free(&heads_arrived);

	print_heads(INFO, DEPARTED, heads_departed);
	slist_free_vals(&heads_departed, head_free);

	switch (displ->config_state) {
		case SUCCEEDED:
			handle_success();
			displ->config_state = IDLE;
			break;

		case OUTSTANDING:
			// wait
			return;

		case FAILED:
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


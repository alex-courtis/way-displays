#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "layout.h"

#include "cfg.h"
#include "conditions.h"
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

void handle_failure(void);

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

#include <stdio.h>

void desire_enabled(struct Head *head) {

	// lid closed
	head->desired.enabled = !lid_is_closed(head->name);

	// ignore lid closed when there is only the laptop display, for smoother sleeping
	head->desired.enabled |= slist_length(heads) == 1;

	// iterate over all matching NAME_DESC's and evaluate their conditions
	struct SList *d = cfg->disabled;
	while ((d = slist_find_equal(d, head_disabled_matches_head, head)) != NULL) {
		struct Disabled *disabled_if = (struct Disabled*)d->val;
		head->desired.enabled &= !condition_list_evaluate(disabled_if->conditions);

		if (!head->desired.enabled) break;

		d = d->nex;
	}
}

void desire_mode(struct Head *head) {
	if (!head->desired.enabled) {
		return;
	}

	// attempt to find a mode, will log and call back
	struct Mode *mode = head_find_mode(head);

	if (mode) {
		head->desired.mode = mode;
	} else {

		if (!head->warned_no_mode) {
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
		head->desired.scale = head_get_fixed_scale(head, 1.0, head->scaling_base);
		return;
	}

	// user scale first
	struct UserScale *user_scale;
	for (struct SList *i = cfg->user_scales; i; i = i->nex) {
		user_scale = (struct UserScale*)i->val;
		if (head_matches_name_desc(head, user_scale->name_desc)) {
			head->desired.scale = head_get_fixed_scale(head, user_scale->scale, head->scaling_base);
			return;
		}
	}

	// auto or 1
	if (cfg->auto_scale == ON) {
		head->desired.scale =
			head_auto_scale(head, cfg->auto_scale_min, cfg->auto_scale_max);
	} else {
		head->desired.scale = head_get_fixed_scale(head, 1.0, head->scaling_base);
	}
}

void desire_transform(struct Head *head) {
	if (!head->desired.enabled) {
		return;
	}

	// maybe user transform
	struct UserTransform *user_transform;
	for (struct SList *i = cfg->user_transforms; i; i = i->nex) {
		user_transform = (struct UserTransform*)i->val;
		if (head_matches_name_desc(head, user_transform->name_desc)) {
			head->desired.transform = user_transform->transform;
			return;
		}
	}

	// normal if not specified
	head->desired.transform = WL_OUTPUT_TRANSFORM_NORMAL;
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

static void desire(void) {

	for (struct SList *i = heads; i; i = i->nex) {
		struct Head *head = (struct Head*)i->val;

		memcpy(&head->desired, &head->current, sizeof(struct HeadState));

		desire_enabled(head);
		desire_mode(head);
		desire_scale(head);
		desire_transform(head);
		desire_adaptive_sync(head);

		head_set_scaled_dimensions(head);
	}

	struct SList *heads_ordered = order_heads(cfg->order_name_desc, heads);

	position_heads(heads_ordered);

	slist_free(&heads_ordered);
}

static void apply(void) {
	struct SList *heads_changing = NULL;

	displ_delta_destroy();

	// determine whether changes are needed before initiating output configuration
	struct SList *i = heads;
	while ((i = slist_find(i, head_current_not_desired))) {
		slist_append(&heads_changing, i->val);
		i = i->nex;
	}
	if (!heads_changing)
		return;

	// passed into our configuration listener
	struct zwlr_output_configuration_v1 *zwlr_config = zwlr_output_manager_v1_create_configuration(displ->zwlr_output_manager, displ->zwlr_output_manager_serial);
	zwlr_output_configuration_v1_add_listener(zwlr_config, zwlr_output_configuration_listener(), displ);

	struct Head *head;
	if ((head = slist_find_val(heads, head_current_mode_not_desired))) {
		displ_delta_init(MODE, head);

		print_head(INFO, DELTA, head);

		// mode change in its own operation; mode change desire is always enabled
		head->zwlr_config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
		zwlr_output_configuration_head_v1_set_mode(head->zwlr_config_head, head->desired.mode->zwlr_mode);

		displ->delta.human = delta_human_mode(displ->state, head);

	} else if ((head = slist_find_val(heads, head_current_adaptive_sync_not_desired))) {
		displ_delta_init(VRR_OFF, head);

		print_head(INFO, DELTA, head);

		// adaptive sync change in its own operation; adaptive sync change desire is always enabled
		head->zwlr_config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
		zwlr_output_configuration_head_v1_set_adaptive_sync(head->zwlr_config_head, head->desired.adaptive_sync);

		displ->delta.human = delta_human_adaptive_sync(displ->state, head);

	} else {
		displ_delta_init(0, NULL);

		print_heads(INFO, DELTA, heads);

		// all other changes
		for (i = heads_changing; i; i = i->nex) {
			struct Head *head = (struct Head*)i->val;

			if (head->desired.enabled) {
				head->zwlr_config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
				zwlr_output_configuration_head_v1_set_scale(head->zwlr_config_head, head->desired.scale);
				zwlr_output_configuration_head_v1_set_position(head->zwlr_config_head, head->desired.x, head->desired.y);
				zwlr_output_configuration_head_v1_set_transform(head->zwlr_config_head, head->desired.transform);
			} else {
				zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);
			}
		}

		displ->delta.human = delta_human(displ->state, heads_changing);
	}

	zwlr_output_configuration_v1_apply(zwlr_config);

	displ->state = OUTSTANDING;

	slist_free(&heads_changing);
}

void handle_success(void) {
	switch(displ->delta.element) {
		case MODE:
			// successful mode change is not always reported
			displ->delta.head->current.mode = displ->delta.head->desired.mode;
			break;

		case VRR_OFF:
			// sway reports adaptive sync failure as success
			if (head_current_adaptive_sync_not_desired(displ->delta.head)) {
				handle_failure();
				return;
			}
			break;

		default:
			break;
	}

	log_info("\nChanges successful");
	call_back(INFO, displ->delta.human ? displ->delta.human : "Changes successful", NULL);

	displ_delta_destroy();
}

void handle_failure(void) {
	switch(displ->delta.element) {
		case MODE:

			print_mode_fail(ERROR, displ->delta.head, displ->delta.head->desired.mode);
			call_back_mode_fail(ERROR, displ->delta.head, displ->delta.head->desired.mode);

			// mode setting failure, try again
			slist_append(&displ->delta.head->modes_failed, displ->delta.head->desired.mode);

			// current mode may be misreported
			displ->delta.head->current.mode = NULL;

			break;

		case VRR_OFF:
			// river reports adaptive sync failure as failure
			if (head_current_adaptive_sync_not_desired(displ->delta.head)) {

				print_adaptive_sync_fail(WARNING, displ->delta.head);
				call_back_adaptive_sync_fail(WARNING, displ->delta.head);

				displ->delta.head->adaptive_sync_failed = true;
			}

			break;
		default:
			log_fatal("\nChanges failed, exiting");
			call_back(FATAL, displ->delta.human, "\nChanges failed, exiting");

			wd_exit_message(EXIT_FAILURE);
			break;
	}

	displ_delta_destroy();
}

void layout(void) {

	print_heads(INFO, ARRIVED, heads_arrived);
	slist_free(&heads_arrived);

	print_heads(INFO, DEPARTED, heads_departed);
	slist_free_vals(&heads_departed, head_free);

	switch (displ->state) {
		case SUCCEEDED:
			handle_success();
			displ->state = IDLE;
			break;

		case OUTSTANDING:
			// wait
			return;

		case FAILED:
			handle_failure();
			displ->state = IDLE;
			break;

		case CANCELLED:
			log_warn("\nChanges cancelled, retrying");
			displ->state = IDLE;
			break;  // TODO: temporary fix, remove later!!!

		case IDLE:
		default:
			break;
	}

	desire();
	apply();
}


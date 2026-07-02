#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "layout.h"

#include "cfg.h"
#include "cfg/disabled.h"
#include "convert.h"
#include "displ.h"
#include "fn.h"
#include "head.h"
#include "info.h"
#include "lid.h"
#include "listeners.h"
#include "log.h"
#include "mode.h"
#include "process.h"
#include "pset.h"
#include "slist.h"
#include "smapi.h"
#include "sset.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"

#define MAX_CANCELLATION_RETRIES 5

int g_cancellation_retries = 0;

void handle_failure(void);

void position_heads(struct SList *heads) {
	struct Head *head;
	int32_t tallest = 0, widest = 0, x = 0, y = 0;

	// find tallest/widest
	for (struct SList *i = heads; i; i = i->nex) {
		head = i->val;
		if (!head || !head->desired.wlr_mode || !head->desired.enabled) {
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
		if (!head || !head->desired.wlr_mode || !head->desired.enabled) {
			continue;
		}

		switch (g_cfg->arrange) {
			case COL:
				// position
				head->desired.y = y;
				y += head->scaled.height;

				// align
				switch (g_cfg->align) {
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
				switch (g_cfg->align) {
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

struct SList *order_heads(const struct SSet * const order_name_desc, struct SList *heads) {
	if (!heads)
		return NULL;

	unsigned long n_order = sset_size(order_name_desc);
	unsigned long i;
	struct SList *sorting = slist_clone(heads, NULL);

	// array of order to list of heads matched
	struct SList **order_heads = calloc(n_order, sizeof(struct SList*));

	// exact match
	i = 0;
	for (const struct SSetIt *it = sset_it(order_name_desc); it; it = sset_it_next(it)) {
		slist_move(&order_heads[i], &sorting, (fn_equal)head_matches_name_desc_exact, it->val);
		i++;
	}

	// regex
	i = 0;
	for (const struct SSetIt *it = sset_it(order_name_desc); it; it = sset_it_next(it)) {
		slist_move(&order_heads[i], &sorting, (fn_equal)head_matches_name_desc_regex, it->val);
		i++;
	}

	// fuzzy
	i = 0;
	for (const struct SSetIt *it = sset_it(order_name_desc); it; it = sset_it_next(it)) {
		slist_move(&order_heads[i], &sorting, (fn_equal)head_matches_name_desc_fuzzy, it->val);
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
	bool enabled = false;

	// lid closed
	enabled = !lid_is_closed(head->name);

	// ignore lid closed when there is only the laptop display, for smoother sleeping
	enabled |= slist_length(g_heads) == 1;

	// name_desc matches and (if present) any condition is true
	enabled &= pset_match(g_cfg->disableds, (fn_match_ptr)disabled_matches_head, head) == NULL;

	// reset manual override when it matches the auto-state
	if (head->overrided_enabled != NoOverride) {
		bool manually_enabled = head->overrided_enabled == OverrideTrue;
		if (enabled == manually_enabled) {
			head->overrided_enabled = NoOverride;
		}
	}

	switch (head->overrided_enabled) {
		case NoOverride:
			head->desired.enabled = enabled;
			break;
		case OverrideTrue:
			head->desired.enabled = true;
			break;
		case OverrideFalse:
			head->desired.enabled = false;
			break;
	}
}

void desire_mode(struct Head *head) {
	if (!head->desired.enabled) {
		return;
	}

	// attempt to find a mode, will log and call back on failure to find a mode
	const struct WlrMode *wlr_mode = head_find_wlr_mode(head);

	if (wlr_mode) {
		head->desired.wlr_mode = wlr_mode;
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
	if (g_cfg->scaling == OFF) {
		head->desired.scale = head_get_fixed_scale(1.0);
		return;
	}

	// user scale first
	const struct SMapIPair pair = smapi_match_key(g_cfg->scales, (fn_match_str)head_name_desc_matches_head, head);
	if (pair.key) {
		head->desired.scale = head_get_fixed_scale((double)pair.val / 1000);
		return;
	}

	// auto or 1
	if (g_cfg->auto_scale == ON) {
		head->desired.scale =
			head_auto_scale(head, g_cfg->auto_scale_min, g_cfg->auto_scale_max);
	} else {
		head->desired.scale = head_get_fixed_scale(1.0);
	}
}

void desire_transform(struct Head *head) {
	if (!head->desired.enabled) {
		return;
	}

	// maybe user transform
	enum wl_output_transform transform = smapi_match_key(g_cfg->transforms, (fn_match_str)head_name_desc_matches_head, head).val;
	if (transform) {
		head->desired.transform = transform;
		return;
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

	if (sset_match(g_cfg->adaptive_sync_off, (fn_match_str)head_name_desc_matches_head, head)) {
		head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	} else {
		head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	}
}

void desire_reapply(struct Head *head) {
	if (head->reapply_required)
		head->desired.enabled = false;
}

static void desire(void) {

	for (struct SList *i = g_heads; i; i = i->nex) {
		struct Head *head = (struct Head*)i->val;

		memcpy(&head->desired, &head->current, sizeof(struct HeadState));

		desire_enabled(head);
		desire_mode(head);
		desire_scale(head);
		desire_transform(head);
		desire_adaptive_sync(head);

		head_set_scaled_dimensions(head);

		desire_reapply(head);
	}

	struct SList *heads_ordered = order_heads(g_cfg->order_name_desc, g_heads);

	position_heads(heads_ordered);

	slist_free(&heads_ordered);
}

static void apply(void) {
	struct SList *heads_changing = NULL;

	displ_delta_destroy();

	// determine whether changes are needed before initiating output configuration
	struct SList *i = g_heads;
	while ((i = slist_find(i, (fn_test)head_current_not_desired))) {
		slist_append(&heads_changing, i->val);
		i = i->nex;
	}
	if (!heads_changing)
		return;

	// passed into our configuration listener
	struct zwlr_output_configuration_v1 *zwlr_config = zwlr_output_manager_v1_create_configuration(g_displ->zwlr_output_manager, g_displ->zwlr_output_manager_serial);
	zwlr_output_configuration_v1_add_listener(zwlr_config, zwlr_output_configuration_listener(), g_displ);

	struct Head *head;

	if ((head = slist_find_val(g_heads, (fn_test)head_reapply_required))) {
		displ_delta_init(0, head);

		print_head(INFO, DELTA, head);

		zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);

		g_displ->delta.human = delta_human_reapply(head);

		head->reapply_required = false;

	} else if ((head = slist_find_val(g_heads, (fn_test)head_current_mode_not_desired))) {
		log_debug("APPLY mode");

		displ_delta_init(MODE, head);

		print_head(INFO, DELTA, head);

		// mode change in its own operation; mode change desire is always enabled
		head->zwlr_config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
		zwlr_output_configuration_head_v1_set_mode(head->zwlr_config_head, head->desired.wlr_mode->zwlr_mode);

		g_displ->delta.human = delta_human_mode(head);

	} else if ((head = slist_find_val(g_heads, (fn_test)head_current_adaptive_sync_not_desired))) {
		log_debug("APPLY vrr");

		displ_delta_init(VRR_OFF, head);

		print_head(INFO, DELTA, head);

		// adaptive sync change in its own operation; adaptive sync change desire is always enabled
		head->zwlr_config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
		zwlr_output_configuration_head_v1_set_adaptive_sync(head->zwlr_config_head, head->desired.adaptive_sync);

		g_displ->delta.human = delta_human_adaptive_sync(head);

	} else {
		log_debug("APPLY remainder");

		displ_delta_init(0, NULL);

		print_heads(INFO, DELTA, g_heads);

		// all other changes
		for (i = heads_changing; i; i = i->nex) {
			head = (struct Head*)i->val;

			if (head->desired.enabled) {
				head->zwlr_config_head = zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
				zwlr_output_configuration_head_v1_set_scale(head->zwlr_config_head, head->desired.scale);
				zwlr_output_configuration_head_v1_set_position(head->zwlr_config_head, head->desired.x, head->desired.y);
				zwlr_output_configuration_head_v1_set_transform(head->zwlr_config_head, head->desired.transform);
			} else {
				zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);
			}
		}

		g_displ->delta.human = delta_human(heads_changing);
	}

	zwlr_output_configuration_v1_apply(zwlr_config);

	g_displ->state = OUTSTANDING;

	slist_free(&heads_changing);
}

void handle_success(void) {
	g_cancellation_retries = 0;

	switch(g_displ->delta.element) {
		case MODE:
			// successful mode change is not always reported
			g_displ->delta.head->current.wlr_mode = g_displ->delta.head->desired.wlr_mode;
			break;

		case VRR_OFF:
			// sway reports adaptive sync failure as success
			if (head_current_adaptive_sync_not_desired(g_displ->delta.head)) {
				handle_failure();
				return;
			}
			break;

		default:
			break;
	}

	log_info(NULL);
	log_info("Changes successful");
	call_back(INFO, g_displ->delta.human ? g_displ->delta.human : "Changes successful", NULL);

	displ_delta_destroy();
}

bool handle_cancelled(void) {
	char *msg;
	bool ret = false;

	if (++g_cancellation_retries <= MAX_CANCELLATION_RETRIES) {
		msg = sprintf_alloc("Changes cancelled, retrying (attempt %i)", g_cancellation_retries);
		ret = true;
	} else {
		msg = sprintf_alloc("Changes cancelled after %i retries", MAX_CANCELLATION_RETRIES);
		ret = false;
	}

	log_warn(NULL);
	log_warn("%s", msg);
	call_back(WARNING, msg, NULL);

	free(msg);

	return ret;
}

void handle_failure(void) {
	switch(g_displ->delta.element) {
		case MODE:

			print_mode_fail(ERROR, g_displ->delta.head, g_displ->delta.head->desired.wlr_mode);
			call_back_mode_fail(ERROR, g_displ->delta.head, g_displ->delta.head->desired.wlr_mode);

			// mode setting failure, try again
			slist_append(&g_displ->delta.head->wlr_modes_failed, (void*)g_displ->delta.head->desired.wlr_mode);

			// current mode may be misreported
			g_displ->delta.head->current.wlr_mode = NULL;

			break;

		case VRR_OFF:
			// river reports adaptive sync failure as failure
			if (head_current_adaptive_sync_not_desired(g_displ->delta.head)) {

				print_adaptive_sync_fail(WARNING, g_displ->delta.head);
				call_back_adaptive_sync_fail(WARNING, g_displ->delta.head);

				g_displ->delta.head->adaptive_sync_failed = true;
			}

			break;
		default:
			log_fatal(NULL);
			log_fatal("Changes failed, exiting");
			call_back(FATAL, g_displ->delta.human, "\nChanges failed, exiting");

			wd_exit_message(EXIT_FAILURE);
			break;
	}

	displ_delta_destroy();
}

void layout(void) {
	print_heads(INFO, ARRIVED, g_heads_arrived);
	slist_free(&g_heads_arrived);

	print_heads(INFO, DEPARTED, g_heads_departed);
	slist_free_vals(&g_heads_departed, (fn_free)head_free);

	log_debug("LAYOUT %s %zu", displ_state_name(g_displ->state), head_num_current_not_desired(g_heads));

	switch (g_displ->state) {
		case SUCCEEDED:
			handle_success();
			g_displ->state = IDLE;
			break;

		case OUTSTANDING:
			// wait
			return;

		case FAILED:
			handle_failure();
			g_displ->state = IDLE;
			break;

		case CANCELLED:
			g_displ->state = IDLE;
			// whether to keep retrying
			if (handle_cancelled()) {
				break;
			} else {
				return;
			}

		case IDLE:
		default:
			break;
	}

	desire();
	log_debug("LAYOUT desired %s %zu", displ_state_name(g_displ->state), head_num_current_not_desired(g_heads));

	apply();
	log_debug("LAYOUT applied %s %zu", displ_state_name(g_displ->state), head_num_current_not_desired(g_heads));
}


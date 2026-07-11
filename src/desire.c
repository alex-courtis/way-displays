#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "desire.h"

#include "cfg.h"
#include "cfg/disabled.h"
#include "fn.h"
#include "head.h"
#include "lid.h"
#include "mode.h"
#include "pset.h"
#include "pslist.h"
#include "smapi.h"
#include "sset.h"
#include "wlr-output-management-unstable-v1.h"

void desire(void) {

	for (struct Pslist *i = g_heads; i; i = i->nex) {
		struct Head *head = (struct Head*)i->val;

		memcpy(&head->desired, &head->current, sizeof(struct HeadState));

		desire_enabled(head);
		desire_mode(head);
		desire_scale(head);
		desire_transform(head);
		desire_adaptive_sync(head);
		desire_scaled_dimensions(head);
		desire_reapply(head);
	}

	struct Pslist *heads_ordered = desire_order(g_cfg->order_name_desc, g_heads);

	desire_positions(heads_ordered);

	pslist_free(&heads_ordered);
}

void desire_enabled(struct Head *head) {
	bool enabled = false;

	// lid closed
	enabled = !lid_is_closed(head->name);

	// ignore lid closed when there is only the laptop display, for smoother sleeping
	enabled |= pslist_length(g_heads) == 1;

	// name_desc matches and (if present) any condition is true
	enabled &= pset_match(g_cfg->disableds, (fn_2pred)disabled_matches_head, head) == NULL;

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
	const struct Mode *mode = head_find_mode(head);

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
	if (g_cfg->scaling == OFF) {
		head->desired.scale = head_get_fixed_scale(1.0);
		return;
	}

	// user scale first
	const struct SMapIPair pair = smapi_match_key(g_cfg->scales, (fn_2pred_str)head_name_desc_matches_head, head);
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
	enum wl_output_transform transform = smapi_match_key(g_cfg->transforms, (fn_2pred_str)head_name_desc_matches_head, head).val;
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

	if (sset_match(g_cfg->adaptive_sync_off, (fn_2pred_str)head_name_desc_matches_head, head)) {
		head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	} else {
		head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	}
}

static int32_t desired_scaled_length(const struct Head * const head, const int32_t length) {
	// scales a (pixel) length by fixed_scale

	int32_t b = g_cfg->scale_round_to ? g_cfg->scale_round_to : SCALE_ROUND_TO_DEFAULT;

	wl_fixed_t f = (double)head->desired.scale / 256 * b + 0.5;

	// wayland truncates when calculating size
	return floor((double)length * b / f);
}

void desire_scaled_dimensions(struct Head * const head) {
	if (!head || !head->desired.mode || !head->desired.scale) {
		return;
	}

	if (head->desired.transform % 2 == 0) {
		head->scaled.width = head->desired.mode->width;
		head->scaled.height = head->desired.mode->height;
	} else {
		head->scaled.width = head->desired.mode->height;
		head->scaled.height = head->desired.mode->width;
	}

	head->scaled.height = desired_scaled_length(head, head->scaled.height);
	head->scaled.width = desired_scaled_length(head, head->scaled.width);
}

void desire_reapply(struct Head *head) {
	if (head->reapply_required)
		head->desired.enabled = false;
}

// TODO this can be simplified
struct Pslist *desire_order(const struct SSet * const order_name_desc, struct Pslist *heads) {
	if (!heads)
		return NULL;

	unsigned long n_order = sset_size(order_name_desc);
	unsigned long i;
	struct Pslist *sorting = pslist_clone(heads, NULL);

	// array of order to list of heads matched
	struct Pslist **order_heads = calloc(n_order, sizeof(struct Pslist*));

	// exact match
	i = 0;
	for (const struct SSetIt *it = sset_it(order_name_desc); it; it = sset_it_next(it)) {
		pslist_move(&order_heads[i], &sorting, (fn_equal)head_matches_name_desc_exact, it->val);
		i++;
	}

	// regex
	i = 0;
	for (const struct SSetIt *it = sset_it(order_name_desc); it; it = sset_it_next(it)) {
		pslist_move(&order_heads[i], &sorting, (fn_equal)head_matches_name_desc_regex, it->val);
		i++;
	}

	// fuzzy
	i = 0;
	for (const struct SSetIt *it = sset_it(order_name_desc); it; it = sset_it_next(it)) {
		pslist_move(&order_heads[i], &sorting, (fn_equal)head_matches_name_desc_fuzzy, it->val);
		i++;
	}

	// marshal the ordered
	struct Pslist *sorted = NULL;
	for (i = 0; i < n_order; i++) {
		struct Pslist *order_list = (struct Pslist*)order_heads[i];
		for (struct Pslist *h = order_list; h; h = h->nex) {
			pslist_append(&sorted, h->val);
		}
		pslist_free(&order_list);
	}

	// remaing in discovered order
	for (struct Pslist *h = sorting; h; h = h->nex) {
		pslist_append(&sorted, h->val);
	}

	pslist_free(&sorting);
	free(order_heads);

	return sorted;
}

void desire_positions(struct Pslist *heads) {
	struct Head *head;
	int32_t tallest = 0, widest = 0, x = 0, y = 0;

	// find tallest/widest
	for (struct Pslist *i = heads; i; i = i->nex) {
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
	for (struct Pslist *i = heads; i; i = i->nex) {
		head = i->val;
		if (!head || !head->desired.mode || !head->desired.enabled) {
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


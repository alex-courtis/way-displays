#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "desire.h"

#include "cfg/cfg.h"
#include "cfg/disabled.h"
#include "displ.h"
#include "enum.h"
#include "fn.h"
#include "head.h"
#include "lid.h"
#include "mode.h"
#include "ppmap.h"
#include "pset.h"
#include "simap.h"
#include "spmap.h"
#include "sset.h"
#include "wlr-output-management-unstable-v1.h"

void desire(void) {

	for (const struct PPmapIt *it = ppmap_it(g_displ->heads); it; it = ppmap_it_next(it)) {
		struct Head *head = (struct Head*)it->val;

		memcpy(&head->des, &head->cur, sizeof(struct HeadState));

		desire_enabled(head);
		desire_mode(head);
		desire_scale(head);
		desire_transform(head);
		desire_adaptive_sync(head);
		desire_scaled_dimensions(head);
		desire_reapply(head);
	}

	const struct Pset *heads_ordered = desire_order(g_cfg->order_name_desc, g_displ->heads);

	desire_positions(heads_ordered);

	pset_free(heads_ordered);
}

void desire_enabled(struct Head *head) {
	bool enabled = false;

	// lid closed
	enabled = !g_lid_is_closed(head->name);

	// ignore lid closed when there is only the laptop display, for smoother sleeping
	enabled |= ppmap_size(g_displ->heads) == 1;

	// name_desc matches and (if present) any condition is true
	enabled &= pset_find(g_cfg->disableds, (fn_2pred)cfg_disabled_matches_head, head) == NULL;

	// reset manual override when it matches the auto-state
	if (head->overrided_enabled != NoOverride) {
		bool manually_enabled = head->overrided_enabled == OverrideTrue;
		if (enabled == manually_enabled) {
			head->overrided_enabled = NoOverride;
		}
	}

	switch (head->overrided_enabled) {
		case NoOverride:
			head->des.enabled = enabled;
			break;
		case OverrideTrue:
			head->des.enabled = true;
			break;
		case OverrideFalse:
			head->des.enabled = false;
			break;
	}
}

void desire_mode(struct Head *head) {
	if (!head->des.enabled) {
		return;
	}

	// attempt to find a mode, will log and call back on failure to find a mode
	const struct zwlr_output_mode_v1 *zmode = head_find_mode(head);

	if (zmode) {
		head->des.zmode = zmode;
	} else {
		if (!head->warned_no_mode) {
			head->warned_no_mode = true;
		}
		head->des.enabled = false;
	}
}

void desire_scale(struct Head *head) {
	if (!head->des.enabled) {
		return;
	}

	// all scaling disabled
	if (g_cfg->scaling == OFF) {
		head->des.scale = head_get_fixed_scale(1.0);
		return;
	}

	// user scale first
	const struct SImapPair pair = simap_find_key(g_cfg->scales, (fn_2pred_str)head_name_desc_matches_head, head);
	if (pair.key) {
		head->des.scale = head_get_fixed_scale((double)pair.val / 1000);
		return;
	}

	// auto or 1
	if (g_cfg->auto_scale == ON) {
		head->des.scale =
			head_auto_scale(head, g_cfg->auto_scale_min, g_cfg->auto_scale_max);
	} else {
		head->des.scale = head_get_fixed_scale(1.0);
	}
}

void desire_transform(struct Head *head) {
	if (!head->des.enabled) {
		return;
	}

	// maybe user transform
	enum wl_output_transform transform = simap_find_key(g_cfg->transforms, (fn_2pred_str)head_name_desc_matches_head, head).val;
	if (transform) {
		head->des.transform = transform;
		return;
	}

	// normal if not specified
	head->des.transform = WL_OUTPUT_TRANSFORM_NORMAL;
}

void desire_adaptive_sync(struct Head *head) {
	if (!head->des.enabled) {
		return;
	}

	if (head->adaptive_sync_failed) {
		return;
	}

	if (sset_find(g_cfg->adaptive_sync_off, (fn_2pred_str)head_name_desc_matches_head, head)) {
		head->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	} else {
		head->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	}
}

static int32_t desired_scaled_length(const struct Head * const head, const int32_t length) {
	// scales a (pixel) length by fixed_scale

	int32_t b = g_cfg->scale_round_to ? g_cfg->scale_round_to : SCALE_ROUND_TO_DEFAULT;

	wl_fixed_t f = (double)head->des.scale / 256 * b + 0.5;

	// wayland truncates when calculating size
	return floor((double)length * b / f);
}

void desire_scaled_dimensions(struct Head * const head) {
	if (!head || !head->des.scale) {
		return;
	}

	const struct Mode *mode_des = ppmap_get(head->modes, head->des.zmode);
	if (!mode_des)
		return;

	if (head->des.transform % 2 == 0) {
		head->scaled.width = mode_des->width;
		head->scaled.height = mode_des->height;
	} else {
		head->scaled.width = mode_des->height;
		head->scaled.height = mode_des->width;
	}

	head->scaled.height = desired_scaled_length(head, head->scaled.height);
	head->scaled.width = desired_scaled_length(head, head->scaled.width);
}

void desire_reapply(struct Head *head) {
	if (head->reapply_required)
		head->des.enabled = false;
}

static void fill_order_buckets(const struct SPmap *buckets, const struct Pset *candidates, fn_2pred pred) {
	for (const struct SPmapIt *bit = spmap_it(buckets); bit; bit = spmap_it_next(bit)) {
		for (const struct PsetIt *cit = pset_filter_it(candidates, pred, bit->key); cit; cit = pset_it_next(cit)) {
			pset_add(bit->val, cit->val);
			pset_it_remove(cit);
		}
	}
}

const struct Pset *desire_order(const struct Sset * const order_name_desc, const struct PPmap *heads) {

	// buckets for each order_name_desc
	const struct SPmapParams params = { .free_val = (fn_free)pset_free, };
	const struct SPmap *buckets = spmap_init_with(params);
	for (const struct SsetIt *it = sset_it(order_name_desc); it; it = sset_it_next(it)) {
		spmap_put(buckets, it->val, head_pset_init());
	}

	// all candidates to be moved into buckets
	const struct Pset *candidates = ppmap_vals_pset(heads);

	// fill buckets in preferential order
	fill_order_buckets(buckets, candidates, (fn_2pred)head_matches_name_desc_exact);
	fill_order_buckets(buckets, candidates, (fn_2pred)head_matches_name_desc_regex);
	fill_order_buckets(buckets, candidates, (fn_2pred)head_matches_name_desc_fuzzy);

	// marshal buckets in final order
	const struct Pset *sorted = head_pset_init();
	for (const struct SPmapIt *it = spmap_it(buckets); it; it = spmap_it_next(it)) {
		pset_add_all(sorted, it->val);
	}

	// add the remainder that didn't match
	pset_add_all(sorted, candidates);

	pset_free(candidates);
	spmap_free_vals(buckets);

	return sorted;
}

void desire_positions(const struct Pset *heads_sorted) {
	int32_t tallest = 0, widest = 0, x = 0, y = 0;

	// find tallest/widest
	for (const struct PsetIt *it = pset_it(heads_sorted); it; it = pset_it_next(it)) {
		const struct Head *head = it->val;
		if (!head->des.zmode || !head->des.enabled) {
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
	for (const struct PsetIt *it = pset_it(heads_sorted); it; it = pset_it_next(it)) {
		struct Head *head = (struct Head*)it->val;
		if (!head->des.zmode || !head->des.enabled) {
			continue;
		}

		switch (g_cfg->arrange) {
			case COL:
				// position
				head->des.y = y;
				y += head->scaled.height;

				// align
				switch (g_cfg->align) {
					case RIGHT:
						head->des.x = widest - head->scaled.width;
						break;
					case MIDDLE:
						head->des.x = (widest - head->scaled.width) / 2.0 + 0.5;
						break;
					case LEFT:
					default:
						head->des.x = 0;
						break;
				}
				break;
			case ROW:
			default:
				// position
				head->des.x = x;
				x += head->scaled.width;

				// align
				switch (g_cfg->align) {
					case BOTTOM:
						head->des.y = tallest - head->scaled.height;
						break;
					case MIDDLE:
						head->des.y = (tallest - head->scaled.height) / 2.0 + 0.5;
						break;
					case TOP:
					default:
						head->des.y = 0;
						break;
				}
				break;
		}
	}
}


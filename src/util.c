#include <string.h>

#include <wayland-util.h>

#include "util.h"

struct Mode *optimal_mode(struct SList *modes) {
	struct Mode *mode, *optimal_mode;

	optimal_mode = NULL;
	for (struct SList *i = modes; i; i = i->nex) {
		mode = i->val;

		if (!mode) {
			continue;
		}

		if (!optimal_mode) {
			optimal_mode = mode;
		}

		// preferred first
		if (mode->preferred) {
			return mode;
		}

		// highest resolution
		if (mode->width * mode->height > optimal_mode->width * optimal_mode->height) {
			optimal_mode = mode;
			continue;
		}

		// highest refresh at highest resolution
		if (mode->width == optimal_mode->width &&
				mode->height == optimal_mode->height &&
				mode->refresh_mHz > optimal_mode->refresh_mHz) {
			optimal_mode = mode;
			continue;
		}
	}

	return optimal_mode;
}

wl_fixed_t auto_scale(struct Head *head) {
	if (!head
			|| !head->desired.mode
			|| head->desired.mode->width == 0
			|| head->desired.mode->height == 0
			|| head->width_mm == 0
			|| head->height_mm == 0) {
		return wl_fixed_from_int(0);
	}

	// average dpi
	double dpi_horiz = (double)(head->desired.mode->width) / head->width_mm * 25.4;
	double dpi_vert = (double)(head->desired.mode->height) / head->height_mm * 25.4;
	double dpi = (dpi_horiz + dpi_vert) / 2;

	// round the dpi to the nearest 12, so that we get a nice even wl_fixed_t
	long dpi_quantized = (long)(dpi / 12 + 0.5) * 12;

	// 96dpi approximately correct for older monitors and became the convention for 1:1 scaling
	return 256 * dpi_quantized / 96;
}

struct SList *order_heads(struct SList *order_name_desc, struct SList *heads) {
	struct SList *heads_ordered = NULL;
	struct Head *head;
	struct SList *i, *j;

	struct SList *sorting = slist_shallow_clone(heads);

	// specified order first
	for (i = order_name_desc; i; i = i->nex) {
		j = sorting;
		while(j) {
			head = j->val;
			j = j->nex;
			if (!head) {
				continue;
			}
			if (i->val &&
					((head->name && strcmp(i->val, head->name) == 0) ||
					 (head->description && strcmp(i->val, head->description) == 0))) {
				slist_append(&heads_ordered, head);
				slist_remove(&sorting, head);
			}
		}
	}

	// remaing in discovered order
	for (i = sorting; i; i = i->nex) {
		head = i->val;
		if (!head) {
			continue;
		}

		slist_append(&heads_ordered, head);
	}

	slist_free(&sorting);

	return heads_ordered;
}

void ltr_heads(struct SList *heads) {
	struct Head *head;
	int32_t x;

	x = 0;
	for (struct SList *i = heads; i; i = i->nex) {
		head = i->val;
		if (!head || !head->desired.mode || !head->enabled) {
			continue;
		}

		head->desired.x = x;
		head->desired.y = 0;

		x += (int32_t)((double)head->desired.mode->width * 256 / head->desired.scale + 0.5);
	}
}


#include <stdint.h>
#include <string.h>
#include <wayland-util.h>

#include "calc.h"

#include "cfg.h"
#include "head.h"
#include "list.h"
#include "mode.h"
#include "server.h"

wl_fixed_t calc_auto_scale(struct Head *head) {
	if (!head || !head->desired.mode) {
		return wl_fixed_from_int(1);
	}

	// average dpi
	double dpi = mode_dpi(head->desired.mode);
	if (dpi == 0) {
		return wl_fixed_from_int(1);
	}

	// round the dpi to the nearest 12, so that we get a nice even wl_fixed_t
	long dpi_quantized = (long)(dpi / 12 + 0.5) * 12;

	// 96dpi approximately correct for older monitors and became the convention for 1:1 scaling
	return 256 * dpi_quantized / 96;
}

void calc_scaled_dimensions(struct Head *head) {
	if (!head || !head->desired.mode || !head->desired.scale) {
		return;
	}

	if (head->transform % 2 == 0) {
		head->scaled.width = head->desired.mode->width;
		head->scaled.height = head->desired.mode->height;
	} else {
		head->scaled.width = head->desired.mode->height;
		head->scaled.height = head->desired.mode->width;
	}

	head->scaled.height = (int32_t)((double)head->scaled.height * 256 / head->desired.scale + 0.5);
	head->scaled.width = (int32_t)((double)head->scaled.width * 256 / head->desired.scale + 0.5);
}

struct SList *calc_head_order(struct SList *order_name_desc, struct SList *heads) {
	struct SList *heads_ordered = NULL;
	struct Head *head;
	struct SList *i, *j, *r;

	struct SList *sorting = slist_shallow_clone(heads);

	// specified order first
	for (i = order_name_desc; i; i = i->nex) {
		j = sorting;
		while(j) {
			head = j->val;
			r = j;
			j = j->nex;
			if (!head) {
				continue;
			}
			if (i->val && head_matches_name_desc(i->val, head)) {
				slist_append(&heads_ordered, head);
				slist_remove(&sorting, &r);
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

void calc_head_positions(struct SList *heads) {
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
						head->desired.x = (widest - head->scaled.width + 0.5) / 2;
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
						head->desired.y = (tallest - head->scaled.height + 0.5) / 2;
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


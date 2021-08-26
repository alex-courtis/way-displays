#include "util.h"

struct Mode *optimal_mode(struct SList *modes) {
	struct Mode *mode, *optimal_mode;

	optimal_mode = NULL;
	for (struct SList *i = modes; i; i = i->nex) {
		mode = i->val;

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

void order_desired_heads(struct OutputManager *output_manager) {

	// TODO apply optional desired.order

	struct Head *head;
	for (struct SList *i = output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (head->desired.enabled) {
			slist_append(&output_manager->desired.heads_enabled, head);
		} else {
			slist_append(&output_manager->desired.heads_disabled, head);
		}
	}
}


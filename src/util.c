#include <stdio.h>

#include "util.h"

struct Mode *optimal_mode(struct wl_list *modes) {
	struct Mode *mode, *optimal_mode;

	optimal_mode = NULL;
	wl_list_for_each(mode, modes, link) {

		if (!optimal_mode) {
			optimal_mode = mode;
			continue;
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
			|| !head->desired_mode
			|| head->desired_mode->width == 0
			|| head->desired_mode->height == 0
			|| head->width_mm == 0
			|| head->height_mm == 0) {
		return wl_fixed_from_int(0);
	}

	// average dpi
	double dpi_horiz = (double)(head->desired_mode->width) / head->width_mm * 25.4;
	double dpi_vert = (double)(head->desired_mode->height) / head->height_mm * 25.4;
	double dpi = (dpi_horiz + dpi_vert) / 2;

	// round the dpi to the nearest 12, so that we get a nice even wl_fixed_t
	long dpi_quantized = (long)(dpi / 12 + 0.5) * 12;

	// 96dpi approximately correct for older monitors and became the convention for 1:1 scaling
	return 256 * dpi_quantized / 96;
}


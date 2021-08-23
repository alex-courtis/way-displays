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


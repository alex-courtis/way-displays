#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mode.h"

#include "cfg.h"
#include "cfg/user-mode.h"
#include "fn.h"
#include "head.h"
#include "str.h"
#include "pset.h"
#include "wlr-output-management-unstable-v1.h"

// TODO split by wlrmode/usermode

void wlr_mode_free(struct WlrMode *wlr_mode) {
	free(wlr_mode);
}

char *wlr_mode_str(const struct WlrMode * const wlr_mode) {
	if (!wlr_mode)
		return NULL;

	return sprintf_alloc("%dx%d@%dHz (%d,%03dmHz)%s",
			wlr_mode->width,
			wlr_mode->height,
			mhz_to_hz_rounded(wlr_mode->refresh_mhz),
			wlr_mode->refresh_mhz / 1000,
			wlr_mode->refresh_mhz % 1000,
			wlr_mode->preferred ? " (preferred)" : ""
			);
}

const struct PSet *wlr_mode_pset_init(void) {
	const struct PSetParams params = {
		.free_val = (fn_free)wlr_mode_free,
		.str_val = (fn_str)wlr_mode_str,
	};
	return pset_init_with(params);
}

const struct WlrMode *mode_preferred(const struct PSet* const wlr_modes, const struct PSet* const wlr_modes_failed) {
	const struct WlrMode *wlr_mode_preferred = NULL;

	const struct PSetIt *it;

	for (it = pset_it(wlr_modes); it; it = pset_it_next(it)) {
		const struct WlrMode *wlr_mode = it->val;

		if (wlr_mode->preferred && !pset_contains(wlr_modes_failed, wlr_mode)) {
			wlr_mode_preferred = wlr_mode;
			break;
		}
	}

	pset_it_free(it);
	return wlr_mode_preferred;
}

const struct WlrMode *mode_max_preferred(const struct PSet* wlr_modes, const struct PSet* const wlr_modes_failed) {
	const struct WlrMode *preferred = mode_preferred(wlr_modes, wlr_modes_failed);

	if (!preferred)
		return NULL;

	const struct WlrMode *wlr_mode = NULL;
	const struct WlrMode *wlr_mode_max = NULL;

	for (const struct PSetIt *it = pset_it(wlr_modes); it; it = pset_it_next(it)) {
		wlr_mode = it->val;

		if (pset_contains(wlr_modes_failed, wlr_mode)) {
			continue;
		}

		if (wlr_mode->width != preferred->width || wlr_mode->height != preferred->height) {
			continue;
		}

		if (!wlr_mode_max) {
			wlr_mode_max = wlr_mode;
		} else if (wlr_mode->refresh_mhz > wlr_mode_max->refresh_mhz) {
			wlr_mode_max = wlr_mode;
		}
	}

	return wlr_mode_max;
}

const char *mhz_to_hz_str(int32_t mhz) {
	static char buf[64];
	snprintf(buf, 64, "%g", ((float)mhz) / 1000);
	return buf;
}

int32_t hz_str_to_mhz(const char *hz_str) {
	if (!hz_str)
		return 0;

	return lround(atof(hz_str) * 1000);
}

int32_t mhz_to_hz_rounded(int32_t mhz) {
	return (mhz + 500) / 1000;
}

static bool mode_equal_user_mode_res_mhz(const struct WlrMode* const wlr_mode, const struct UserMode* const user_mode) {
	if (!wlr_mode || !user_mode) {
		return false;
	}

	return wlr_mode->width == user_mode->width &&
		wlr_mode->height == user_mode->height &&
		wlr_mode->refresh_mhz == user_mode->refresh_mhz;
}

bool mode_greater_than_res_refresh(const struct WlrMode* const a, const struct WlrMode* const b) {
	if (!a || !b) {
		return false;
	}

	if (a->width > b->width) {
		return true;
	} else if (a->width != b->width) {
		return false;
	}

	if (a->height > b->height) {
		return true;
	} else if (a->height != b->height) {
		return false;
	}

	if (a->refresh_mhz > b->refresh_mhz) {
		return true;
	}

	return false;
}

static bool wlr_mode_satisfies_user_mode(const struct WlrMode* const wlr_mode, const struct UserMode *user_mode) {
	if (!wlr_mode || !user_mode)
		return false;

	// TODO mode_equal_res_hz
	return user_mode->max ||
		(wlr_mode->width == user_mode->width &&
		 wlr_mode->height == user_mode->height &&
		 (user_mode->refresh_mhz == -1 || mhz_to_hz_rounded(wlr_mode->refresh_mhz) == mhz_to_hz_rounded(user_mode->refresh_mhz)));
}

double mode_dpi(const struct WlrMode* const wlr_mode) {
	if (!wlr_mode || !wlr_mode->head || !wlr_mode->head->width_mm || !wlr_mode->head->height_mm) {
		return 0;
	}

	double dpi_horiz = (double)(wlr_mode->width) / wlr_mode->head->width_mm * 25.4;
	double dpi_vert = (double)(wlr_mode->height) / wlr_mode->head->height_mm * 25.4;
	return (dpi_horiz + dpi_vert) / 2;
}

double mode_scale(const struct WlrMode* const wlr_mode) {
	double dpi = mode_dpi(wlr_mode);

	if (dpi == 0) {
		return 1;
	}

	return dpi / (g_cfg->auto_scale_dpi ? g_cfg->auto_scale_dpi : AUTO_SCALE_DPI_DEFAULT);
}

const struct WlrMode *mode_user_mode(const struct PSet* const wlr_modes, const struct PSet* const wlr_modes_failed, const struct UserMode *user_mode) {
	if (!wlr_modes || !user_mode)
		return NULL;

	const struct WlrMode *wlr_mode = NULL;

	// exact res and refresh
	const struct WlrMode *wlr_mode_exact = pset_match(wlr_modes, (fn_match_ptr)mode_equal_user_mode_res_mhz, user_mode);
	if (wlr_mode_exact && !pset_contains(wlr_modes_failed, wlr_mode_exact)) {
		return wlr_mode_exact;
	}

	// search from the top down
	const struct PSet *wlr_modes_sorted = pset_clone_shallow(wlr_modes);
	pset_sort(wlr_modes_sorted, (fn_less_than)mode_greater_than_res_refresh);

	// first matching the user mode
	for (const struct PSetIt *it = pset_match_it(wlr_modes_sorted, (fn_match_ptr)wlr_mode_satisfies_user_mode, user_mode); it; it = pset_it_next(it)) {
		if (!pset_contains(wlr_modes_failed, it->val)) {
			wlr_mode = it->val;
			pset_it_free(it);
			break;
		}
	}

	pset_free(wlr_modes_sorted);

	return wlr_mode;
}

struct WlrMode *wlr_mode_init(struct Head *head, struct zwlr_output_mode_v1 *zwlr_mode, int32_t width, int32_t height, int32_t refresh_mhz, bool preferred) {
	struct WlrMode *wlr_mode = calloc(1, sizeof(struct WlrMode));

	wlr_mode->head = head;
	wlr_mode->zwlr_mode = zwlr_mode;
	wlr_mode->width = width;
	wlr_mode->height = height;
	wlr_mode->refresh_mhz = refresh_mhz;
	wlr_mode->preferred = preferred;

	return wlr_mode;
}

// TODO this could just be a fn_test; add them to map/set
bool wlr_mode_match_preferred(const struct WlrMode *wlr_mode, const void* const data) {
	return wlr_mode && wlr_mode->preferred;
}

bool wlr_mode_match_zwlr_mode(const struct WlrMode *wlr_mode, const struct zwlr_output_mode_v1 *zwlr_mode) {
	return wlr_mode ? wlr_mode->zwlr_mode == zwlr_mode : false;
}


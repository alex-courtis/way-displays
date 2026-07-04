#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mode.h"

#include "cfg.h"
#include "fn.h"
#include "head.h"
#include "log.h"
#include "pset.h"
#include "smap.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"

struct WlrMode *wlr_mode_init(void) {
	struct WlrMode *wlr_mode = calloc(1, sizeof(struct WlrMode));

	wlr_mode->width = -1;
	wlr_mode->height = -1;
	wlr_mode->refresh_mhz = -1;

	return wlr_mode;
}

struct WlrMode *wlr_mode_clone(const struct WlrMode * const from) {
	if (!from)
		return NULL;

	struct WlrMode *to = (struct WlrMode*)calloc(1, sizeof(struct WlrMode));

	*to = *from;

	return to;
}

const struct SMap *wlr_mode_smap_init(void) {
	const struct SMapParams params = {
		.equal_val = (fn_equal)wlr_mode_equal,
		.free_val = (fn_free)wlr_mode_free,
		.str_val = (fn_str)wlr_mode_str,
		.clone_val = (fn_clone)wlr_mode_clone,
	};
	return smap_init_with(params);
}

const struct PSet *wlr_mode_pset_init(void) {
	const struct PSetParams params = {
		.free_val = (fn_free)wlr_mode_free,
		.str_val = (fn_str)wlr_mode_str,
	};
	return pset_init_with(params);
}

void wlr_mode_free(struct WlrMode *wlr_mode) {
	free(wlr_mode);
}

bool wlr_mode_equal(const struct WlrMode* const a, const struct WlrMode* const b) {
	return a && b && memcmp(a, b, sizeof(struct WlrMode)) == 0;
}

bool wlr_mode_equal_res_hz(const struct WlrMode* const a, const struct WlrMode* const b) {
	return a && b &&
		a->width == b->width &&
		a->height == b->height &&
		mhz_to_hz_rounded(a->refresh_mhz) == mhz_to_hz_rounded(b->refresh_mhz);
}

// TODO inline
bool wlr_mode_equal_user_mode_res_mhz(const struct WlrMode* const wlr_mode, const struct WlrMode* const user_mode) {
	if (!wlr_mode || !user_mode) {
		return false;
	}

	return wlr_mode->width == user_mode->width &&
		wlr_mode->height == user_mode->height &&
		wlr_mode->refresh_mhz == user_mode->refresh_mhz;
}

bool wlr_mode_greater_than_res_refresh(const struct WlrMode* const a, const struct WlrMode* const b) {
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

char *user_mode_str(const struct WlrMode * const user_mode) {
	if (!user_mode)
		return NULL;

	if (user_mode->max) {
		return sprintf_alloc("MAX");
	} else if (user_mode->refresh_mhz != -1) {
		return sprintf_alloc("%dx%d@%gHz",
				user_mode->width,
				user_mode->height,
				((float)user_mode->refresh_mhz) / 1000
				);
	} else {
		return sprintf_alloc("%dx%d",
				user_mode->width,
				user_mode->height
				);
	}
}

bool wlr_mode_is_preferred(const struct WlrMode *wlr_mode, const void* const unused) {
	return wlr_mode && wlr_mode->preferred;
}

bool wlr_mode_is_zwlr_mode(const struct WlrMode *wlr_mode, const struct zwlr_output_mode_v1 *zwlr_mode) {
	return wlr_mode ? wlr_mode->zwlr_mode == zwlr_mode : false;
}

// TODO inline
bool wlr_mode_satisfies_user_mode(const struct WlrMode* const wlr_mode, const struct WlrMode *user_mode) {
	if (!wlr_mode || !user_mode)
		return false;

	return user_mode->max ||
		(wlr_mode->width == user_mode->width &&
		 wlr_mode->height == user_mode->height &&
		 (user_mode->refresh_mhz == -1 || mhz_to_hz_rounded(wlr_mode->refresh_mhz) == mhz_to_hz_rounded(user_mode->refresh_mhz)));
}

bool user_mode_invalid(const char* const name_desc, const struct WlrMode* const user_mode, const void* const data) {
	if (!user_mode || !name_desc) {
		return true;
	}
	if (user_mode->width != -1 && user_mode->width <= 0) {
		log_warn(NULL);
		log_warn("Ignoring non-positive MODE %s WIDTH %d", name_desc, user_mode->width);
		return true;
	}
	if (user_mode->height != -1 && user_mode->height <= 0) {
		log_warn(NULL);
		log_warn("Ignoring non-positive MODE %s HEIGHT %d", name_desc, user_mode->height);
		return true;
	}
	if (user_mode->refresh_mhz != -1 && user_mode->refresh_mhz <= 0) {
		log_warn(NULL);
		log_warn("Ignoring non-positive MODE %s HZ %g", name_desc, ((float)user_mode->refresh_mhz) / 1000);
		return true;
	}

	if (!user_mode->max) {
		if (user_mode->width == -1) {
			log_warn(NULL);
			log_warn("Ignoring invalid MODE %s missing WIDTH", name_desc);
			return true;
		}
		if (user_mode->height == -1) {
			log_warn(NULL);
			log_warn("Ignoring invalid MODE %s missing HEIGHT", name_desc);
			return true;
		}
	}

	return false;
}

int32_t mhz_to_hz_rounded(int32_t mhz) {
	return (mhz + 500) / 1000;
}

double wlr_mode_dpi(const struct WlrMode* const wlr_mode) {
	if (!wlr_mode || !wlr_mode->head || !wlr_mode->head->width_mm || !wlr_mode->head->height_mm) {
		return 0;
	}

	double dpi_horiz = (double)(wlr_mode->width) / wlr_mode->head->width_mm * 25.4;
	double dpi_vert = (double)(wlr_mode->height) / wlr_mode->head->height_mm * 25.4;
	return (dpi_horiz + dpi_vert) / 2;
}

double wlr_mode_scale(const struct WlrMode* const wlr_mode) {
	double dpi = wlr_mode_dpi(wlr_mode);

	if (dpi == 0) {
		return 1;
	}

	return dpi / (g_cfg->auto_scale_dpi ? g_cfg->auto_scale_dpi : AUTO_SCALE_DPI_DEFAULT);
}

const struct WlrMode *wlr_mode_preferred(const struct PSet* const wlr_modes, const struct PSet* const wlr_modes_failed) {
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

const struct WlrMode *wlr_mode_max_preferred(const struct PSet* wlr_modes, const struct PSet* const wlr_modes_failed) {
	const struct WlrMode *preferred = wlr_mode_preferred(wlr_modes, wlr_modes_failed);

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

const struct WlrMode *wlr_mode_for_user_mode(const struct PSet* const wlr_modes, const struct PSet* const wlr_modes_failed, const struct WlrMode *user_mode) {
	if (!wlr_modes || !user_mode)
		return NULL;

	// TODO strip modes failed first

	const struct WlrMode *wlr_mode = NULL;

	// exact res and refresh
	const struct WlrMode *wlr_mode_exact = pset_match(wlr_modes, (fn_match_ptr)wlr_mode_equal_user_mode_res_mhz, user_mode);
	if (wlr_mode_exact && !pset_contains(wlr_modes_failed, wlr_mode_exact)) {
		return wlr_mode_exact;
	}

	// search from the top down
	const struct PSet *wlr_modes_sorted = pset_clone_shallow(wlr_modes);
	pset_sort(wlr_modes_sorted, (fn_less_than)wlr_mode_greater_than_res_refresh);

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


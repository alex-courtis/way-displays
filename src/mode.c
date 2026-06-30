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
#include "pmap.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"

// TODO name by wlrmode/usermode

const struct WlrMode *mode_preferred(const struct PMap* const wlr_modes, struct SList *wlr_modes_failed) {

	const struct PMapIt *it;

	for (it = pmap_it(wlr_modes); it; it = pmap_it_next(it)) {
		const struct WlrMode *wlr_mode = it->val;

		if (wlr_mode->preferred && !slist_find_equal(wlr_modes_failed, NULL, wlr_mode)) {
			pmap_it_free(it);
			return wlr_mode;
		}
	}

	pmap_it_free(it);
	return NULL;
}

const struct WlrMode *mode_max_preferred(const struct PMap* wlr_modes, struct SList *wlr_modes_failed) {
	const struct WlrMode *preferred = mode_preferred(wlr_modes, wlr_modes_failed);

	if (!preferred)
		return NULL;

	const struct WlrMode *wlr_mode = NULL;
	const struct WlrMode *wlr_mode_max = NULL;

	for (const struct PMapIt *it = pmap_it(wlr_modes); it; it = pmap_it_next(it)) {
		wlr_mode = it->val;

		if (slist_find_equal(wlr_modes_failed, NULL, wlr_mode)) {
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

static bool mode_equal_res_hz(const struct WlrMode* const a, const struct WlrMode* const b) {
	if (!a || !b) {
		return false;
	}

	return a->width == b->width &&
		a->height == b->height &&
		mhz_to_hz_rounded(a->refresh_mhz) == mhz_to_hz_rounded(b->refresh_mhz);
}

static bool mode_equal_user_mode_res_hz(const void* const key, const struct WlrMode* const wlr_mode, const struct UserMode* const user_mode) {
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

static bool mrr_satisfies_user_mode(const struct ModesResRefresh *mrr, const struct UserMode *user_mode) {
	if (!mrr || !user_mode) {
		return false;
	}

	return user_mode->max ||
		(mrr->width == user_mode->width &&
		 mrr->height == user_mode->height &&
		 (user_mode->refresh_mhz == -1 || mhz_to_hz_rounded(mrr->refresh_mhz) == mhz_to_hz_rounded(user_mode->refresh_mhz)));
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

struct SList *modes_res_refresh(const struct PMap* const wlr_modes) {
	struct SList *mrrs = NULL;

	// TODO add sorting to map or key/vals to set
	struct SList *unsorted = pmap_vals_slist_shallow(wlr_modes);
	struct SList *sorted = slist_sort(unsorted, (fn_less_than)mode_greater_than_res_refresh);

	struct ModesResRefresh *mrr = NULL;
	struct WlrMode *wlr_mode = NULL;
	for (struct SList *i = sorted; i; i = i->nex) {
		wlr_mode = i->val;

		if (!mrr || !mode_equal_res_hz(wlr_mode, mrr->wlr_modes->val)) {
			mrr = calloc(1, sizeof(struct ModesResRefresh));
			mrr->width = wlr_mode->width;
			mrr->height = wlr_mode->height;
			mrr->refresh_mhz = wlr_mode->refresh_mhz;
			slist_append(&mrrs, mrr);
		}

		slist_append(&mrr->wlr_modes, wlr_mode);
	}

	slist_free(&unsorted);
	slist_free(&sorted);

	return mrrs;
}

const struct WlrMode *mode_user_mode(const struct PMap* const wlr_modes, struct SList *wlr_modes_failed, const struct UserMode *user_mode) {
	if (!wlr_modes || !user_mode)
		return NULL;

	struct SList *i, *j;

	// exact res and refresh
	const struct WlrMode *wlr_mode_exact = pmap_match(wlr_modes, (fn_match_key_val)mode_equal_user_mode_res_hz, user_mode).val;
	if (wlr_mode_exact && !slist_find_equal_val(wlr_modes_failed, NULL, wlr_mode_exact)) {
		return wlr_mode_exact;
	}

	// highest mode matching the user mode
	struct SList *mrrs = modes_res_refresh(wlr_modes);
	for (i = mrrs; i; i = i->nex) {
		struct ModesResRefresh *mrr = i->val;
		if (mrr && mrr_satisfies_user_mode(mrr, user_mode)) {
			for (j = mrr->wlr_modes; j; j = j->nex) {
				struct WlrMode *wlr_mode = j->val;
				if (!slist_find_equal(wlr_modes_failed, NULL, wlr_mode)) {
					slist_free_vals(&mrrs, (fn_free)mode_res_refresh_free);
					return wlr_mode;
				}
			}
		}
	}
	slist_free_vals(&mrrs, (fn_free)mode_res_refresh_free);

	return NULL;
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

const struct PMap *wlr_mode_pmap_init(void) {
	const struct PMapParams params = {
		.free_val = (fn_free)wlr_mode_free,
	};
	return pmap_init_with(params);
}

void wlr_mode_free(struct WlrMode *wlr_mode) {
	if (!wlr_mode)
		return;

	free(wlr_mode);
}

void mode_res_refresh_free(struct ModesResRefresh *mrr) {
	if (!mrr)
		return;

	slist_free(&mrr->wlr_modes);
	free(mrr);
}


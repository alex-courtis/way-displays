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
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"

// TODO unit tests mostly missing, needs to be "merged" with usermode

struct Mode *mode_preferred(struct SList *modes, struct SList *modes_failed) {
	struct Mode *mode = NULL;

	for (struct SList *i = modes; i; i = i->nex) {
		if (!i->val)
			continue;
		mode = i->val;

		if (mode->preferred && !slist_find_equal(modes_failed, NULL, mode)) {
			return mode;
		}
	}

	return NULL;
}

struct Mode *mode_max_preferred(struct SList *modes, struct SList *modes_failed) {
	const struct Mode *preferred = mode_preferred(modes, modes_failed);

	if (!preferred)
		return NULL;

	struct Mode *mode = NULL, *max = NULL;

	for (struct SList *i = modes; i; i = i->nex) {
		if (!i->val)
			continue;
		mode = i->val;

		if (slist_find_equal(modes_failed, NULL, mode)) {
			continue;
		}

		if (mode->width != preferred->width || mode->height != preferred->height) {
			continue;
		}

		if (!max) {
			max = mode;
		} else if (mode->refresh_mhz > max->refresh_mhz) {
			max = mode;
		}
	}

	return max;
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

static bool mode_equal_res_hz(const struct Mode* const a, const struct Mode* const b) {
	if (!a || !b) {
		return false;
	}

	return a->width == b->width &&
		a->height == b->height &&
		mhz_to_hz_rounded(a->refresh_mhz) == mhz_to_hz_rounded(b->refresh_mhz);
}

static bool mode_equal_user_mode_res_hz(const struct Mode* const mode, const struct UserMode* const user_mode) {
	if (!mode || !user_mode) {
		return false;
	}

	return mode->width == user_mode->width &&
		mode->height == user_mode->height &&
		mode->refresh_mhz == user_mode->refresh_mhz;
}

bool mode_greater_than_res_refresh(const struct Mode* const a, const struct Mode* const b) {
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

double mode_dpi(struct Mode *mode) {
	if (!mode || !mode->head || !mode->head->width_mm || !mode->head->height_mm) {
		return 0;
	}

	double dpi_horiz = (double)(mode->width) / mode->head->width_mm * 25.4;
	double dpi_vert = (double)(mode->height) / mode->head->height_mm * 25.4;
	return (dpi_horiz + dpi_vert) / 2;
}

double mode_scale(struct Mode *mode) {
	double dpi = mode_dpi(mode);

	if (dpi == 0) {
		return 1;
	}

	return dpi / (g_cfg->auto_scale_dpi ? g_cfg->auto_scale_dpi : AUTO_SCALE_DPI_DEFAULT);
}

struct SList *modes_res_refresh(struct SList *modes) {
	struct SList *mrrs = NULL;

	struct SList *sorted = slist_sort(modes, (fn_less_than)mode_greater_than_res_refresh);

	struct ModesResRefresh *mrr = NULL;
	struct Mode *mode = NULL;
	for (struct SList *i = sorted; i; i = i->nex) {
		mode = i->val;

		if (!mrr || !mode_equal_res_hz(mode, mrr->modes->val)) {
			mrr = calloc(1, sizeof(struct ModesResRefresh));
			mrr->width = mode->width;
			mrr->height = mode->height;
			mrr->refresh_mhz = mode->refresh_mhz;
			slist_append(&mrrs, mrr);
		}

		slist_append(&mrr->modes, mode);
	}

	slist_free(&sorted);

	return mrrs;
}

struct Mode *mode_user_mode(struct SList *modes, struct SList *modes_failed, const struct UserMode *user_mode) {
	if (!modes || !user_mode)
		return NULL;

	struct SList *i, *j;

	// exact res and refresh
	struct Mode *mode_exact = slist_find_equal_val(modes, (fn_equal)mode_equal_user_mode_res_hz, user_mode);
	if (mode_exact && !slist_find_equal_val(modes_failed, NULL, mode_exact)) {
		return mode_exact;
	}

	// highest mode matching the user mode
	struct SList *mrrs = modes_res_refresh(modes);
	for (i = mrrs; i; i = i->nex) {
		struct ModesResRefresh *mrr = i->val;
		if (mrr && mrr_satisfies_user_mode(mrr, user_mode)) {
			for (j = mrr->modes; j; j = j->nex) {
				struct Mode *mode = j->val;
				if (!slist_find_equal(modes_failed, NULL, mode)) {
					slist_free_vals(&mrrs, (fn_free)mode_res_refresh_free);
					return mode;
				}
			}
		}
	}
	slist_free_vals(&mrrs, (fn_free)mode_res_refresh_free);

	return NULL;
}

struct Mode *mode_init(struct Head *head, struct zwlr_output_mode_v1 *zwlr_mode, int32_t width, int32_t height, int32_t refresh_mhz, bool preferred) {
	struct Mode *mode = calloc(1, sizeof(struct Mode));

	mode->head = head;
	mode->zwlr_mode = zwlr_mode;
	mode->width = width;
	mode->height = height;
	mode->refresh_mhz = refresh_mhz;
	mode->preferred = preferred;

	return mode;
}

void mode_free(struct Mode *mode) {
	if (!mode)
		return;

	free(mode);
}

void mode_res_refresh_free(struct ModesResRefresh *mrr) {
	if (!mrr)
		return;

	slist_free(&mrr->modes);
	free(mrr);
}


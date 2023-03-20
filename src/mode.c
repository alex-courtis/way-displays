#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "mode.h"

#include "cfg.h"
#include "head.h"
#include "list.h"

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
	struct Mode *preferred = mode_preferred(modes, modes_failed);

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

int32_t mhz_to_hz(int32_t mhz) {
	return (mhz + 500) / 1000;
}

bool equal_mode_res_hz(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct Mode *lhs = (struct Mode*)a;
	struct Mode *rhs = (struct Mode*)b;

	return lhs->width == rhs->width &&
		lhs->height == rhs->height &&
		mhz_to_hz(lhs->refresh_mhz) == mhz_to_hz(rhs->refresh_mhz);
}

bool greater_than_res_refresh(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct Mode *lhs = (struct Mode*)a;
	struct Mode *rhs = (struct Mode*)b;

	if (lhs->width > rhs->width) {
		return true;
	} else if (lhs->width != rhs->width) {
		return false;
	}

	if (lhs->height > rhs->height) {
		return true;
	} else if (lhs->height != rhs->height) {
		return false;
	}

	if (lhs->refresh_mhz > rhs->refresh_mhz) {
		return true;
	}

	return false;
}

bool mrr_satisfies_user_mode(struct ModesResRefresh *mrr, struct UserMode *user_mode) {
	if (!mrr || !user_mode) {
		return false;
	}

	return user_mode->max ||
		(mrr->width == user_mode->width &&
		 mrr->height == user_mode->height &&
		 (user_mode->refresh_hz == -1 || mrr->refresh_hz == user_mode->refresh_hz));
}

double mode_dpi(struct Mode *mode) {
	if (!mode || !mode->head || !mode->head->width_mm || !mode->head->height_mm) {
		return 0;
	}

	double dpi_horiz = (double)(mode->width) / mode->head->width_mm * 25.4;
	double dpi_vert = (double)(mode->height) / mode->head->height_mm * 25.4;
	return (dpi_horiz + dpi_vert) / 2;
}

struct SList *modes_res_refresh(struct SList *modes) {
	struct SList *mrrs = NULL;

	struct SList *sorted = slist_sort(modes, greater_than_res_refresh);

	struct ModesResRefresh *mrr = NULL;
	struct Mode *mode = NULL;
	for (struct SList *i = sorted; i; i = i->nex) {
		mode = i->val;

		if (!mrr || !equal_mode_res_hz(mode, mrr->modes->val)) {
			mrr = calloc(1, sizeof(struct ModesResRefresh));
			mrr->width = mode->width;
			mrr->height = mode->height;
			mrr->refresh_hz = mhz_to_hz(mode->refresh_mhz);
			slist_append(&mrrs, mrr);
		}

		slist_append(&mrr->modes, mode);
	}

	slist_free(&sorted);

	return mrrs;
}

struct Mode *mode_user_mode(struct SList *modes, struct SList *modes_failed, struct UserMode *user_mode) {
	if (!modes || !user_mode)
		return NULL;

	struct SList *i, *j;

	// highest mode matching the user mode
	struct SList *mrrs = modes_res_refresh(modes);
	for (i = mrrs; i; i = i->nex) {
		struct ModesResRefresh *mrr = i->val;
		if (mrr && mrr_satisfies_user_mode(mrr, user_mode)) {
			for (j = mrr->modes; j; j = j->nex) {
				struct Mode *mode = j->val;
				if (!slist_find_equal(modes_failed, NULL, mode)) {
					slist_free_vals(&mrrs, mode_res_refresh_free);
					return mode;
				}
			}
		}
	}
	slist_free_vals(&mrrs, mode_res_refresh_free);

	return NULL;
}

void mode_free(void *data) {
	struct Mode *mode = data;

	if (!mode)
		return;

	free(mode);
}

void mode_res_refresh_free(void *data) {
	struct ModesResRefresh *modes_res_refresh = data;

	if (!modes_res_refresh)
		return;

	slist_free(&modes_res_refresh->modes);
	free(modes_res_refresh);
}


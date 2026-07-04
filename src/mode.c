#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mode.h"

#include "cfg.h"
#include "fn.h"
#include "head.h"
#include "pset.h"
#include "smap.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"

struct Mode *mode_init(void) {
	struct Mode *mode = calloc(1, sizeof(struct Mode));

	mode->width = -1;
	mode->height = -1;
	mode->refresh_mhz = -1;

	return mode;
}

struct Mode *mode_clone(const struct Mode * const from) {
	if (!from)
		return NULL;

	struct Mode *to = (struct Mode*)calloc(1, sizeof(struct Mode));

	memcpy(to, from, sizeof(struct Mode));

	return to;
}

const struct SMap *mode_smap_init(void) {
	const struct SMapParams params = {
		.equal_val = (fn_equal)mode_equal,
		.free_val = (fn_free)mode_free,
		.str_val = (fn_str)mode_str,
		.clone_val = (fn_clone)mode_clone,
	};
	return smap_init_with(params);
}

const struct PSet *mode_pset_init(void) {
	const struct PSetParams params = {
		.free_val = (fn_free)mode_free,
		.str_val = (fn_str)mode_str,
	};
	return pset_init_with(params);
}

void mode_free(struct Mode *mode) {
	free(mode);
}

bool mode_equal(const struct Mode* const a, const struct Mode* const b) {
	return a && b && memcmp(a, b, sizeof(struct Mode)) == 0;
}

bool mode_equal_res_hz(const struct Mode* const a, const struct Mode* const b) {
	return a && b &&
		a->width == b->width &&
		a->height == b->height &&
		mode_hz_rounded(a) == mode_hz_rounded(b);
}

static bool mode_equal_res_mhz(const struct Mode* const a, const struct Mode* const b) {
	return a && b &&
		a->width == b->width &&
		a->height == b->height &&
		a->refresh_mhz == b->refresh_mhz;
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

char *mode_str(const struct Mode * const mode) {
	if (!mode)
		return NULL;

	return sprintf_alloc("%dx%d@%dHz (%d,%03dmHz)%s",
			mode->width,
			mode->height,
			mode_hz_rounded(mode),
			mode->refresh_mhz / 1000,
			mode->refresh_mhz % 1000,
			mode->preferred ? " (preferred)" : ""
			);
}

char *mode_str_brief(const struct Mode * const mode) {
	if (!mode)
		return NULL;

	if (mode->max) {
		return sprintf_alloc("MAX");
	} else if (mode->refresh_mhz != -1) {
		return sprintf_alloc("%dx%d@%gHz",
				mode->width,
				mode->height,
				((float)mode->refresh_mhz) / 1000
				);
	} else {
		return sprintf_alloc("%dx%d",
				mode->width,
				mode->height
				);
	}
}

bool mode_is_preferred(const struct Mode *mode, const void* const unused) {
	return mode && mode->preferred;
}

bool mode_is_zwlr_mode(const struct Mode *mode, const struct zwlr_output_mode_v1 *zwlr_mode) {
	return mode ? mode->zwlr_mode == zwlr_mode : false;
}

static bool mode_satisfies(const struct Mode* const mode, const struct Mode *mode_target) {
	if (!mode || !mode_target)
		return false;

	return mode_target->max ||
		(mode->width == mode_target->width &&
		 mode->height == mode_target->height &&
		 (mode_target->refresh_mhz == -1 || mode_hz_rounded(mode) == mode_hz_rounded(mode_target)));
}

int32_t mode_hz_rounded(const struct Mode* const mode) {
	return mode ? (mode->refresh_mhz + 500) / 1000 : 0;
}

double mode_dpi(const struct Mode* const mode) {
	if (!mode || !mode->head || !mode->head->width_mm || !mode->head->height_mm) {
		return 0;
	}

	double dpi_horiz = (double)(mode->width) / mode->head->width_mm * 25.4;
	double dpi_vert = (double)(mode->height) / mode->head->height_mm * 25.4;
	return (dpi_horiz + dpi_vert) / 2;
}

double mode_scale(const struct Mode* const mode) {
	double dpi = mode_dpi(mode);

	if (dpi == 0) {
		return 1;
	}

	return dpi / (g_cfg->auto_scale_dpi ? g_cfg->auto_scale_dpi : AUTO_SCALE_DPI_DEFAULT);
}

const struct Mode *mode_preferred(const struct PSet* const modes, const struct PSet* const modes_failed) {
	const struct Mode *mode_preferred = NULL;

	const struct PSetIt *it;

	for (it = pset_it(modes); it; it = pset_it_next(it)) {
		const struct Mode *mode = it->val;

		if (mode->preferred && !pset_contains(modes_failed, mode)) {
			mode_preferred = mode;
			break;
		}
	}

	pset_it_free(it);
	return mode_preferred;
}

const struct Mode *mode_max_preferred(const struct PSet* modes, const struct PSet* const modes_failed) {
	const struct Mode *preferred = mode_preferred(modes, modes_failed);

	if (!preferred)
		return NULL;

	const struct Mode *mode = NULL;
	const struct Mode *mode_max = NULL;

	for (const struct PSetIt *it = pset_it(modes); it; it = pset_it_next(it)) {
		mode = it->val;

		if (pset_contains(modes_failed, mode)) {
			continue;
		}

		if (mode->width != preferred->width || mode->height != preferred->height) {
			continue;
		}

		if (!mode_max) {
			mode_max = mode;
		} else if (mode->refresh_mhz > mode_max->refresh_mhz) {
			mode_max = mode;
		}
	}

	return mode_max;
}

const struct Mode *mode_best_satisfying(const struct Mode * const mode_target, const struct PSet* const modes, const struct PSet* const modes_failed) {

	const struct PSet *candidates = pset_clone_shallow(modes);

	// remove failed modes
	for (const struct PSetIt *it = pset_it(modes_failed); it; it = pset_it_next(it))
		pset_remove(candidates, it->val);

	// search from the top down
	pset_sort(candidates, (fn_less_than)mode_greater_than_res_refresh);

	// exact match first
	const struct Mode *mode = pset_match(candidates, (fn_match_ptr)mode_equal_res_mhz, mode_target);

	// otherwise best match
	mode = mode ? mode : pset_match(candidates, (fn_match_ptr)mode_satisfies, mode_target);

	pset_free(candidates);

	return mode;
}


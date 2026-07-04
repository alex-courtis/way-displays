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

	*to = *from;

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
		mhz_to_hz_rounded(a->refresh_mhz) == mhz_to_hz_rounded(b->refresh_mhz);
}

// TODO inline
bool mode_equal_user_mode_res_mhz(const struct Mode* const mode, const struct Mode* const user_mode) {
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

char *mode_str(const struct Mode * const mode) {
	if (!mode)
		return NULL;

	return sprintf_alloc("%dx%d@%dHz (%d,%03dmHz)%s",
			mode->width,
			mode->height,
			mhz_to_hz_rounded(mode->refresh_mhz),
			mode->refresh_mhz / 1000,
			mode->refresh_mhz % 1000,
			mode->preferred ? " (preferred)" : ""
			);
}

char *user_mode_str(const struct Mode * const mode) {
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

// TODO inline
bool mode_satisfies_user_mode(const struct Mode* const mode, const struct Mode *user_mode) {
	if (!mode || !user_mode)
		return false;

	return user_mode->max ||
		(mode->width == user_mode->width &&
		 mode->height == user_mode->height &&
		 (user_mode->refresh_mhz == -1 || mhz_to_hz_rounded(mode->refresh_mhz) == mhz_to_hz_rounded(user_mode->refresh_mhz)));
}

bool user_mode_invalid(const char* const name_desc, const struct Mode* const user_mode, const void* const data) {
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

const struct Mode *mode_for_user_mode(const struct PSet* const modes, const struct PSet* const modes_failed, const struct Mode *user_mode) {
	if (!modes || !user_mode)
		return NULL;

	// TODO strip modes failed first

	const struct Mode *mode = NULL;

	// exact res and refresh
	const struct Mode *mode_exact = pset_match(modes, (fn_match_ptr)mode_equal_user_mode_res_mhz, user_mode);
	if (mode_exact && !pset_contains(modes_failed, mode_exact)) {
		return mode_exact;
	}

	// search from the top down
	const struct PSet *modes_sorted = pset_clone_shallow(modes);
	pset_sort(modes_sorted, (fn_less_than)mode_greater_than_res_refresh);

	// first matching the user mode
	for (const struct PSetIt *it = pset_match_it(modes_sorted, (fn_match_ptr)mode_satisfies_user_mode, user_mode); it; it = pset_it_next(it)) {
		if (!pset_contains(modes_failed, it->val)) {
			mode = it->val;
			pset_it_free(it);
			break;
		}
	}

	pset_free(modes_sorted);

	return mode;
}


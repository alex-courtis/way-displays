#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include <stdint.h>

#include "pset.h"
#include "wlr-output-management-unstable-v1.h"

/*
 * Mode contexts:
 * - Cfg: partial dimensions or max
 * - State: dimensions and preferred
 * - Head: dimensions, preferred and pointers
 */

struct Mode {
	struct Head *head;

	struct zwlr_output_mode_v1 *zwlr_mode;

	int32_t width;
	int32_t height;
	int32_t refresh_mhz;

	bool max;

	bool preferred;

	bool warned_no_mode;
};

/*
 * lifecycle
 */

struct Mode *mode_init(void);

struct Mode *mode_clone(const struct Mode * const from);

const struct PSet *mode_pset_init(void);

const struct SMap *mode_smap_init(void);

void mode_free(struct Mode *mode);

/*
 * equals
 */

bool mode_equal(const struct Mode* const a, const struct Mode* const b);

bool mode_equal_res_mhz(const struct Mode* const a, const struct Mode* const b);

bool mode_equal_res_hz(const struct Mode* const a, const struct Mode* const b);

/*
 * comparison
 */

bool mode_greater_than_res_refresh(const struct Mode* const a, const struct Mode* const b);

/*
 * rendering
 */

char *mode_str(const struct Mode * const mode);

char *mode_str_brief(const struct Mode * const mode);

/*
 * predicates
 */

bool mode_is_preferred(const struct Mode *mode, const void* const unused);

bool mode_is_zwlr_mode(const struct Mode *mode, const struct zwlr_output_mode_v1 *zwlr_mode);

/*
 * utility
 */

int32_t mode_mhz_to_hz_rounded(int32_t mhz);

double mode_dpi(const struct Mode* const mode);

double mode_scale(const struct Mode* const mode);

/*
 * search
 */

const struct Mode *mode_preferred(const struct PSet* const modes, const struct PSet* const modes_failed);

const struct Mode *mode_max_preferred(const struct PSet* modes, const struct PSet* const modes_failed);

const struct Mode *mode_best_satisfying(const struct Mode * const mode_target, const struct PSet* const modes, const struct PSet* const modes_failed);

#endif // MODE_H


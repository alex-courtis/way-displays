#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include <stdint.h>

#include "pset.h"
#include "wlr-output-management-unstable-v1.h"

/*
 * Mode contexts:
 * - Cfg: partial res/refresh or max
 * - State: res/refresh
 * - Head: res/refresh and pointers
 */

struct Mode {
	struct Head *head;

	struct zwlr_output_mode_v1 *zwlr_mode;

	int32_t width;
	int32_t height;
	int32_t refresh_mhz;

	bool max;

	bool warned_no_mode;
};

/*
 * lifecycle
 */

struct Mode *mode_init(void);

struct Mode *mode_clone(const struct Mode * const from);

// raw pointers
const struct PSet *mode_pset_init(void);

// mode_equal
const struct SMap *mode_smap_init(void);

void mode_free(struct Mode *mode);

/*
 * equals
 */

// exact pointers and values - memcmp
bool mode_equal(const struct Mode* const a, const struct Mode* const b);

// w/h and rounded refresh
bool mode_equal_res_hz(const struct Mode* const a, const struct Mode* const b);

// w/h and exact refresh
bool mode_equal_res_mhz(const struct Mode* const a, const struct Mode* const b);

/*
 * comparison
 */

// w greater, h greater when w equal, refresh greater than when w/h equal
bool mode_greater_than_res_refresh(const struct Mode* const a, const struct Mode* const b);

/*
 * to string
 */

// WxH@Hz (mHz) (preferred)
char *mode_str(const struct Mode * const mode);

// MAX, WxH@Hz, WxH
char *mode_str_brief(const struct Mode * const mode);

/*
 * predicates
 */

bool mode_is_zwlr_mode(const struct Mode *mode, const struct zwlr_output_mode_v1 *zwlr_mode);

// target is MAX  or  WxH@Hz  or  WxH with no target refresh
bool mode_satisfies(const struct Mode* const mode, const struct Mode *mode_target);

/*
 * utility
 */

// mHz rounded up to Hz
int32_t mode_hz_rounded(const struct Mode* const mode);

// DPI when mode applied to head, 0 when no head or no head dimensions
double mode_dpi(const struct Mode* const mode);

// DPI / AUTO_SCALE_DPI, 1 when no DPI available
double mode_scale(const struct Mode* const mode);

/*
 * search
 */

// highest refresh matching target resolution, NULL when no target
const struct Mode *mode_max_refresh(const struct Mode* const mode_target, const struct PSet* modes, const struct PSet* const modes_failed);

// mode exactly matching target otherwise mode_satisfies, NULL when no target
const struct Mode *mode_best_satisfying(const struct Mode * const mode_target, const struct PSet* const modes, const struct PSet* const modes_failed);

#endif // MODE_H


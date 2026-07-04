#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include <stdint.h>

#include "cfg/user-mode.h"
#include "pset.h"
#include "wlr-output-management-unstable-v1.h"

struct WlrMode {
	struct Head *head;

	struct zwlr_output_mode_v1 *zwlr_mode;

	int32_t width;
	int32_t height;
	int32_t refresh_mhz;
	bool preferred;
};

/*
 * lifecycle
 */
struct WlrMode *wlr_mode_init(struct Head *head, struct zwlr_output_mode_v1 *zwlr_mode, int32_t width, int32_t height, int32_t refresh_mhz, bool preferred);

const struct PSet *wlr_mode_pset_init(void);

void wlr_mode_free(struct WlrMode *wlr_mode);

/*
 * equals
 */
bool wlr_mode_equal(const struct WlrMode* const a, const struct WlrMode* const b);

bool wlr_mode_equal_user_mode_res_mhz(const struct WlrMode* const wlr_mode, const struct UserMode* const user_mode);

bool wlr_mode_equal_res_hz(const struct WlrMode* const a, const struct WlrMode* const b);

/*
 * comparison
 */
bool wlr_mode_greater_than_res_refresh(const struct WlrMode* const a, const struct WlrMode* const b);

/*
 * rendering
 */
char *wlr_mode_str(const struct WlrMode * const wlr_mode);

/*
 * predicates
 */
bool wlr_mode_is_preferred(const struct WlrMode *wlr_mode, const void* const unused);

bool wlr_mode_is_zwlr_mode(const struct WlrMode *wlr_mode, const struct zwlr_output_mode_v1 *zwlr_mode);

bool wlr_mode_satisfies_user_mode(const struct WlrMode* const wlr_mode, const struct UserMode *user_mode);

/*
 * utility
 */

int32_t mhz_to_hz_rounded(int32_t mhz);

double wlr_mode_dpi(const struct WlrMode* const wlr_mode);

double wlr_mode_scale(const struct WlrMode* const wlr_mode);

/*
 * search
 */
const struct WlrMode *wlr_mode_preferred(const struct PSet* const wlr_modes, const struct PSet* const wlr_modes_failed);

const struct WlrMode *wlr_mode_max_preferred(const struct PSet* wlr_modes, const struct PSet* const wlr_modes_failed);

const struct WlrMode *wlr_mode_for_user_mode(const struct PSet* const wlr_modes, const struct PSet* const wlr_modes_failed, const struct UserMode *user_mode);

#endif // MODE_H


#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include <stdint.h>

#include "pset.h"
#include "wlr-output-management-unstable-v1.h"

struct WlrMode {
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

struct WlrMode *wlr_mode_init(void);

struct WlrMode *wlr_mode_clone(const struct WlrMode * const from);

const struct PSet *wlr_mode_pset_init(void);

const struct SMap *wlr_mode_smap_init(void);

void wlr_mode_free(struct WlrMode *wlr_mode);

/*
 * equals
 */
bool wlr_mode_equal(const struct WlrMode* const a, const struct WlrMode* const b);

bool wlr_mode_equal_user_mode_res_mhz(const struct WlrMode* const wlr_mode, const struct WlrMode* const user_mode);

bool wlr_mode_equal_res_hz(const struct WlrMode* const a, const struct WlrMode* const b);

/*
 * comparison
 */
bool wlr_mode_greater_than_res_refresh(const struct WlrMode* const a, const struct WlrMode* const b);

/*
 * rendering
 */
char *wlr_mode_str(const struct WlrMode * const wlr_mode);

// TODO normalise with wlr_mode_str or move to info
char *user_mode_str(const struct WlrMode * const user_mode);

/*
 * predicates
 */
bool wlr_mode_is_preferred(const struct WlrMode *wlr_mode, const void* const unused);

bool wlr_mode_is_zwlr_mode(const struct WlrMode *wlr_mode, const struct zwlr_output_mode_v1 *zwlr_mode);

bool wlr_mode_satisfies_user_mode(const struct WlrMode* const wlr_mode, const struct WlrMode *user_mode);

bool user_mode_invalid(const char* const name_desc, const struct WlrMode* const user_mode, const void* const data);

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

const struct WlrMode *wlr_mode_for_user_mode(const struct PSet* const wlr_modes, const struct PSet* const wlr_modes_failed, const struct WlrMode *user_mode);

#endif // MODE_H


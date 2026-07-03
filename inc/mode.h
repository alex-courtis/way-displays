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

const struct WlrMode *mode_preferred(const struct PSet* const wlr_modes, const struct PSet* const wlr_modes_failed);

const struct WlrMode *mode_max_preferred(const struct PSet* wlr_modes, const struct PSet* const wlr_modes_failed);

bool mode_greater_than_res_refresh(const struct WlrMode* const a, const struct WlrMode* const b);

char *wlr_mode_str(const struct WlrMode * const wlr_mode);

// up to 3 d.p.
const char *mhz_to_hz_str(int32_t mhz);

// hz float string to milliHz, 0 on failure
int32_t hz_str_to_mhz(const char *hz_str);

// rounded integer
int32_t mhz_to_hz_rounded(int32_t mhz);

double mode_dpi(const struct WlrMode* const wlr_mode);

double mode_scale(const struct WlrMode* const wlr_mode);

struct WlrMode *wlr_mode_init(struct Head *head, struct zwlr_output_mode_v1 *zwlr_mode, int32_t width, int32_t height, int32_t refresh_mhz, bool preferred);

const struct PSet *wlr_mode_pset_init(void);

bool wlr_mode_match_preferred(const struct WlrMode *wlr_mode, const void* const data);

bool wlr_mode_match_zwlr_mode(const struct WlrMode *wlr_mode, const struct zwlr_output_mode_v1 *zwlr_mode);

void wlr_mode_free(struct WlrMode *wlr_mode);

const struct WlrMode *mode_user_mode(const struct PSet* const wlr_modes, const struct PSet* const wlr_modes_failed, const struct UserMode *user_mode);

#endif // MODE_H


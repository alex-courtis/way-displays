#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include <stdint.h>

#include "cfg/user-mode.h"
#include "slist.h"
#include "pmap.h"
#include "wlr-output-management-unstable-v1.h"

struct WlrMode {
	struct Head *head;

	struct zwlr_output_mode_v1 *zwlr_mode;

	int32_t width;
	int32_t height;
	int32_t refresh_mhz;
	bool preferred;
};

struct ModesResRefresh {
	int32_t width;
	int32_t height;
	int32_t refresh_mhz;
	struct SList *wlr_modes;
};

const struct WlrMode *mode_preferred(const struct PMap* const wlr_modes, struct SList *wlr_modes_failed);

const struct WlrMode *mode_max_preferred(const struct PMap* wlr_modes, struct SList *wlr_modes_failed);

bool mode_greater_than_res_refresh(const struct WlrMode* const a, const struct WlrMode* const b);

// up to 3 d.p.
const char *mhz_to_hz_str(int32_t mhz);

// hz float string to milliHz, 0 on failure
int32_t hz_str_to_mhz(const char *hz_str);

// rounded integer
int32_t mhz_to_hz_rounded(int32_t mhz);

double mode_dpi(const struct WlrMode* const wlr_mode);

double mode_scale(const struct WlrMode* const wlr_mode);

struct SList *modes_res_refresh(const struct PMap* const wlr_modes);

struct WlrMode *wlr_mode_init(struct Head *head, struct zwlr_output_mode_v1 *zwlr_mode, int32_t width, int32_t height, int32_t refresh_mhz, bool preferred);

const struct PMap *wlr_mode_pmap_init(void);

void wlr_mode_free(struct WlrMode *wlr_mode);

void mode_res_refresh_free(struct ModesResRefresh *mrr);

const struct WlrMode *mode_user_mode(const struct PMap* const wlr_modes, struct SList *wlr_modes_failed, const struct UserMode *user_mode);

#endif // MODE_H


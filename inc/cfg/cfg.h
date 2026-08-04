#ifndef CFG_H
#define CFG_H

#include <stdbool.h>
#include <stdint.h>

#include "enum.h"

// global singleton
extern struct Cfg *g_cfg;

struct Cfg {
	enum Arrange arrange;	                      // ARRANGE
	enum Align align;                             // ALIGN

	const struct Sset *order_name_desc;           // ORDER

	enum OnOff scaling;                           // SCALING
	enum ScaleRoundStrategy scale_round_strategy; // SCALE_ROUND_STRATEGY
	unsigned int scale_round_to;                  // SCALE_ROUND_TO

	enum OnOff auto_scale;                        // AUTO_SCALE
	int32_t auto_scale_dpi;                       // AUTO_SCALE_DPI
	float auto_scale_min;                         // AUTO_SCALE_MIN
	float auto_scale_max;                         // AUTO_SCALE_MAX

	// by name_desc
	const struct SImap *scales;                   // SCALE               milliscale
	const struct SPmap *modes;                    // MODE                mode_spmap_init
	const struct SImap *transforms;               // TRANSFORM           wl_output_transform
	const struct Sset *adaptive_sync_off;         // VRR_OFF
	const struct Sset *max_preferred_refresh;     // MAX_PREFERRED_REFRESH

	char *callback_cmd;                           // CALLBACK_CMD        empty string means no callback

	char *laptop_display_prefix;                  // LAPTOP_DISPLAY_PREFIX
	enum OnOff laptop_lid_monitor;                // LAPTOP_LID_MONITOR

	enum LogThreshold log_threshold;              // LOG_THRESHOLD

	const struct SPmap *disableds;                // DISABLED           cfg_disabled_spmap_init
};

/*
 * lifecycle - cfg
 */

struct Cfg *cfg_init(void);

// init and cfg_apply_defaults
struct Cfg *cfg_default(void);

struct Cfg *cfg_clone(struct Cfg *from);

void cfg_free(struct Cfg *cfg);

// free and set g_cfg to NULL
void g_cfg_destroy(void);

/*
 * equality
 */

bool cfg_equal(const struct Cfg *a, const struct Cfg *b);

/*
 * mutation
 */

// apply default only for unset values
void cfg_apply_defaults(struct Cfg *cfg);


/*
 * merge from into to for command, only Ipc settable fields are merged
 */
struct Cfg *cfg_merge(struct Cfg *to, const struct Cfg *from, const enum IpcCommand command);
struct Cfg *cfg_merge_set(struct Cfg *to, const struct Cfg *from);
struct Cfg *cfg_merge_toggle(struct Cfg *to, const struct Cfg *from);
struct Cfg *cfg_merge_del(struct Cfg *to, const struct Cfg *from);

/*
 * validation
 */

// validate some elements, logging warnings
void cfg_validate_warn(const struct Cfg * const cfg);

// validate some elements, changing them and logging warnings
void cfg_validate_fix(struct Cfg *cfg);

#endif // CFG_H

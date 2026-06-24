#ifndef CFG_H
#define CFG_H

#include <stdbool.h>
#include <stdint.h>

#include "log.h"
#include "ipc.h"

// global singleton
extern struct Cfg *g_cfg;

#define AUTO_SCALE_DPI_DEFAULT 96
#define AUTO_SCALE_DPI_MIN SCALE_ROUND_TO_DEFAULT

#define AUTO_SCALE_MIN_DEFAULT 1.0f

#define AUTO_SCALE_MAX_DEFAULT -1.0f

#define SCALE_ROUND_TO_DEFAULT 8

enum Arrange {
	ROW = 1,
	COL,
	ARRANGE_DEFAULT = ROW,
};

enum Align {
	TOP = 1,
	MIDDLE,
	BOTTOM,
	LEFT,
	RIGHT,
	ALIGN_DEFAULT = TOP,
};

enum OnOff {
	ON = 1,
	OFF,
	SCALING_DEFAULT = ON,
	AUTO_SCALE_DEFAULT = ON,
	LAPTOP_LID_MONITOR_DEFAULT = ON,
};

enum ScaleRoundStrategy {
	NEAREST = 1,
	UP,
	DOWN,
	SCALE_ROUND_STRATEGY_DEFAULT = NEAREST,
};

#define CALLBACK_CMD_DEFAULT "notify-send \"way-displays ${CALLBACK_LEVEL}\" \"${CALLBACK_MSG}\""

#define COMMENT_YAML_SCHEMA "# yaml-language-server: $schema=https://raw.githubusercontent.com/alex-courtis/way-displays/refs/heads/master/schema/cfg-1.2.0.yaml"

struct Cfg {
	char *dir_path;
	char *file_path;
	char *file_name;
	char *resolved_from;

	bool updated;

	char *callback_cmd;
	char *laptop_display_prefix;
	enum OnOff laptop_lid_monitor;
	struct SList *order_name_desc;
	enum Arrange arrange;
	enum Align align;
	enum OnOff scaling;
	enum OnOff auto_scale;
	const struct SMap *user_scales;
	const struct SMap *user_modes;
	const struct SSet *adaptive_sync_off;
	struct SList *max_preferred_refresh_name_desc;
	const struct PSet *disableds;
	const struct SMap *user_transforms;
	enum LogThreshold log_threshold;

	int32_t auto_scale_dpi;
	float auto_scale_min;
	float auto_scale_max;
	enum ScaleRoundStrategy scale_round_strategy;
	unsigned int scale_round_to;
};

enum CfgElement {
	ARRANGE = 1,
	ALIGN,
	ORDER,
	SCALING,
	AUTO_SCALE,
	SCALE,
	MODE,
	TRANSFORM,
	VRR_OFF,
	CALLBACK_CMD,
	LAPTOP_DISPLAY_PREFIX,
	LAPTOP_LID_MONITOR,
	MAX_PREFERRED_REFRESH,
	LOG_THRESHOLD,
	DISABLED,
	ARRANGE_ALIGN,
	AUTO_SCALE_DPI,
	AUTO_SCALE_MIN,
	AUTO_SCALE_MAX,
	SCALE_ROUND_TO,
	SCALE_ROUND_STRATEGY,
	// legacy
	CHANGE_SUCCESS_CMD,
};

void cfg_file_paths_init(const char *user_path);

void cfg_file_write(void);

void cfg_destroy(void);

void cfg_file_paths_destroy(void);

bool cfg_resolve_file_path(struct Cfg *cfg);

void cfg_copy_file_path(struct Cfg *to, const struct Cfg *from);

struct Cfg *cfg_merge(struct Cfg *to, const struct Cfg *from, const enum IpcCommand command);

void validate_warn(const struct Cfg * const cfg);

void validate_fix(struct Cfg *cfg);

//
// init functions
//
struct Cfg *cfg_init(void);

struct Cfg *cfg_default(void);

void cfg_apply_defaults(struct Cfg *cfg);

struct Disabled *disabled_init_always(const char *name_desc);

//
// equality functions
//
bool cfg_equal(const struct Cfg *a, const struct Cfg *b);

//
// freeing functions
//
void cfg_free(struct Cfg *cfg);

#endif // CFG_H

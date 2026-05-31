#ifndef CFG_H
#define CFG_H

#include <wayland-client-protocol.h>

#include <stdbool.h>
#include <stdint.h>
#include "log.h"
#include "ipc.h"

// global singleton
extern struct Cfg *g_cfg;

#define AUTO_SCALE_DPI_DEFAULT 96

#define AUTO_SCALE_MIN_DEFAULT 1.0f

#define AUTO_SCALE_MAX_DEFAULT -1.0f

#define SCALE_ROUND_TO_DEFAULT 8

struct UserScale {
	char *name_desc;
	float scale;
};

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

struct UserMode {
	char *name_desc;
	bool max;
	int32_t width;
	int32_t height;
	int32_t refresh_mhz;
	bool warned_no_mode;
};

struct UserTransform {
	char *name_desc;
	enum wl_output_transform transform;
};

struct Disabled {
	char *name_desc;
	struct SList *conditions;
};

#define WL_OUTPUT_TRANSFORM_MAX WL_OUTPUT_TRANSFORM_FLIPPED_270

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
	struct SList *user_scales;
	struct SList *user_modes;
	struct SList *adaptive_sync_off_name_desc;
	struct SList *max_preferred_refresh_name_desc;
	struct SList *disabled;
	struct SList *user_transforms;
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

void cfg_copy_file_path(struct Cfg *from, struct Cfg *to);

struct Cfg *cfg_merge(struct Cfg *to, struct Cfg *from, enum IpcCommand command);

//
// init functions
//
struct Cfg *cfg_init(void);

struct Cfg *cfg_default(void);

void cfg_apply_defaults(struct Cfg *cfg);

struct UserMode *cfg_user_mode_init(const char *name_desc, const bool max, const int32_t width, const int32_t height, const int32_t refresh_hz, const bool warned_no_mode);

struct UserMode *cfg_user_mode_default(void);

struct UserScale *cfg_user_scale_init(const char *name_desc, const float scale);

struct UserTransform *cfg_user_transform_init(const char *name_desc, const enum wl_output_transform transform);

struct Disabled *cfg_disabled_always(const char *name_desc);

//
// equality functions
//
bool cfg_equal(const struct Cfg *a, const struct Cfg *b);

//
// freeing functions
//
void cfg_free(struct Cfg *cfg);

void cfg_user_scale_free(const void *val);

void cfg_user_mode_free(const void *val);

void cfg_user_transform_free(const void *val);

void cfg_disabled_free(const void *val);

//
// cloning functions
//
void* cfg_disabled_clone(const void *data);

//
// visible for testing
//
struct Cfg *merge_set(struct Cfg *to, struct Cfg *from);
struct Cfg *merge_toggle(struct Cfg *to, struct Cfg *from);
struct Cfg *merge_del(struct Cfg *to, struct Cfg *from);
void validate_warn(struct Cfg *cfg);
void validate_fix(struct Cfg *cfg);

#endif // CFG_H

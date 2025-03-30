#ifndef CFG_H
#define CFG_H

#include <wayland-client-protocol.h>

#include <stdbool.h>
#include <stdint.h>
#include "log.h"
#include "ipc.h"

#define AUTO_SCALE_MIN_DEFAULT 1.0f
#define AUTO_SCALE_MAX_DEFAULT -1.0f

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

#define WL_OUTPUT_TRANSFORM_MAX WL_OUTPUT_TRANSFORM_FLIPPED_270

#define CALLBACK_CMD_DEFAULT "notify-send \"way-displays ${CALLBACK_LEVEL}\" \"${CALLBACK_MSG}\""

struct Cfg {
	char *dir_path;
	char *file_path;
	char *file_name;
	char *resolved_from;

	bool updated;

	char *callback_cmd;
	char *laptop_display_prefix;
	struct SList *order_name_desc;
	enum Arrange arrange;
	enum Align align;
	enum OnOff scaling;
	enum OnOff auto_scale;
	struct SList *user_scales;
	struct SList *user_modes;
	struct SList *adaptive_sync_off_name_desc;
	struct SList *max_preferred_refresh_name_desc;
	struct SList *disabled_name_desc;
	struct SList *user_transforms;
	enum LogThreshold log_threshold;

	float auto_scale_min;
	float auto_scale_max;
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
	MAX_PREFERRED_REFRESH,
	LOG_THRESHOLD,
	DISABLED,
	ARRANGE_ALIGN,
	AUTO_SCALE_MIN,
	AUTO_SCALE_MAX
};

void cfg_file_paths_init(const char *user_path);

void cfg_init_path(const char *cfg_path);

void cfg_file_reload(void);

void cfg_file_write(void);

void cfg_destroy(void);

void cfg_file_paths_destroy(void);

struct Cfg *cfg_merge(struct Cfg *to, struct Cfg *from, enum IpcCommand command);

//
// init functions
//
struct Cfg *cfg_init(void);

struct Cfg *cfg_default(void);

struct UserMode *cfg_user_mode_init(const char *name_desc, const bool max, const int32_t width, const int32_t height, const int32_t refresh_hz, const bool warned_no_mode);

struct UserMode *cfg_user_mode_default(void);

struct UserScale *cfg_user_scale_init(const char *name_desc, const float scale);

struct UserTransform *cfg_user_transform_init(const char *name_desc, const enum wl_output_transform transform);

//
// equality functions
//
bool cfg_equal(const struct Cfg *a, const struct Cfg *b);

bool cfg_user_scale_name_equal(const void *a, const void *b);

bool cfg_user_mode_name_equal(const void *a, const void *b);

bool cfg_user_transform_name_equal(const void *a, const void *b);

//
// cloning functions
//
void* cfg_user_mode_clone(const void* const val);

void* cfg_user_transform_clone(const void* const val);

void* cfg_user_scale_clone(const void* const val);

//
// freeing functions
//
void cfg_free(struct Cfg *cfg);

void cfg_user_scale_free(const void *val);

void cfg_user_mode_free(const void *val);

void cfg_user_transform_free(const void *val);

//
// visible for testing
//
struct Cfg *merge_set(struct Cfg *to, struct Cfg *from);
struct Cfg *merge_toggle(struct Cfg *to, struct Cfg *from);
struct Cfg *merge_del(struct Cfg *to, struct Cfg *from);
void validate_warn(struct Cfg *cfg);
void validate_fix(struct Cfg *cfg);
bool resolve_cfg_file(struct Cfg *cfg);

#endif // CFG_H

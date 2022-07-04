#ifndef CFG_H
#define CFG_H

#ifdef __cplusplus
#include <yaml-cpp/emitter.h>
#include <yaml-cpp/node/node.h>
#endif

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" { //}
#endif

#include <stdint.h>
#include "log.h"

#include <wayland-client-protocol.h>
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

enum AutoScale {
	ON = 1,
	OFF,
	AUTO_SCALE_DEFAULT = ON,
};

struct UserMode {
	char *name_desc;
	bool max;
	int32_t width;
	int32_t height;
	int32_t refresh_hz;
	bool warned_no_mode;
};

struct UserTransform {
	char *name_desc;
	enum wl_output_transform transform;
};

struct Cfg {
	char *dir_path;
	char *file_path;
	char *file_name;

	bool written;

	char *laptop_display_prefix;
	struct SList *order_name_desc;
	enum Arrange arrange;
	enum Align align;
	enum AutoScale auto_scale;
	struct SList *user_scales;
	struct SList *user_modes;
	struct SList *user_transform;
	struct SList *max_preferred_refresh_name_desc;
	struct SList *disabled_name_desc;
	enum LogThreshold log_threshold;
};

enum CfgElement {
	ARRANGE = 1,
	ALIGN,
	ORDER,
	AUTO_SCALE,
	SCALE,
	MODE,
	TRANSFORM,
	LAPTOP_DISPLAY_PREFIX,
	MAX_PREFERRED_REFRESH,
	LOG_THRESHOLD,
	DISABLED,
	ARRANGE_ALIGN,
};

enum CfgMergeType {
	SET = 1,
	DEL,
};

void cfg_init(void);

struct Cfg *cfg_merge(struct Cfg *to, struct Cfg *from, enum CfgMergeType merge_type);

void cfg_file_reload(void);

void cfg_file_write(void);

struct Cfg *cfg_default(void);

struct UserMode *cfg_user_mode_default(void);

struct UserTransform *cfg_user_transform_default(void);

void cfg_user_scale_free(void *user_scale);

void cfg_user_mode_free(void *user_mode);

void cfg_user_transform_free(void *user_transform);

void cfg_destroy(void);

void cfg_free(struct Cfg *cfg);

#if __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus

void cfg_emit(YAML::Emitter &e, struct Cfg *cfg);

void cfg_parse_node(struct Cfg *cfg, YAML::Node &node);

#endif

#endif // CFG_H


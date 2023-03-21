#ifndef CFG_H
#define CFG_H

#include <stdbool.h>
#include <stdint.h>
#include "log.h"

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
	struct SList *vrr_off_name_desc;
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
	VRR_OFF,
	LAPTOP_DISPLAY_PREFIX,
	MAX_PREFERRED_REFRESH,
	LOG_THRESHOLD,
	DISABLED,
	ARRANGE_ALIGN,
};

void cfg_init(const char *cfg_path);

bool cfg_equal(struct Cfg *a, struct Cfg *b);

struct Cfg *cfg_merge(struct Cfg *to, struct Cfg *from, bool del);

void cfg_file_reload(void);

void cfg_file_write(void);

struct Cfg *cfg_default(void);

struct UserMode *cfg_user_mode_init(const char *name_desc, const bool max, const int32_t width, const int32_t height, const int32_t refresh_hz, const bool warned_no_mode);

struct UserMode *cfg_user_mode_default(void);

struct UserScale *cfg_user_scale_init(const char *name_desc, const float scale);

bool cfg_equal_user_scale_name(const void *value, const void *data);

bool cfg_equal_user_scale(const void *value, const void *data);

bool cfg_equal_user_mode_name(const void *value, const void *data);

bool cfg_equal_user_mode(const void *value, const void *data);

void cfg_user_scale_free(void *user_scale);

void cfg_user_mode_free(void *user_mode);

void cfg_destroy(void);

void cfg_free(struct Cfg *cfg);

#endif // CFG_H


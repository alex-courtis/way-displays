#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client-protocol.h>

#include "cfg.h"

#include "fs.h"
#include "fds.h"
#include "fn.h"
#include "conditions.h"
#include "convert.h"
#include "global.h"
#include "info.h"
#include "ipc.h"
#include "mode.h"
#include "slist.h"
#include "log.h"
#include "marshalling.h"

static enum OnOff on_off_invert(enum OnOff val) {
	return (val == ON) ? OFF : ON;
}

static void cfg_paths_free(struct Cfg *cfg) {
	if (!cfg)
		return;

	free(cfg->dir_path);
	cfg->dir_path = NULL;

	free(cfg->file_path);
	cfg->file_path = NULL;

	free(cfg->file_name);
	cfg->file_name = NULL;

	cfg->resolved_from = NULL;
}

//
// cloning functions
//
static void* cfg_user_mode_clone(const void* const val) {
	struct UserMode *original = (struct UserMode*)val;
	struct UserMode *clone = (struct UserMode*)calloc(1, sizeof(struct UserMode));

	*clone = *original;
	clone->name_desc = strdup(original->name_desc);

	return clone;
}

static void* cfg_user_transform_clone(const void* const val) {
	struct UserTransform *original = (struct UserTransform*)val;
	struct UserTransform *clone = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

	*clone = *original;
	clone->name_desc = strdup(original->name_desc);

	return clone;
}

static void* cfg_user_scale_clone(const void* const val) {
	struct UserScale *original = (struct UserScale*)val;
	struct UserScale *clone = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	*clone = *original;
	clone->name_desc = strdup(original->name_desc);

	return clone;
}

void* cfg_disabled_clone(const void *val) {
	struct Disabled *original = (struct Disabled*)val;
	struct Disabled *clone = (struct Disabled*)calloc(1, sizeof(struct Disabled));

	clone->name_desc = strdup(original->name_desc);
	clone->conditions = slist_clone(original->conditions, condition_clone);

	return clone;
}

//
// equality functions
//
bool cfg_user_mode_name_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct UserMode *lhs = (struct UserMode*)a;
	struct UserMode *rhs = (struct UserMode*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcmp(lhs->name_desc, rhs->name_desc) == 0;
}

bool cfg_user_scale_name_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct UserScale *lhs = (struct UserScale*)a;
	struct UserScale *rhs = (struct UserScale*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcmp(lhs->name_desc, rhs->name_desc) == 0;
}

static bool cfg_user_scale_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct UserScale *lhs = (struct UserScale*)a;
	struct UserScale *rhs = (struct UserScale*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcmp(lhs->name_desc, rhs->name_desc) == 0 && lhs->scale == rhs->scale;
}

static bool cfg_user_mode_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct UserMode *lhs = (struct UserMode*)a;
	struct UserMode *rhs = (struct UserMode*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	if (strcmp(lhs->name_desc, rhs->name_desc) != 0) {
		return false;
	}

	if (lhs->max != rhs->max) {
		return false;
	}

	if (lhs->width != rhs->width || lhs->height != rhs->height) {
		return false;
	}

	if ((lhs->refresh_mhz != -1 || rhs->refresh_mhz != -1) && lhs->refresh_mhz != rhs->refresh_mhz) {
		return false;
	}

	return true;
}

bool cfg_user_transform_name_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct UserTransform *lhs = (struct UserTransform*)a;
	struct UserTransform *rhs = (struct UserTransform*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcmp(lhs->name_desc, rhs->name_desc) == 0;
}

static bool cfg_user_transform_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct UserTransform *lhs = (struct UserTransform*)a;
	struct UserTransform *rhs = (struct UserTransform*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	if (strcmp(lhs->name_desc, rhs->name_desc) != 0) {
		return false;
	}

	if (lhs->transform != rhs->transform) {
		return false;
	}

	return true;
}

bool cfg_disabled_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct Disabled *lhs = (struct Disabled*)a;
	struct Disabled *rhs = (struct Disabled*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	if (strcmp(lhs->name_desc, rhs->name_desc) != 0) {
		return false;
	}

	return slist_equal(lhs->conditions, rhs->conditions, condition_equal);
}

static bool invalid_user_scale(const void *a, const void *b) {
	if (!a) {
		return true;
	}

	struct UserScale *user_scale = (struct UserScale*)a;

	if (user_scale->scale <= 0) {
		log_warn("\nIgnoring non-positive SCALE %s %.3f", user_scale->name_desc, user_scale->scale);
		return true;
	}

	return false;
}

static bool invalid_user_mode(const void *a, const void *b) {
	if (!a) {
		return true;
	}
	struct UserMode *user_mode = (struct UserMode*)a;

	if (user_mode->width != -1 && user_mode->width <= 0) {
		log_warn("\nIgnoring non-positive MODE %s WIDTH %d", user_mode->name_desc, user_mode->width);
		return true;
	}
	if (user_mode->height != -1 && user_mode->height <= 0) {
		log_warn("\nIgnoring non-positive MODE %s HEIGHT %d", user_mode->name_desc, user_mode->height);
		return true;
	}
	if (user_mode->refresh_mhz != -1 && user_mode->refresh_mhz <= 0) {
		log_warn("\nIgnoring non-positive MODE %s HZ %s", user_mode->name_desc, mhz_to_hz_str(user_mode->refresh_mhz));
		return true;
	}

	if (!user_mode->max) {
		if (user_mode->width == -1) {
			log_warn("\nIgnoring invalid MODE %s missing WIDTH", user_mode->name_desc);
			return true;
		}
		if (user_mode->height == -1) {
			log_warn("\nIgnoring invalid MODE %s missing HEIGHT", user_mode->name_desc);
			return true;
		}
	}

	return false;
}

static void warn_short_name_desc(const char *name_desc, const char *element) {
	if (!name_desc)
		return;

	if (strlen(name_desc) < 4) {
		log_warn("\n%s '%s' is less than 4 characters, which may result in some unwanted matches.", element, name_desc);
	}
}

static struct Cfg *clone_cfg(struct Cfg *from) {
	if (!from) {
		return NULL;
	}

	struct Cfg *to = cfg_init();

	to->dir_path = from->dir_path ? strdup(from->dir_path) : NULL;
	to->file_path = from->file_path ? strdup(from->file_path) : NULL;
	to->file_name = from->file_name ? strdup(from->file_name) : NULL;

	// ARRANGE
	if (from->arrange) {
		to->arrange = from->arrange;
	}

	// ALIGN
	if (from->align) {
		to->align = from->align;
	}

	// ORDER
	to->order_name_desc = slist_clone(from->order_name_desc, fn_clone_strdup);

	// SCALING
	if (from->scaling) {
		to->scaling = from->scaling;
	}

	// AUTO_SCALE
	to->auto_scale = from->auto_scale;
	to->auto_scale_min = from->auto_scale_min;
	to->auto_scale_max = from->auto_scale_max;

	// SCALE
	to->user_scales = slist_clone(from->user_scales, cfg_user_scale_clone);

	// TRANSFORM
	to->user_transforms = slist_clone(from->user_transforms, cfg_user_transform_clone);

	// MODE
	to->user_modes = slist_clone(from->user_modes, cfg_user_mode_clone);

	// VRR_OFF
	to->adaptive_sync_off_name_desc = slist_clone(from->adaptive_sync_off_name_desc, fn_clone_strdup);

	// CALLBACK_CMD
	if (from->callback_cmd) {
		to->callback_cmd = strdup(from->callback_cmd);
	}

	// LAPTOP_DISPLAY_PREFIX
	if (from->laptop_display_prefix) {
		to->laptop_display_prefix = strdup(from->laptop_display_prefix);
	}

	// MAX_PREFERRED_REFRESH
	to->max_preferred_refresh_name_desc = slist_clone(from->max_preferred_refresh_name_desc, fn_clone_strdup);

	// DISABLED
	to->disabled = slist_clone(from->disabled, cfg_disabled_clone);

	// LOG_THRESHOLD
	if (from->log_threshold) {
		to->log_threshold = from->log_threshold;
	}

	return to;
}

bool cfg_equal(const struct Cfg *a, const struct Cfg *b) {
	if (!a || !b) {
		return false;
	}

	// ARRANGE
	if (a->arrange != b->arrange) {
		return false;
	}

	// ALIGN
	if (a->align != b->align) {
		return false;
	}

	// ORDER
	if (!slist_equal(a->order_name_desc, b->order_name_desc, fn_comp_equals_strcmp)) {
		return false;
	}

	// SCALING
	if (a->scaling != b->scaling) {
		return false;
	}

	// AUTO_SCALE
	if (a->auto_scale != b->auto_scale) {
		return false;
	}
	if (a->auto_scale_min != b->auto_scale_min) {
		return false;
	}
	if (a->auto_scale_max != b->auto_scale_max) {
		return false;
	}

	// SCALE
	if (!slist_equal(a->user_scales, b->user_scales, cfg_user_scale_equal)) {
		return false;
	}

	// MODE
	if (!slist_equal(a->user_modes, b->user_modes, cfg_user_mode_equal)) {
		return false;
	}

	// TRANSFORM
	if (!slist_equal(a->user_transforms, b->user_transforms, cfg_user_transform_equal)) {
		return false;
	}

	// VRR_OFF
	if (!slist_equal(a->adaptive_sync_off_name_desc, b->adaptive_sync_off_name_desc, fn_comp_equals_strcmp)) {
		return false;
	}

	// CALLBACK_CMD
	char *ao = a->callback_cmd;
	char *bo = b->callback_cmd;
	if ((ao && !bo) || (!ao && bo) || (ao && bo && strcmp(ao, bo) != 0)) {
		return false;
	}

	// LAPTOP_DISPLAY_PREFIX
	char *al = a->laptop_display_prefix;
	char *bl = b->laptop_display_prefix;
	if ((al && !bl) || (!al && bl) || (al && bl && strcmp(al, bl) != 0)) {
		return false;
	}

	// MAX_PREFERRED_REFRESH
	if (!slist_equal(a->max_preferred_refresh_name_desc, b->max_preferred_refresh_name_desc, fn_comp_equals_strcmp)) {
		return false;
	}

	// DISABLED
	if (!slist_equal(a->disabled, b->disabled, cfg_disabled_equal)) {
		return false;
	}

	// LOG_THRESHOLD
	if (a->log_threshold != b->log_threshold) {
		return false;
	}

	return true;
}

//
// init functions
//
struct Cfg *cfg_init(void) {
	struct Cfg *cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	return cfg;
}

struct Cfg *cfg_default(void) {
	struct Cfg *def = cfg_init();

	def->arrange = ARRANGE_DEFAULT;
	def->align = ALIGN_DEFAULT;
	def->scaling = SCALING_DEFAULT;
	def->auto_scale = AUTO_SCALE_DEFAULT;
	def->auto_scale_min = AUTO_SCALE_MIN_DEFAULT;
	def->auto_scale_max = AUTO_SCALE_MAX_DEFAULT;
	def->callback_cmd = strdup(CALLBACK_CMD_DEFAULT);

	return def;
}

struct UserMode *cfg_user_mode_default(void) {
	return cfg_user_mode_init(NULL, false, -1, -1, -1, false);
}

struct UserMode *cfg_user_mode_init(const char *name_desc, const bool max, const int32_t width, const int32_t height, const int32_t refresh_mhz, const bool warned_no_mode) {
	struct UserMode *um = (struct UserMode*)calloc(1, sizeof(struct UserMode));

	if (name_desc) {
		um->name_desc = strdup(name_desc);
	}
	um->max = max;
	um->width = width;
	um->height = height;
	um->refresh_mhz = refresh_mhz;
	um->warned_no_mode = warned_no_mode;

	return um;
}

struct UserScale *cfg_user_scale_init(const char *name_desc, const float scale) {
	struct UserScale *us = calloc(1, sizeof(struct UserScale));

	us->name_desc = strdup(name_desc);
	us->scale = scale;

	return us;
}

struct UserTransform *cfg_user_transform_init(const char *name_desc, const enum wl_output_transform transform) {
	struct UserTransform *ut = calloc(1, sizeof(struct UserTransform));

	ut->name_desc = strdup(name_desc);
	ut->transform = transform;

	return ut;
}

struct Disabled *cfg_disabled_always(const char *name_desc) {
	struct Disabled *d = calloc(1, sizeof(struct Disabled));

	d->name_desc = strdup(name_desc);
	d->conditions = NULL;

	return d;
}

static void set_paths(struct Cfg *cfg, char *resolved_from, const char *file_path) {
	static char path[PATH_MAX];

	cfg->resolved_from = resolved_from;

	cfg->file_path = strdup(file_path);

	// dirname modifies path
	strcpy(path, cfg->file_path);
	free(cfg->dir_path);
	cfg->dir_path = strdup(dirname(path));

	// basename modifies path
	strcpy(path, cfg->file_path);
	free(cfg->file_name);
	cfg->file_name = strdup(basename(path));
}

bool resolve_cfg_file(struct Cfg *cfg) {
	if (!cfg)
		return false;

	cfg_paths_free(cfg);

	for (struct SList *i = cfg_file_paths; i; i = i->nex) {
		if (access(i->val, R_OK) == 0) {

			char *file_path = realpath(i->val, NULL);

			if (!file_path) {
				continue;
			}
			if (access(file_path, R_OK) != 0) {
				free(file_path);
				continue;
			}

			set_paths(cfg, i->val, file_path);

			free(file_path);

			return true;
		}
	}

	return false;
}

void validate_fix(struct Cfg *cfg) {
	if (!cfg) {
		return;
	}
	enum Align align = cfg->align;
	enum Arrange arrange = cfg->arrange;
	switch(arrange) {
		case COL:
			if (align != LEFT && align != MIDDLE && align != RIGHT) {
				log_warn("\nIgnoring invalid ALIGN %s for %s arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.", align_name(align), arrange_name(arrange));
				cfg->align = LEFT;
			}
			break;
		case ROW:
		default:
			if (align != TOP && align != MIDDLE && align != BOTTOM) {
				log_warn("\nIgnoring invalid ALIGN %s for %s arrange. Valid values are TOP, MIDDLE and BOTTOM. Using default TOP.", align_name(align), arrange_name(arrange));
				cfg->align = TOP;
			}
			break;
	}

	slist_remove_all_free(&cfg->user_scales, invalid_user_scale, NULL, cfg_user_scale_free);

	slist_remove_all_free(&cfg->user_modes, invalid_user_mode, NULL, cfg_user_mode_free);
}

void validate_warn(struct Cfg *cfg) {
	if (!cfg)
		return;

	struct SList *i = NULL;

	for (i = cfg->user_scales; i; i = i->nex) {
		if (!i->val)
			continue;
		struct UserScale *user_scale = (struct UserScale*)i->val;
		warn_short_name_desc(user_scale->name_desc, "SCALE");
	}
	for (i = cfg->user_modes; i; i = i->nex) {
		if (!i->val)
			continue;
		struct UserMode *user_mode = (struct UserMode*)i->val;
		warn_short_name_desc(user_mode->name_desc, "MODE");
	}
	for (i = cfg->user_transforms; i; i = i->nex) {
		if (!i->val)
			continue;
		struct UserTransform *user_transform = (struct UserTransform*)i->val;
		warn_short_name_desc(user_transform->name_desc, "TRANSFORM");
	}
	for (i = cfg->order_name_desc; i; i = i->nex) {
		if (!i->val)
			continue;
		warn_short_name_desc((const char*)i->val, "ORDER");
	}
	for (i = cfg->adaptive_sync_off_name_desc; i; i = i->nex) {
		if (!i->val)
			continue;
		warn_short_name_desc((const char*)i->val, "VRR_OFF");
	}
	for (i = cfg->max_preferred_refresh_name_desc; i; i = i->nex) {
		if (!i->val)
			continue;
		warn_short_name_desc((const char*)i->val, "MAX_PREFERRED_REFRESH");
	}
	for (i = cfg->disabled; i; i = i->nex) {
		if (!i->val)
			continue;
		struct Disabled *disabled = (struct Disabled*)i->val;
		warn_short_name_desc((const char*)disabled->name_desc, "DISABLED");
	}
}

struct Cfg *merge_set(struct Cfg *to, struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = clone_cfg(to);

	struct SList *i;

	// ARRANGE
	if (from->arrange) {
		merged->arrange = from->arrange;
	}

	// ALIGN
	if (from->align) {
		merged->align = from->align;
	}

	// ORDER, replace
	if (from->order_name_desc) {
		slist_free_vals(&merged->order_name_desc, NULL);
		merged->order_name_desc = slist_clone(from->order_name_desc, fn_clone_strdup);
	}

	// SCALING
	if (from->scaling) {
		merged->scaling = from->scaling;
	}

	// AUTO_SCALE
	if (from->auto_scale) {
		merged->auto_scale = from->auto_scale;
		merged->auto_scale_min = from->auto_scale_min;
		merged->auto_scale_max = from->auto_scale_max;
	}

	// SCALE
	struct UserScale *set_user_scale = NULL;
	struct UserScale *merged_user_scale = NULL;
	for (i = from->user_scales; i; i = i->nex) {
		set_user_scale = (struct UserScale*)i->val;
		if (!(merged_user_scale = (struct UserScale*)slist_find_equal_val(merged->user_scales, cfg_user_scale_name_equal, set_user_scale))) {
			merged_user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));
			merged_user_scale->name_desc = strdup(set_user_scale->name_desc);
			slist_append(&merged->user_scales, merged_user_scale);
		}
		merged_user_scale->scale = set_user_scale->scale;
	}

	// MODE
	struct UserMode *set_user_mode = NULL;
	struct UserMode *merged_user_mode = NULL;
	for (i = from->user_modes; i; i = i->nex) {
		set_user_mode = (struct UserMode*)i->val;
		if (!(merged_user_mode = (struct UserMode*)slist_find_equal_val(merged->user_modes, cfg_user_mode_name_equal, set_user_mode))) {
			merged_user_mode = cfg_user_mode_default();
			merged_user_mode->name_desc = strdup(set_user_mode->name_desc);
			slist_append(&merged->user_modes, merged_user_mode);
		}
		merged_user_mode->max = set_user_mode->max;
		merged_user_mode->width = set_user_mode->width;
		merged_user_mode->height = set_user_mode->height;
		merged_user_mode->refresh_mhz = set_user_mode->refresh_mhz;
		merged_user_mode->warned_no_mode = set_user_mode->warned_no_mode;
	}

	// TRANSFORM
	struct UserTransform *set_user_transform = NULL;
	struct UserTransform *merged_user_transform = NULL;
	for (i = from->user_transforms; i; i = i->nex) {
		set_user_transform = (struct UserTransform*)i->val;
		if (!(merged_user_transform = (struct UserTransform*)slist_find_equal_val(merged->user_transforms, cfg_user_transform_name_equal, set_user_transform))) {
			merged_user_transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));
			merged_user_transform->name_desc = strdup(set_user_transform->name_desc);
			slist_append(&merged->user_transforms, merged_user_transform);
		}
		merged_user_transform->transform = set_user_transform->transform;
	}

	// VRR_OFF
	for (i = from->adaptive_sync_off_name_desc; i; i = i->nex) {
		if (!slist_find_equal(merged->adaptive_sync_off_name_desc, fn_comp_equals_strcmp, i->val)) {
			slist_append(&merged->adaptive_sync_off_name_desc, strdup((char*)i->val));
		}
	}

	// DISABLED
	for (i = from->disabled; i; i = i->nex) {
		if (!slist_find_equal(merged->disabled, cfg_disabled_equal, i->val)) {
			slist_append(&merged->disabled, cfg_disabled_clone(i->val));
		}
	}

	// CALLBACK_CMD
	if (from->callback_cmd) {
		if (merged->callback_cmd) {
			free(merged->callback_cmd);
		}
		merged->callback_cmd = strdup(from->callback_cmd);
	}

	return merged;
}

struct Cfg *merge_del(struct Cfg *to, struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = clone_cfg(to);

	struct SList *i;

	// SCALE
	for (i = from->user_scales; i; i = i->nex) {
		slist_remove_all_free(&merged->user_scales, cfg_user_scale_name_equal, i->val, cfg_user_scale_free);
	}

	// MODE
	for (i = from->user_modes; i; i = i->nex) {
		slist_remove_all_free(&merged->user_modes, cfg_user_mode_name_equal, i->val, cfg_user_mode_free);
	}

	// TRANSFORM
	for (i = from->user_transforms; i; i = i->nex) {
		slist_remove_all_free(&merged->user_transforms, cfg_user_transform_name_equal, i->val, cfg_user_transform_free);
	}

	// VRR_OFF
	for (i = from->adaptive_sync_off_name_desc; i; i = i->nex) {
		slist_remove_all_free(&merged->adaptive_sync_off_name_desc, fn_comp_equals_strcmp, i->val, NULL);
	}

	// DISABLED
	for (i = from->disabled; i; i = i->nex) {
		slist_remove_all_free(&merged->disabled, cfg_disabled_equal, i->val, cfg_disabled_free);
	}

	// CALLBACK_CMD
	if (from->callback_cmd && strlen(from->callback_cmd) == 0) {
		free(merged->callback_cmd);
		merged->callback_cmd = NULL;
	}

	return merged;
}

struct Cfg *merge_toggle(struct Cfg *to, struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = clone_cfg(to);

	// SCALE
	if (from->scaling == ON) {
		merged->scaling = on_off_invert(merged->scaling);
	}

	// AUTO_SCALE
	if (from->auto_scale == ON) {
		merged->auto_scale = on_off_invert(merged->auto_scale);
	}

	// VRR_OFF
	slist_xor_free(&merged->adaptive_sync_off_name_desc, from->adaptive_sync_off_name_desc, fn_comp_equals_strcmp, NULL, fn_clone_strdup);

	return merged;
}

struct Cfg *cfg_merge(struct Cfg *to, struct Cfg *from, enum IpcCommand command) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = NULL;

	if (command == CFG_DEL) {
		merged = merge_del(to, from);
	} else if (command == CFG_TOGGLE) {
		merged = merge_toggle(to, from);
	} else {
		merged = merge_set(to, from);
	}

	if (merged) {
		validate_fix(merged);
		validate_warn(merged);

		if (cfg_equal(merged, to)) {
			cfg_free(merged);
			merged = NULL;
		}
	}

	return merged;
}

void cfg_file_paths_init(const char *user_path) {
	char path[PATH_MAX];

	// maybe user
	if (user_path && access(user_path, R_OK) == 0) {
		slist_append(&cfg_file_paths, strdup(user_path));
	}

	if (getenv("XDG_CONFIG_HOME") != NULL) {
		// maybe XDG_CONFIG_HOME
		snprintf(path, PATH_MAX, "%s/way-displays/cfg.yaml", getenv("XDG_CONFIG_HOME"));
		slist_append(&cfg_file_paths, strdup(path));
	} else if (getenv("HOME") != NULL) {
		// ~/.config
		snprintf(path, PATH_MAX, "%s/.config/way-displays/cfg.yaml", getenv("HOME"));
		slist_append(&cfg_file_paths, strdup(path));
	}

	// etc
	slist_append(&cfg_file_paths, strdup("/usr/local/etc/way-displays/cfg.yaml"));
	slist_append(&cfg_file_paths, strdup(ROOT_ETC"/way-displays/cfg.yaml"));
}

void cfg_init_path(const char *user_path) {
	cfg = cfg_default();

	bool found = resolve_cfg_file(cfg);

	if (found) {
		log_info("\nFound configuration file: %s", cfg->file_path);
		if (!unmarshal_cfg_from_file(cfg)) {
			log_info("\nUsing default configuration:");
			struct Cfg *def = cfg_default();
			def->dir_path = cfg->dir_path ? strdup(cfg->dir_path) : NULL;
			def->file_path = cfg->file_path ? strdup(cfg->file_path) : NULL;
			def->file_name = cfg->file_name ? strdup(cfg->file_name) : NULL;
			cfg_free(cfg);
			cfg = def;
		}
	} else {
		log_info("\nNo configuration file found, using defaults:");
	}
	validate_fix(cfg);
	log_info("\nActive configuration:");
	print_cfg(INFO, cfg, false);
	validate_warn(cfg);
}

void cfg_file_reload(void) {
	if (!cfg->file_path)
		return;

	struct Cfg *reloaded = cfg_default();
	reloaded->dir_path = cfg->dir_path ? strdup(cfg->dir_path) : NULL;
	reloaded->file_path = cfg->file_path ? strdup(cfg->file_path) : NULL;
	reloaded->file_name = cfg->file_name ? strdup(cfg->file_name) : NULL;

	log_info("\nReloading configuration file: %s", cfg->file_path);
	if (unmarshal_cfg_from_file(reloaded)) {
		cfg_free(cfg);
		cfg = reloaded;
		log_set_threshold(cfg->log_threshold, false);
		validate_fix(cfg);
		log_info("\nNew configuration:");
		print_cfg(INFO, cfg, false);
		validate_warn(cfg);
	} else {
		log_info("\nConfiguration unchanged:");
		print_cfg(INFO, cfg, false);
		cfg_free(reloaded);
	}
}

void cfg_file_write(void) {
	char *yaml = NULL;
	char *resolved_from = cfg->resolved_from;
	bool written = false;

	cfg->updated = false;

	if (!(yaml = marshal_cfg(cfg))) {
		goto end;
	}

	if (cfg->file_path && (written = file_write(cfg->file_path, yaml, "w"))) {
		cfg->updated = true;
		goto end;
	}

	if (!written) {

		// kill that cfg file
		cfg_paths_free(cfg);
		fd_wd_cfg_dir_destroy();

		// write preferred alternatives
		for (struct SList *i = cfg_file_paths; i; i = i->nex) {

			// skip previously resolved
			if (resolved_from == i->val) {
				continue;
			}

			set_paths(cfg, i->val, i->val);

			// attempt to write
			if (mkdir_p(cfg->dir_path, 0755) && (written = file_write(i->val, yaml, "w"))) {

				// watch the new
				fd_wd_cfg_dir_create();
				goto end;
			}

			cfg_paths_free(cfg);
		}
	}

end:
	free(yaml);

	if (written) {
		log_info("\nWrote configuration file: %s", cfg->file_path);
	}
}

void cfg_destroy(void) {
	cfg_free(cfg);
	cfg = NULL;
}

void cfg_file_paths_destroy(void) {
	slist_free_vals(&cfg_file_paths, NULL);
}

//
// freeing functions
//
void cfg_free(struct Cfg *cfg) {
	if (!cfg)
		return;

	cfg_paths_free(cfg);

	free(cfg->callback_cmd);

	free(cfg->laptop_display_prefix);

	slist_free_vals(&cfg->order_name_desc, NULL);

	slist_free_vals(&cfg->user_scales, cfg_user_scale_free);

	slist_free_vals(&cfg->user_modes, cfg_user_mode_free);

	slist_free_vals(&cfg->adaptive_sync_off_name_desc, NULL);

	slist_free_vals(&cfg->max_preferred_refresh_name_desc, NULL);

	slist_free_vals(&cfg->disabled, cfg_disabled_free);

	slist_free_vals(&cfg->user_transforms, cfg_user_transform_free);

	free(cfg);
}

void cfg_user_scale_free(const void *val) {
	struct UserScale *user_scale = (struct UserScale*)val;

	if (!user_scale)
		return;

	free(user_scale->name_desc);

	free(user_scale);
}

void cfg_user_mode_free(const void *val) {
	struct UserMode *user_mode = (struct UserMode*)val;

	if (!user_mode)
		return;

	free(user_mode->name_desc);

	free(user_mode);
}

void cfg_user_transform_free(const void *val) {
	struct UserTransform *user_transform = (struct UserTransform*)val;

	if (!user_transform)
		return;

	free(user_transform->name_desc);

	free(user_transform);
}

void cfg_disabled_free(const void *val) {
	struct Disabled *disabled = (struct Disabled*)val;

	if (!disabled)
		 return;

	free(disabled->name_desc);

	slist_free_vals(&disabled->conditions, condition_free);
}

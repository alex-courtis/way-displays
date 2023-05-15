#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cfg.h"

#include "fs.h"
#include "convert.h"
#include "global.h"
#include "info.h"
#include "list.h"
#include "log.h"
#include "marshalling.h"

bool cfg_equal_user_mode_name(const void *value, const void *data) {
	if (!value || !data) {
		return false;
	}

	struct UserMode *lhs = (struct UserMode*)value;
	struct UserMode *rhs = (struct UserMode*)data;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcmp(lhs->name_desc, rhs->name_desc) == 0;
}

bool cfg_equal_user_scale_name(const void *value, const void *data) {
	if (!value || !data) {
		return false;
	}

	struct UserScale *lhs = (struct UserScale*)value;
	struct UserScale *rhs = (struct UserScale*)data;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcmp(lhs->name_desc, rhs->name_desc) == 0;
}

bool cfg_equal_user_scale(const void *value, const void *data) {
	if (!value || !data) {
		return false;
	}

	struct UserScale *lhs = (struct UserScale*)value;
	struct UserScale *rhs = (struct UserScale*)data;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcmp(lhs->name_desc, rhs->name_desc) == 0 && lhs->scale == rhs->scale;
}

bool cfg_equal_user_mode(const void *value, const void *data) {
	if (!value || !data) {
		return false;
	}

	struct UserMode *lhs = (struct UserMode*)value;
	struct UserMode *rhs = (struct UserMode*)data;

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

	if ((lhs->refresh_hz != -1 || rhs->refresh_hz != -1) && lhs->refresh_hz != rhs->refresh_hz) {
		return false;
	}

	return true;
}

bool invalid_user_scale(const void *value, const void *data) {
	if (!value) {
		return true;
	}

	struct UserScale *user_scale = (struct UserScale*)value;

	if (user_scale->scale <= 0) {
		log_warn("\nIgnoring non-positive SCALE %s %.3f", user_scale->name_desc, user_scale->scale);
		return true;
	}

	return false;
}

bool invalid_user_mode(const void *value, const void *data) {
	if (!value) {
		return true;
	}
	struct UserMode *user_mode = (struct UserMode*)value;

	if (user_mode->width != -1 && user_mode->width <= 0) {
		log_warn("\nIgnoring non-positive MODE %s WIDTH %d", user_mode->name_desc, user_mode->width);
		return true;
	}
	if (user_mode->height != -1 && user_mode->height <= 0) {
		log_warn("\nIgnoring non-positive MODE %s HEIGHT %d", user_mode->name_desc, user_mode->height);
		return true;
	}
	if (user_mode->refresh_hz != -1 && user_mode->refresh_hz <= 0) {
		log_warn("\nIgnoring non-positive MODE %s HZ %d", user_mode->name_desc, user_mode->refresh_hz);
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

void warn_short_name_desc(const char *name_desc, const char *element) {
	if (!name_desc)
		return;

	if (strlen(name_desc) < 4) {
		log_warn("\n%s '%s' is less than 4 characters, which may result in some unwanted matches.", element, name_desc);
	}
}

struct Cfg *clone_cfg(struct Cfg *from) {
	if (!from) {
		return NULL;
	}

	struct SList *i;
	struct Cfg *to = (struct Cfg*)calloc(1, sizeof(struct Cfg));

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
	for (i = from->order_name_desc; i; i = i->nex) {
		slist_append(&to->order_name_desc, strdup((char*)i->val));
	}

	// SCALING
	if (from->scaling) {
		to->scaling = from->scaling;
	}

	// AUTO_SCALE
	if (from->auto_scale) {
		to->auto_scale = from->auto_scale;
	}

	// SCALE
	for (i = from->user_scales; i; i = i->nex) {
		struct UserScale *from_scale = (struct UserScale*)i->val;
		struct UserScale *to_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));
		to_scale->name_desc = strdup(from_scale->name_desc);
		to_scale->scale = from_scale->scale;
		slist_append(&to->user_scales, to_scale);
	}

	// MODE
	for (i = from->user_modes; i; i = i->nex) {
		struct UserMode *from_user_mode = (struct UserMode*)i->val;
		struct UserMode *to_user_mode = (struct UserMode*)calloc(1, sizeof(struct UserMode));
		to_user_mode->name_desc = strdup(from_user_mode->name_desc);
		to_user_mode->max = from_user_mode->max;
		to_user_mode->width = from_user_mode->width;
		to_user_mode->height = from_user_mode->height;
		to_user_mode->refresh_hz = from_user_mode->refresh_hz;
		to_user_mode->warned_no_mode = from_user_mode->warned_no_mode;
		slist_append(&to->user_modes, to_user_mode);
	}

	// VRR_OFF
	for (i = from->adaptive_sync_off_name_desc; i; i = i->nex) {
		slist_append(&to->adaptive_sync_off_name_desc, strdup((char*)i->val));
	}

	// LAPTOP_DISPLAY_PREFIX
	if (from->laptop_display_prefix) {
		to->laptop_display_prefix = strdup(from->laptop_display_prefix);
	}

	// MAX_PREFERRED_REFRESH
	for (i = from->max_preferred_refresh_name_desc; i; i = i->nex) {
		slist_append(&to->max_preferred_refresh_name_desc, strdup((char*)i->val));
	}

	// DISABLED
	for (i = from->disabled_name_desc; i; i = i->nex) {
		slist_append(&to->disabled_name_desc, strdup((char*)i->val));
	}

	// LOG_THRESHOLD
	if (from->log_threshold) {
		to->log_threshold = from->log_threshold;
	}

	return to;
}

bool cfg_equal(struct Cfg *a, struct Cfg *b) {
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
	if (!slist_equal(a->order_name_desc, b->order_name_desc, slist_equal_strcmp)) {
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

	// SCALE
	if (!slist_equal(a->user_scales, b->user_scales, cfg_equal_user_scale)) {
		return false;
	}

	// MODE
	if (!slist_equal(a->user_modes, b->user_modes, cfg_equal_user_mode)) {
		return false;
	}

	// VRR_OFF
	if (!slist_equal(a->adaptive_sync_off_name_desc, b->adaptive_sync_off_name_desc, slist_equal_strcmp)) {
		return false;
	}

	// LAPTOP_DISPLAY_PREFIX
	char *al = a->laptop_display_prefix;
	char *bl = b->laptop_display_prefix;
	if ((al && !bl) || (!al && bl) || (al && bl && strcmp(al, bl) != 0)) {
		return false;
	}

	// MAX_PREFERRED_REFRESH
	if (!slist_equal(a->max_preferred_refresh_name_desc, b->max_preferred_refresh_name_desc, slist_equal_strcmp)) {
		return false;
	}

	// DISABLED
	if (!slist_equal(a->disabled_name_desc, b->disabled_name_desc, slist_equal_strcmp)) {
		return false;
	}

	// LOG_THRESHOLD
	if (a->log_threshold != b->log_threshold) {
		return false;
	}

	return true;
}

struct Cfg *cfg_default(void) {
	struct Cfg *def = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	def->arrange = ARRANGE_DEFAULT;
	def->align = ALIGN_DEFAULT;
	def->scaling = SCALING_DEFAULT;
	def->auto_scale = AUTO_SCALE_DEFAULT;

	return def;
}

struct UserMode *cfg_user_mode_default(void) {
	return cfg_user_mode_init(NULL, false, -1, -1, -1, false);
}

struct UserMode *cfg_user_mode_init(const char *name_desc, const bool max, const int32_t width, const int32_t height, const int32_t refresh_hz, const bool warned_no_mode) {
	struct UserMode *um = (struct UserMode*)calloc(1, sizeof(struct UserMode));

	if (name_desc) {
		um->name_desc = strdup(name_desc);
	}
	um->max = max;
	um->width = width;
	um->height = height;
	um->refresh_hz = refresh_hz;
	um->warned_no_mode = warned_no_mode;

	return um;
}

struct UserScale *cfg_user_scale_init(const char *name_desc, const float scale) {
	struct UserScale *us = calloc(1, sizeof(struct UserScale));

	us->name_desc = strdup(name_desc);
	us->scale = scale;

	return us;
}

void set_file_dir(struct Cfg *cfg) {
	static char path[PATH_MAX];

	strcpy(path, cfg->file_path);
	free(cfg->dir_path);
	cfg->dir_path = strdup(dirname(path));

	strcpy(path, cfg->file_path);
	free(cfg->file_name);
	cfg->file_name = strdup(basename(path));
}

bool resolve_cfg_file(struct Cfg *cfg) {
	if (!cfg)
		return false;

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

			free(cfg->file_path);
			cfg->file_path = file_path;

			set_file_dir(cfg);

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
	for (i = cfg->disabled_name_desc; i; i = i->nex) {
		if (!i->val)
			continue;
		warn_short_name_desc((const char*)i->val, "DISABLED");
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
		for (i = from->order_name_desc; i; i = i->nex) {
			slist_append(&merged->order_name_desc, strdup((char*)i->val));
		}
	}

	// SCALING
	if (from->scaling) {
		merged->scaling = from->scaling;
	}

	// AUTO_SCALE
	if (from->auto_scale) {
		merged->auto_scale = from->auto_scale;
	}

	// SCALE
	struct UserScale *set_user_scale = NULL;
	struct UserScale *merged_user_scale = NULL;
	for (i = from->user_scales; i; i = i->nex) {
		set_user_scale = (struct UserScale*)i->val;
		if (!(merged_user_scale = (struct UserScale*)slist_find_equal_val(merged->user_scales, cfg_equal_user_scale_name, set_user_scale))) {
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
		if (!(merged_user_mode = (struct UserMode*)slist_find_equal_val(merged->user_modes, cfg_equal_user_mode_name, set_user_mode))) {
			merged_user_mode = cfg_user_mode_default();
			merged_user_mode->name_desc = strdup(set_user_mode->name_desc);
			slist_append(&merged->user_modes, merged_user_mode);
		}
		merged_user_mode->max = set_user_mode->max;
		merged_user_mode->width = set_user_mode->width;
		merged_user_mode->height = set_user_mode->height;
		merged_user_mode->refresh_hz = set_user_mode->refresh_hz;
		merged_user_mode->warned_no_mode = set_user_mode->warned_no_mode;
	}

	// VRR_OFF
	for (i = from->adaptive_sync_off_name_desc; i; i = i->nex) {
		if (!slist_find_equal(merged->adaptive_sync_off_name_desc, slist_equal_strcmp, i->val)) {
			slist_append(&merged->adaptive_sync_off_name_desc, strdup((char*)i->val));
		}
	}

	// DISABLED
	for (i = from->disabled_name_desc; i; i = i->nex) {
		if (!slist_find_equal(merged->disabled_name_desc, slist_equal_strcmp, i->val)) {
			slist_append(&merged->disabled_name_desc, strdup((char*)i->val));
		}
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
		slist_remove_all_free(&merged->user_scales, cfg_equal_user_scale_name, i->val, cfg_user_scale_free);
	}

	// MODE
	for (i = from->user_modes; i; i = i->nex) {
		slist_remove_all_free(&merged->user_modes, cfg_equal_user_mode_name, i->val, cfg_user_mode_free);
	}

	// VRR_OFF
	for (i = from->adaptive_sync_off_name_desc; i; i = i->nex) {
		slist_remove_all_free(&merged->adaptive_sync_off_name_desc, slist_equal_strcmp, i->val, NULL);
	}

	// DISABLED
	for (i = from->disabled_name_desc; i; i = i->nex) {
		slist_remove_all_free(&merged->disabled_name_desc, slist_equal_strcmp, i->val, NULL);
	}

	return merged;
}

struct Cfg *cfg_merge(struct Cfg *to, struct Cfg *from, bool del) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = NULL;

	if (del) {
		merged = merge_del(to, from);
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

	// maybe XDG_CONFIG_HOME
	if (getenv("XDG_CONFIG_HOME") != NULL) {
		snprintf(path, PATH_MAX, "%s/way-displays/cfg.yaml", getenv("XDG_CONFIG_HOME"));
		slist_append(&cfg_file_paths, strdup(path));
	}

	// ~/.config
	snprintf(path, PATH_MAX, "%s/.config/way-displays/cfg.yaml", getenv("HOME"));
	slist_append(&cfg_file_paths, strdup(path));

	// etc
	slist_append(&cfg_file_paths, strdup("/usr/local/etc/way-displays/cfg.yaml"));
	slist_append(&cfg_file_paths, strdup(ROOT_ETC"/way-displays/cfg.yaml"));
}

void cfg_init(const char *user_path) {
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

	// TODO write new
	if (!cfg->file_path) {
		log_error("\nMissing file path");
		goto end;
	}

	yaml = marshal_cfg(cfg);
	if (!yaml) {
		goto end;
	}

	cfg->written = file_write(cfg->file_path, yaml);

	if (!cfg->written) {
		// TODO destroy inotify

		// write preferred alternatives
		for (struct SList *i = cfg_file_paths; i; i = i->nex) {
			if (strcmp(i->val, cfg->file_path) == 0) {
				continue;
			}
			if ((cfg->written = file_write(i->val, yaml))) {
				cfg->file_path = strdup(i->val);
				set_file_dir(cfg);
				break;
			}
		}

		if (cfg->written) {
			// TODO create inotify
		} else {
			free(cfg->file_path);
			cfg->file_path = NULL;
			free(cfg->dir_path);
			cfg->dir_path = NULL;
			free(cfg->file_name);
			cfg->file_name = NULL;
		}
	}

	if (cfg->written) {
		log_info("\nWrote configuration file: %s", cfg->file_path);
	}

end:
	free(yaml);
}

void cfg_destroy(void) {
	cfg_free(cfg);
	cfg = NULL;
}

void cfg_free(struct Cfg *cfg) {
	if (!cfg)
		return;

	free(cfg->dir_path);
	free(cfg->file_path);
	free(cfg->file_name);
	free(cfg->laptop_display_prefix);

	slist_free_vals(&cfg->order_name_desc, NULL);

	slist_free_vals(&cfg->user_scales, cfg_user_scale_free);

	slist_free_vals(&cfg->user_modes, cfg_user_mode_free);

	slist_free_vals(&cfg->adaptive_sync_off_name_desc, NULL);

	slist_free_vals(&cfg->max_preferred_refresh_name_desc, NULL);

	slist_free_vals(&cfg->disabled_name_desc, NULL);

	free(cfg);
}

void cfg_user_scale_free(void *data) {
	struct UserScale *user_scale = (struct UserScale*)data;

	if (!user_scale)
		return;

	free(user_scale->name_desc);

	free(user_scale);
}

void cfg_user_mode_free(void *data) {
	struct UserMode *user_mode = (struct UserMode*)data;

	if (!user_mode)
		return;

	free(user_mode->name_desc);

	free(user_mode);
}

void cfg_file_paths_destroy(void) {
	slist_free_vals(&cfg_file_paths, NULL);
}


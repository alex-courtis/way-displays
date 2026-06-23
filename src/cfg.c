#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client-protocol.h>

#include "cfg.h"

#include "cfg/disabled.h"
#include "cfg/user-mode.h"
#include "conditions.h"
#include "convert.h"
#include "fds.h"
#include "fn.h"
#include "fs.h"
#include "ipc.h"
#include "log.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "sset.h"
#include "yaml/marshal-types.h"
#include "yaml/marshal.h"

struct Cfg *g_cfg = NULL;

// one-shot singleton set via cfg_file_paths_init
struct SList *cfg_file_paths = NULL;

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
static void* fn_clone_cfg_user_transform(const void* const val) {
	const struct UserTransform *original = (struct UserTransform*)val;
	struct UserTransform *clone = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

	*clone = *original;
	clone->name_desc = strdup(original->name_desc);

	return clone;
}

static void* fn_clone_cfg_user_scale(const void* const val) {
	const struct UserScale *original = (struct UserScale*)val;
	struct UserScale *clone = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	*clone = *original;
	clone->name_desc = strdup(original->name_desc);

	return clone;
}

//
// equality functions
//
static bool cfg_user_scale_name_equal(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	const struct UserScale *lhs = (struct UserScale*)a;
	const struct UserScale *rhs = (struct UserScale*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcmp(lhs->name_desc, rhs->name_desc) == 0;
}

static bool fn_equal_cfg_user_scale(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	const struct UserScale *lhs = (struct UserScale*)a;
	const struct UserScale *rhs = (struct UserScale*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcmp(lhs->name_desc, rhs->name_desc) == 0 && lhs->scale == rhs->scale;
}

static bool fn_equal_cfg_user_transform_name(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	const struct UserTransform *lhs = (struct UserTransform*)a;
	const struct UserTransform *rhs = (struct UserTransform*)b;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcmp(lhs->name_desc, rhs->name_desc) == 0;
}

static bool fn_equal_cfg_user_transform(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	const struct UserTransform *lhs = (struct UserTransform*)a;
	const struct UserTransform *rhs = (struct UserTransform*)b;

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

static bool invalid_user_scale(const void *a, const void *b) {
	if (!a) {
		return true;
	}

	struct UserScale *user_scale = (struct UserScale*)a;

	if (user_scale->scale <= 0) {
		log_warn(NULL);
		log_warn("Ignoring non-positive SCALE %s %.3f", user_scale->name_desc, user_scale->scale);
		return true;
	}

	return false;
}

static void warn_ambiguous_name_desc(const char *name_desc, const char *element) {
	if (!name_desc)
		return;

	if (strlen(name_desc) < 4) {
		log_warn(NULL);
		log_warn("%s '%s' is less than 4 characters, which may result in some unwanted matches.", element, name_desc);
	}

	if (strcmp(name_desc, "DP-1") == 0) {
		log_warn(NULL);
		log_warn("%s '%s' will match eDP-1 and DP-1. Consider using regex '!^DP-1$' to exactly match.", element, name_desc);
	}
}

// TODO refactor into assignments into an empty struct
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
	to->auto_scale_dpi = from->auto_scale_dpi;
	to->auto_scale_min = from->auto_scale_min;
	to->auto_scale_max = from->auto_scale_max;

	// SCALE_ROUND_TO
	to->scale_round_to = from->scale_round_to;

	// SCALE_ROUND_STRATEGY
	to->scale_round_strategy = from->scale_round_strategy;

	// SCALE
	to->user_scales = slist_clone(from->user_scales, fn_clone_cfg_user_scale);

	// TRANSFORM
	to->user_transforms = slist_clone(from->user_transforms, fn_clone_cfg_user_transform);

	// MODE
	smap_free(to->user_modes);
	to->user_modes = smap_clone_deep(from->user_modes);

	// VRR_OFF
	sset_free(to->adaptive_sync_off);
	to->adaptive_sync_off = sset_clone(from->adaptive_sync_off);

	// CALLBACK_CMD
	if (from->callback_cmd) {
		to->callback_cmd = strdup(from->callback_cmd);
	}

	// LAPTOP_DISPLAY_PREFIX
	if (from->laptop_display_prefix) {
		to->laptop_display_prefix = strdup(from->laptop_display_prefix);
	}

	// LAPTOP_LID_MONITOR
	if (from->laptop_lid_monitor) {
		to->laptop_lid_monitor = from->laptop_lid_monitor;
	}

	// MAX_PREFERRED_REFRESH
	to->max_preferred_refresh_name_desc = slist_clone(from->max_preferred_refresh_name_desc, fn_clone_strdup);

	// DISABLED
	pset_free(to->disableds);
	to->disableds = pset_clone_deep(from->disableds);

	// LOG_THRESHOLD
	if (from->log_threshold) {
		to->log_threshold = from->log_threshold;
	}

	return to;
}

bool cfg_equal(const struct Cfg *a, const struct Cfg *b) {
	return a && b &&
		// TODO SSet this could be order insensitive
		sset_equal(a->adaptive_sync_off, b->adaptive_sync_off) &&
		a->align == b->align &&
		a->arrange == b->arrange &&
		a->auto_scale == b->auto_scale &&
		a->auto_scale_dpi == b->auto_scale_dpi &&
		a->auto_scale_max == b->auto_scale_max &&
		a->auto_scale_min == b->auto_scale_min &&
		fn_equal_strcmp(a->callback_cmd, b->callback_cmd) &&
		pset_equal(a->disableds, b->disableds) &&
		fn_equal_strcmp(a->laptop_display_prefix, b->laptop_display_prefix) &&
		a->laptop_lid_monitor == b->laptop_lid_monitor &&
		a->log_threshold == b->log_threshold &&
		slist_equal(a->max_preferred_refresh_name_desc, b->max_preferred_refresh_name_desc, (fn_equal)fn_equal_strcmp) &&
		slist_equal(a->order_name_desc, b->order_name_desc, (fn_equal)fn_equal_strcmp) &&
		a->scale_round_strategy == b->scale_round_strategy &&
		a->scale_round_to == b->scale_round_to &&
		a->scaling == b->scaling &&
		smap_equal(a->user_modes, b->user_modes) &&
		slist_equal(a->user_scales, b->user_scales, fn_equal_cfg_user_scale) &&
		slist_equal(a->user_transforms, b->user_transforms, fn_equal_cfg_user_transform);
}

//
// init functions
//
struct Cfg *cfg_init(void) {
	struct Cfg *cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	cfg->user_modes = user_mode_smap_init();
	cfg->adaptive_sync_off = sset_init();
	cfg->disableds = disabled_pset_init();

	return cfg;
}

struct Cfg *cfg_default(void) {
	struct Cfg *def = cfg_init();

	cfg_apply_defaults(def);

	return def;
}

void cfg_apply_defaults(struct Cfg *cfg) {

	if (!cfg->arrange)
		cfg->arrange = ARRANGE_DEFAULT;

	if (!cfg->align)
		cfg->align = ALIGN_DEFAULT;

	if (!cfg->scaling)
		cfg->scaling = SCALING_DEFAULT;

	if (!cfg->auto_scale)
		cfg->auto_scale = AUTO_SCALE_DEFAULT;

	if (!cfg->scale_round_to)
		cfg->scale_round_to = SCALE_ROUND_TO_DEFAULT;

	if (!cfg->scale_round_strategy)
		cfg->scale_round_strategy = SCALE_ROUND_STRATEGY_DEFAULT;

	if (!cfg->auto_scale_dpi)
		cfg->auto_scale_dpi = AUTO_SCALE_DPI_DEFAULT;

	if (!cfg->auto_scale_min)
		cfg->auto_scale_min = AUTO_SCALE_MIN_DEFAULT;

	if (!cfg->auto_scale_max)
		cfg->auto_scale_max = AUTO_SCALE_MAX_DEFAULT;

	if (!cfg->callback_cmd)
		cfg->callback_cmd = strdup(CALLBACK_CMD_DEFAULT);

	if (!cfg->laptop_lid_monitor)
		cfg->laptop_lid_monitor = LAPTOP_LID_MONITOR_DEFAULT;
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

static void set_paths(struct Cfg *cfg, char *resolved_from, const char *file_path) {
	static char path[PATH_MAX];

	cfg->resolved_from = resolved_from;

	cfg->file_path = strdup(file_path);

	// dirname modifies path
	strncpy(path, cfg->file_path, PATH_MAX - 1);
	free(cfg->dir_path);
	cfg->dir_path = strdup(dirname(path));

	// basename modifies path
	strncpy(path, cfg->file_path, PATH_MAX - 1);
	free(cfg->file_name);
	cfg->file_name = strdup(basename(path));
}

bool cfg_resolve_file_path(struct Cfg *dst) {
	if (!dst)
		return false;

	cfg_paths_free(dst);

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

			set_paths(dst, i->val, file_path);

			free(file_path);

			return true;
		}
	}

	return false;
}

void cfg_copy_file_path(struct Cfg *to, const struct Cfg *from) {
	if (!from || !to)
		return;

	free(to->dir_path);
	free(to->file_path);
	free(to->file_name);

	to->dir_path = from->dir_path ? strdup(from->dir_path) : NULL;
	to->file_path = from->file_path ? strdup(from->file_path) : NULL;
	to->file_name = from->file_name ? strdup(from->file_name) : NULL;
}

static void remove_duplicate_user_scales(struct Cfg *cfg) {
	const struct SMap *by_name_desc = smap_init();

	for (const struct SList *i = cfg->user_scales; i; i = i->nex) {
		const struct UserScale *user_scale = i->val;
		const struct UserScale *dup = smap_put(by_name_desc, user_scale->name_desc, user_scale);
		if (dup) {
			log_warn(NULL);
			log_warn("Removing duplicate SCALE %s", dup->name_desc);
			cfg_user_scale_free(dup);
		}
	}

	slist_free(&cfg->user_scales);
	cfg->user_scales = smap_vals_slist_shallow(by_name_desc);
	smap_free(by_name_desc);
}

static void remove_duplicate_user_transforms(struct Cfg *cfg) {
	const struct SMap *by_name_desc = smap_init();

	for (const struct SList *i = cfg->user_transforms; i; i = i->nex) {
		const struct UserTransform *transform = i->val;
		const struct UserTransform *dup = smap_put(by_name_desc, transform->name_desc, transform);
		if (dup) {
			log_warn(NULL);
			log_warn("Removing duplicate TRANSFORM %s", dup->name_desc);
			cfg_user_transform_free(dup);
		}
	}

	slist_free(&cfg->user_transforms);
	cfg->user_transforms = smap_vals_slist_shallow(by_name_desc);
	smap_free(by_name_desc);
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
				log_warn(NULL);
				log_warn("Ignoring invalid ALIGN %s for %s arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.", align_name(align), arrange_name(arrange));
				cfg->align = LEFT;
			}
			break;
		case ROW:
		default:
			if (align != TOP && align != MIDDLE && align != BOTTOM) {
				log_warn(NULL);
				log_warn("Ignoring invalid ALIGN %s for %s arrange. Valid values are TOP, MIDDLE and BOTTOM. Using default TOP.", align_name(align), arrange_name(arrange));
				cfg->align = TOP;
			}
			break;
	}

	if (cfg->auto_scale_dpi <= AUTO_SCALE_DPI_MIN) {
		log_warn(NULL);
		log_warn("Ignoring AUTO_SCALE_DPI %d < %d. Using default %d.", cfg->auto_scale_dpi, AUTO_SCALE_DPI_MIN, AUTO_SCALE_DPI_DEFAULT);
		cfg->auto_scale_dpi = AUTO_SCALE_DPI_DEFAULT;
	}

	slist_remove_all_free(&cfg->user_scales, invalid_user_scale, NULL, cfg_user_scale_free);
	remove_duplicate_user_scales(cfg);

	// TODO SMap remove_match, depends on whether we get many similar cases
	const char *name_desc;
	while ((name_desc = smap_match(cfg->user_modes, (fn_match_smap)user_mode_invalid, NULL).key)) {
		smap_remove_free(cfg->user_modes, name_desc);
	}

	remove_duplicate_user_transforms(cfg);
}

static void warn_ambiguous_name_desc_list(const struct SList *name_desc, const char * const element) {
	for (const struct SList *i = name_desc; i; i = i->nex) {
		warn_ambiguous_name_desc((const char*)i->val, element);
	}
}

static void warn_ambiguous_name_desc_sset(const struct SSet *name_descs, const char * const element) {
	for (const struct SSetIt *it = sset_it(name_descs); it; it = sset_it_next(it)) {
		warn_ambiguous_name_desc(it->val, element);
	}
}

void validate_warn(struct Cfg *cfg) {
	if (!cfg)
		return;

	for (struct SList *i = cfg->user_scales; i; i = i->nex) {
		if (!i->val)
			continue;
		const struct UserScale *user_scale = (struct UserScale*)i->val;
		warn_ambiguous_name_desc(user_scale->name_desc, "SCALE");
	}
	for (const struct SMapIt  *it = smap_it(cfg->user_modes); it; it = smap_it_next(it)) {
		warn_ambiguous_name_desc(it->key, "MODE");
	}
	for (struct SList *i = cfg->user_transforms; i; i = i->nex) {
		if (!i->val)
			continue;
		const struct UserTransform *user_transform = (struct UserTransform*)i->val;
		warn_ambiguous_name_desc(user_transform->name_desc, "TRANSFORM");
	}

	warn_ambiguous_name_desc_list(cfg->order_name_desc, "ORDER");
	warn_ambiguous_name_desc_sset(cfg->adaptive_sync_off, "VRR_OFF");
	warn_ambiguous_name_desc_list(cfg->max_preferred_refresh_name_desc, "MAX_PREFERRED_REFRESH");

	for (const struct PSetIt *it = pset_it(cfg->disableds); it; it = pset_it_next(it)) {
		struct Disabled *disabled = (struct Disabled*)it->val;
		warn_ambiguous_name_desc((const char*)disabled->name_desc, "DISABLED");

		for (struct SList *j = disabled->conditions; j; j = j->nex) {
			if (!j->val)
				continue;
			const struct Condition *condition = (struct Condition*)j->val;
			warn_ambiguous_name_desc_list(condition->plugged, "PLUGGED");
			warn_ambiguous_name_desc_list(condition->unplugged, "UNPLUGGED");
		}
	}
}

struct Cfg *merge_set(struct Cfg *to, const struct Cfg *from) {
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
		merged->auto_scale_dpi = from->auto_scale_dpi;
		merged->auto_scale_min = from->auto_scale_min;
		merged->auto_scale_max = from->auto_scale_max;
	}

	// SCALE_ROUND_TO
	if (from->scale_round_to) {
		merged->scale_round_to = from->scale_round_to;
	}

	// SCALE_ROUND_STRATEGY
	if (from->scale_round_strategy) {
		merged->scale_round_strategy = from->scale_round_strategy;
	}

	// SCALE
	const struct UserScale *set_user_scale = NULL;
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
	for (const struct SMapIt *it = smap_it(from->user_modes); it; it = smap_it_next(it)) {
		smap_put_free(merged->user_modes, it->key, user_mode_clone(it->val));
	}

	// TRANSFORM
	const struct UserTransform *set_user_transform = NULL;
	struct UserTransform *merged_user_transform = NULL;
	for (i = from->user_transforms; i; i = i->nex) {
		set_user_transform = (struct UserTransform*)i->val;
		if (!(merged_user_transform = (struct UserTransform*)slist_find_equal_val(merged->user_transforms, fn_equal_cfg_user_transform_name, set_user_transform))) {
			merged_user_transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));
			merged_user_transform->name_desc = strdup(set_user_transform->name_desc);
			slist_append(&merged->user_transforms, merged_user_transform);
		}
		merged_user_transform->transform = set_user_transform->transform;
	}

	// VRR_OFF
	for (const struct SSetIt *it = sset_it(from->adaptive_sync_off); it; it = sset_it_next(it)) {
		sset_add(merged->adaptive_sync_off, it->val);
	}

	// DISABLED
	for (const struct PSetIt *it = pset_it(from->disableds); it; it = pset_it_next(it)) {
		const struct Disabled *d = disabled_clone(it->val);
		if (!pset_add(merged->disableds, d)) {
			disabled_free(d);
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

struct Cfg *merge_del(struct Cfg *to, const struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = clone_cfg(to);

	const struct SList *i;

	// SCALE
	for (i = from->user_scales; i; i = i->nex) {
		slist_remove_all_free(&merged->user_scales, cfg_user_scale_name_equal, i->val, cfg_user_scale_free);
	}

	// MODE
	for (const struct SMapIt *it = smap_it(from->user_modes); it; it = smap_it_next(it)) {
		smap_remove_free(merged->user_modes, it->key);
	}

	// TRANSFORM
	for (i = from->user_transforms; i; i = i->nex) {
		slist_remove_all_free(&merged->user_transforms, fn_equal_cfg_user_transform_name, i->val, cfg_user_transform_free);
	}

	// TODO SSet remove_set
	// VRR_OFF
	for (const struct SSetIt *it = sset_it(from->adaptive_sync_off); it; it = sset_it_next(it)) {
		sset_remove(merged->adaptive_sync_off, it->val);
	}

	// TODO PSet remove_set
	// DISABLED
	for (const struct PSetIt *it = pset_it(from->disableds); it; it = pset_it_next(it)) {
		pset_remove_free(merged->disableds, it->val);
	}

	// CALLBACK_CMD
	if (from->callback_cmd && strlen(from->callback_cmd) == 0) {
		free(merged->callback_cmd);
		merged->callback_cmd = NULL;
	}

	return merged;
}

struct Cfg *merge_toggle(struct Cfg *to, const struct Cfg *from) {
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
	for (const struct SSetIt *it = sset_it(from->adaptive_sync_off); it; it = sset_it_next(it)) {
		if (!sset_remove(merged->adaptive_sync_off, it->val)) {
			sset_add(merged->adaptive_sync_off, it->val);
		}
	}

	return merged;
}

struct Cfg *cfg_merge(struct Cfg *to, const struct Cfg *from, const enum IpcCommand command) {
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
		snprintf(path, PATH_MAX - 1, "%s/way-displays/cfg.yaml", getenv("XDG_CONFIG_HOME"));
		slist_append(&cfg_file_paths, strdup(path));
	} else if (getenv("HOME") != NULL) {
		// ~/.config
		snprintf(path, PATH_MAX - 1, "%s/.config/way-displays/cfg.yaml", getenv("HOME"));
		slist_append(&cfg_file_paths, strdup(path));
	}

	// etc
	slist_append(&cfg_file_paths, strdup("/usr/local/etc/way-displays/cfg.yaml"));
	slist_append(&cfg_file_paths, strdup(ROOT_ETC"/way-displays/cfg.yaml"));
}

static bool cfg_file_write_content(const char * const yaml) {
	return
		file_write(g_cfg->file_path, COMMENT_YAML_SCHEMA, "w") &&
		file_write(g_cfg->file_path, yaml, "a");
}

void cfg_file_write(void) {
	char *yaml = NULL;
	const char *resolved_from = g_cfg->resolved_from;
	bool written = false;

	g_cfg->updated = false;

	if (!(yaml = yaml_marshal(g_cfg, yaml_doc_cfg, "cfg"))) {
		goto end;
	}

	if (g_cfg->file_path && (written = cfg_file_write_content(yaml))) {
		g_cfg->updated = true;
		goto end;
	}

	if (!written) {

		// kill that cfg file
		cfg_paths_free(g_cfg);
		fd_wd_cfg_dir_destroy();

		// write preferred alternatives
		for (struct SList *i = cfg_file_paths; i; i = i->nex) {

			// skip previously resolved
			if (resolved_from == i->val) {
				continue;
			}

			set_paths(g_cfg, i->val, i->val);

			// attempt to write
			if (mkdir_p(g_cfg->dir_path, 0755) && (written = cfg_file_write_content(yaml))) {

				// watch the new
				fd_wd_cfg_dir_create();
				goto end;
			}

			cfg_paths_free(g_cfg);
		}
	}

end:
	free(yaml);

	if (written) {
		log_info(NULL);
		log_info("Wrote configuration file: %s", g_cfg->file_path);
	}
}

void cfg_destroy(void) {
	cfg_free(g_cfg);
	g_cfg = NULL;
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

	smap_free_vals(cfg->user_modes);

	sset_free(cfg->adaptive_sync_off);

	slist_free_vals(&cfg->max_preferred_refresh_name_desc, NULL);

	pset_free_vals(cfg->disableds);

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

void cfg_user_transform_free(const void *val) {
	struct UserTransform *user_transform = (struct UserTransform*)val;

	if (!user_transform)
		return;

	free(user_transform->name_desc);

	free(user_transform);
}


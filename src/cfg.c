#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cfg.h"

#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "cfg/user-mode.h"
#include "cfg/user-scale.h"
#include "cfg/user-transform.h"
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

static void warn_ambiguous_name_desc(const char *name_desc, const enum CfgElement element) {
	if (!name_desc)
		return;

	if (strlen(name_desc) < 4) {
		log_warn(NULL);
		log_warn("%s '%s' is less than 4 characters, which may result in some unwanted matches.", cfg_element_name(element), name_desc);
	}

	if (strcmp(name_desc, "DP-1") == 0) {
		log_warn(NULL);
		log_warn("%s '%s' will match eDP-1 and DP-1. Consider using regex '!^DP-1$' to exactly match.", cfg_element_name(element), name_desc);
	}
}

static struct Cfg *clone_cfg(struct Cfg *from) {
	if (!from)
		return NULL;

	struct Cfg *to = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	memcpy(to, from, sizeof(struct Cfg));

	to->callback_cmd =          from->callback_cmd ?          strdup(from->callback_cmd) :          NULL;
	to->dir_path =              from->dir_path ?              strdup(from->dir_path) :              NULL;
	to->file_name =             from->file_name ?             strdup(from->file_name) :             NULL;
	to->file_path =             from->file_path ?             strdup(from->file_path) :             NULL;
	to->laptop_display_prefix = from->laptop_display_prefix ? strdup(from->laptop_display_prefix) : NULL;

	to->adaptive_sync_off =               sset_clone(from->adaptive_sync_off);
	to->max_preferred_refresh_name_desc = sset_clone(from->max_preferred_refresh_name_desc);
	to->order_name_desc =                 sset_clone(from->order_name_desc);

	to->disableds =         pset_clone_deep(from->disableds);
	to->user_modes =        smap_clone_deep(from->user_modes);
	to->user_scales =       smap_clone_deep(from->user_scales);
	to->user_transforms =   smap_clone_deep(from->user_transforms);

	return to;
}

bool cfg_equal(const struct Cfg *a, const struct Cfg *b) {
	return a && b &&
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
		sset_equal(a->max_preferred_refresh_name_desc, b->max_preferred_refresh_name_desc) &&
		sset_equal(a->order_name_desc, b->order_name_desc) &&
		a->scale_round_strategy == b->scale_round_strategy &&
		a->scale_round_to == b->scale_round_to &&
		a->scaling == b->scaling &&
		smap_equal(a->user_modes, b->user_modes) &&
		smap_equal(a->user_scales, b->user_scales) &&
		smap_equal(a->user_transforms, b->user_transforms);
}

//
// init functions
//
struct Cfg *cfg_init(void) {
	struct Cfg *cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	cfg->adaptive_sync_off =               sset_init();
	cfg->max_preferred_refresh_name_desc = sset_init();
	cfg->order_name_desc =                 sset_init();

	cfg->disableds =         disabled_pset_init();
	cfg->user_modes =        user_mode_smap_init();
	cfg->user_scales =       user_scale_smap_init();
	cfg->user_transforms =   user_transform_smap_init();

	return cfg;
}

struct Cfg *cfg_default(void) {
	struct Cfg *def = cfg_init();

	cfg_apply_defaults(def);

	return def;
}

void cfg_apply_defaults(struct Cfg *cfg) {
	if (!cfg->arrange)              cfg->arrange =              ARRANGE_DEFAULT;
	if (!cfg->align)                cfg->align =                ALIGN_DEFAULT;
	if (!cfg->scaling)              cfg->scaling =              SCALING_DEFAULT;
	if (!cfg->auto_scale)           cfg->auto_scale =           AUTO_SCALE_DEFAULT;
	if (!cfg->scale_round_to)       cfg->scale_round_to =       SCALE_ROUND_TO_DEFAULT;
	if (!cfg->scale_round_strategy) cfg->scale_round_strategy = SCALE_ROUND_STRATEGY_DEFAULT;
	if (!cfg->auto_scale_dpi)       cfg->auto_scale_dpi =       AUTO_SCALE_DPI_DEFAULT;
	if (!cfg->auto_scale_min)       cfg->auto_scale_min =       AUTO_SCALE_MIN_DEFAULT;
	if (!cfg->auto_scale_max)       cfg->auto_scale_max =       AUTO_SCALE_MAX_DEFAULT;
	if (!cfg->callback_cmd)         cfg->callback_cmd =         strdup(CALLBACK_CMD_DEFAULT);
	if (!cfg->laptop_lid_monitor)   cfg->laptop_lid_monitor =   LAPTOP_LID_MONITOR_DEFAULT;
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

bool cfg_resolve_file_path(struct Cfg *to) {
	if (!to)
		return false;

	cfg_paths_free(to);

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

			set_paths(to, i->val, file_path);

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

	const char *name_desc;

	while ((name_desc = smap_match(cfg->user_scales, (fn_match_smap)user_scale_invalid, NULL).key)) {
		smap_remove_free(cfg->user_scales, name_desc);
	}

	while ((name_desc = smap_match(cfg->user_modes, (fn_match_smap)user_mode_invalid, NULL).key)) {
		smap_remove_free(cfg->user_modes, name_desc);
	}
}

static void warn_ambiguous_name_desc_smap(const struct SMap *name_descs, const enum CfgElement element) {
	for (const struct SMapIt *it = smap_it(name_descs); it; it = smap_it_next(it)) {
		warn_ambiguous_name_desc(it->key, element);
	}
}

static void warn_ambiguous_name_desc_sset(const struct SSet *name_descs, const enum CfgElement element) {
	for (const struct SSetIt *it = sset_it(name_descs); it; it = sset_it_next(it)) {
		warn_ambiguous_name_desc(it->val, element);
	}
}

void validate_warn(const struct Cfg * const cfg) {
	if (!cfg)
		return;

	warn_ambiguous_name_desc_smap(cfg->user_scales, SCALE);
	warn_ambiguous_name_desc_smap(cfg->user_modes, MODE);
	warn_ambiguous_name_desc_smap(cfg->user_transforms, TRANSFORM);

	warn_ambiguous_name_desc_sset(cfg->order_name_desc, ORDER);
	warn_ambiguous_name_desc_sset(cfg->adaptive_sync_off, VRR_OFF);
	warn_ambiguous_name_desc_sset(cfg->max_preferred_refresh_name_desc, MAX_PREFERRED_REFRESH);

	for (const struct PSetIt *dit = pset_it(cfg->disableds); dit; dit = pset_it_next(dit)) {
		if (dit->val) {
			const struct Disabled *disabled = (struct Disabled*)dit->val;
			warn_ambiguous_name_desc(disabled->name_desc, DISABLED);

			for (const struct PSetIt *cit = pset_it(disabled->conditions); cit; cit = pset_it_next(cit)) {
				if (!cit->val)
					continue;
				const struct Condition *condition = (struct Condition*)cit->val;
				warn_ambiguous_name_desc_sset(condition->plugged, PLUGGED);
				warn_ambiguous_name_desc_sset(condition->unplugged, UNPLUGGED);
			}
		}
	}
}

struct Cfg *merge_set(struct Cfg *to, const struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = clone_cfg(to);

	// ARRANGE
	if (from->arrange) {
		merged->arrange = from->arrange;
	}

	// ALIGN
	if (from->align) {
		merged->align = from->align;
	}

	// ORDER, replace
	sset_free(merged->order_name_desc);
	merged->order_name_desc = sset_clone(from->order_name_desc);

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
	for (const struct SMapIt *it = smap_it(from->user_scales); it; it = smap_it_next(it)) {
		smap_put_free(merged->user_scales, it->key, user_scale_clone(it->val));
	}

	// MODE
	for (const struct SMapIt *it = smap_it(from->user_modes); it; it = smap_it_next(it)) {
		smap_put_free(merged->user_modes, it->key, user_mode_clone(it->val));
	}

	// TRANSFORM
	for (const struct SMapIt *it = smap_it(from->user_transforms); it; it = smap_it_next(it)) {
		smap_put_free(merged->user_transforms, it->key, user_transform_clone(it->val));
	}

	// VRR_OFF
	for (const struct SSetIt *it = sset_it(from->adaptive_sync_off); it; it = sset_it_next(it)) {
		sset_add(merged->adaptive_sync_off, it->val);
	}

	// DISABLED
	for (const struct PSetIt *it = pset_it(from->disableds); it; it = pset_it_next(it)) {
		const struct Disabled *d = disabled_clone(it->val);
		if (!pset_add(merged->disableds, d)) {
			disabled_free((struct Disabled*)d);
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

	// SCALE
	for (const struct SMapIt *it = smap_it(from->user_scales); it; it = smap_it_next(it)) {
		smap_remove_free(merged->user_scales, it->key);
	}

	// MODE
	for (const struct SMapIt *it = smap_it(from->user_modes); it; it = smap_it_next(it)) {
		smap_remove_free(merged->user_modes, it->key);
	}

	// TRANSFORM
	for (const struct SMapIt *it = smap_it(from->user_transforms); it; it = smap_it_next(it)) {
		smap_remove_free(merged->user_transforms, it->key);
	}


	// VRR_OFF
	for (const struct SSetIt *it = sset_it(from->adaptive_sync_off); it; it = sset_it_next(it)) {
		sset_remove(merged->adaptive_sync_off, it->val);
	}

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

	if (!(yaml = yaml_marshal(g_cfg, (fn_yaml_doc)yaml_doc_cfg, "cfg"))) {
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
	pset_free_vals(cfg->disableds);
	smap_free_vals(cfg->user_modes);
	smap_free_vals(cfg->user_scales);
	smap_free_vals(cfg->user_transforms);
	sset_free(cfg->adaptive_sync_off);
	sset_free(cfg->max_preferred_refresh_name_desc);
	sset_free(cfg->order_name_desc);

	free(cfg);
}


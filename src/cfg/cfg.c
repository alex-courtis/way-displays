#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg/cfg.h"

#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "enum.h"
#include "fn.h"
#include "info/callback.h"
#include "info/print.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "simap.h"
#include "spmap.h"
#include "sset.h"
#include "str.h"

struct Cfg *g_cfg = NULL;

static void warn_ambiguous_name_desc(const char *name_desc, const enum CfgElement element) {
	if (strlen(name_desc) < 4) {
		log_warn(NULL);
		log_warn("%s '%s' is less than 4 characters, which may result in some unwanted matches.", cfg_element_name(element), name_desc);
	}

	if (strcmp(name_desc, "DP-1") == 0) {
		log_warn(NULL);
		log_warn("%s '%s' will match eDP-1 and DP-1. Consider using regex '!^DP-1$' to exactly match.", cfg_element_name(element), name_desc);
	}
}

static void warn_ambiguous_name_desc_spmap(const struct SPmap *name_descs, const enum CfgElement element) {
	for (const struct SPmapIt *it = spmap_it(name_descs); it; it = spmap_it_next(it)) {
		warn_ambiguous_name_desc(it->key, element);
	}
}

static void warn_ambiguous_name_desc_simap(const struct SImap *name_descs, const enum CfgElement element) {
	for (const struct SImapIt *it = simap_it(name_descs); it; it = simap_it_next(it)) {
		warn_ambiguous_name_desc(it->key, element);
	}
}

static void warn_ambiguous_name_desc_sset(const struct Sset *name_descs, const enum CfgElement element) {
	for (const struct SsetIt *it = sset_it(name_descs); it; it = sset_it_next(it)) {
		warn_ambiguous_name_desc(it->val, element);
	}
}

// fn_pred_sp: not a valid cfg mode
static bool mode_is_invalid(const char* const name_desc, const struct Mode* const mode) {
	if (mode->width != -1 && mode->width <= 0) {
		log_warn(NULL);
		log_warn("Ignoring non-positive MODE %s WIDTH %d", name_desc, mode->width);
		return true;
	}
	if (mode->height != -1 && mode->height <= 0) {
		log_warn(NULL);
		log_warn("Ignoring non-positive MODE %s HEIGHT %d", name_desc, mode->height);
		return true;
	}
	if (mode->refresh_mhz != -1 && mode->refresh_mhz <= 0) {
		log_warn(NULL);
		log_warn("Ignoring non-positive MODE %s HZ %g", name_desc, ((float)mode->refresh_mhz) / 1000);
		return true;
	}

	if (!mode->max && !mode->max_preferred_refresh) {
		if (mode->width == -1) {
			log_warn(NULL);
			log_warn("Ignoring invalid MODE %s missing WIDTH", name_desc);
			return true;
		}
		if (mode->height == -1) {
			log_warn(NULL);
			log_warn("Ignoring invalid MODE %s missing HEIGHT", name_desc);
			return true;
		}
	}

	return false;
}

struct Cfg *cfg_init(void) {
	struct Cfg *cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	cfg->adaptive_sync_off     = sset_init();
	cfg->disableds             = cfg_disabled_spmap_init();
	cfg->modes                 = mode_spmap_init();
	cfg->order_name_desc       = sset_init();
	cfg->scales                = simap_init();
	cfg->transforms            = simap_init();

	return cfg;
}

struct Cfg *cfg_clone(struct Cfg *from) {
	if (!from)
		return NULL;

	struct Cfg *to = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	memcpy(to, from, sizeof(struct Cfg));

	to->callback_cmd          = from->callback_cmd          ? strdup(from->callback_cmd)          : NULL;

	to->adaptive_sync_off     = sset_clone(from->adaptive_sync_off);
	to->order_name_desc       = sset_clone(from->order_name_desc);
	to->disableds             = spmap_clone_deep(from->disableds);
	to->modes                 = spmap_clone_deep(from->modes);
	to->scales                = simap_clone(from->scales);
	to->transforms            = simap_clone(from->transforms);

	return to;
}

void cfg_free(struct Cfg *cfg) {
	if (!cfg)
		return;

	free(cfg->callback_cmd);
	spmap_free_vals(cfg->disableds);
	spmap_free_vals(cfg->modes);
	simap_free(cfg->scales);
	simap_free(cfg->transforms);
	sset_free(cfg->adaptive_sync_off);
	sset_free(cfg->order_name_desc);

	free(cfg);
}

void g_cfg_destroy(void) {
	cfg_free(g_cfg);
	g_cfg = NULL;
}

bool cfg_equal(const struct Cfg *a, const struct Cfg *b) {
	return a && b &&
		a->align == b->align &&
		a->arrange == b->arrange &&
		a->auto_scale == b->auto_scale &&
		a->auto_scale_dpi == b->auto_scale_dpi &&
		a->auto_scale_max == b->auto_scale_max &&
		a->auto_scale_min == b->auto_scale_min &&
		a->laptop_lid_monitor == b->laptop_lid_monitor &&
		a->log_threshold == b->log_threshold &&
		a->scale_round_strategy == b->scale_round_strategy &&
		a->scale_round_to == b->scale_round_to &&
		a->scaling == b->scaling &&
		equal_strcmp(a->callback_cmd, b->callback_cmd) &&
		spmap_equal(a->disableds, b->disableds) &&
		spmap_equal(a->modes, b->modes) &&
		simap_equal(a->scales, b->scales) &&
		simap_equal(a->transforms, b->transforms) &&
		sset_equal(a->adaptive_sync_off, b->adaptive_sync_off) &&
		sset_equal(a->order_name_desc, b->order_name_desc);
}

void cfg_apply_defaults(struct Cfg *cfg) {
	if (!cfg->arrange)              cfg->arrange              = ARRANGE_DEFAULT;
	if (!cfg->align)                cfg->align                = ALIGN_DEFAULT;
	if (!cfg->scaling)              cfg->scaling              = SCALING_DEFAULT;
	if (!cfg->auto_scale)           cfg->auto_scale           = AUTO_SCALE_DEFAULT;
	if (!cfg->scale_round_to)       cfg->scale_round_to       = SCALE_ROUND_TO_DEFAULT;
	if (!cfg->scale_round_strategy) cfg->scale_round_strategy = SCALE_ROUND_STRATEGY_DEFAULT;
	if (!cfg->auto_scale_dpi)       cfg->auto_scale_dpi       = AUTO_SCALE_DPI_DEFAULT;
	if (!cfg->auto_scale_min)       cfg->auto_scale_min       = AUTO_SCALE_MIN_DEFAULT;
	if (!cfg->auto_scale_max)       cfg->auto_scale_max       = AUTO_SCALE_MAX_DEFAULT;
	if (!cfg->callback_cmd)         cfg->callback_cmd         = strdup(CALLBACK_CMD_DEFAULT);
	if (!cfg->laptop_lid_monitor)   cfg->laptop_lid_monitor   = LAPTOP_LID_MONITOR_DEFAULT;

	// add the default lid condition unless the user has specified an empty map or some valid disableds
	if (!cfg->disableds_empty && spmap_size(cfg->disableds) == 0) {
		cfg_disabled_add_lid_default(cfg->disableds, DISABLED_LAPTOP_DISPLAY_NAME_DESC_DEFAULT);
	}
}

void cfg_migrate_v1(struct Cfg *cfg, const char *v1_laptop_display_prefix) {
	print_v1_deprecation();
	callback_v1_deprecation(cfg);

	// note numbers
	log_warn("Migrated NAME_DESC arrays to keyed maps:");

	log_warn("  %s:", cfg_element_name(SCALE));
	for (const struct SImapIt *it = simap_it(cfg->scales); it; it = simap_it_next(it))
		log_warn("    '%s':", it->key);

	log_warn("  %s:", cfg_element_name(MODE));
	for (const struct SPmapIt *it = spmap_it(cfg->modes); it; it = spmap_it_next(it))
		log_warn("    '%s':", it->key);

	log_warn("  %s:", cfg_element_name(TRANSFORM));
	for (const struct SImapIt *it = simap_it(cfg->transforms); it; it = simap_it_next(it))
		log_warn("    '%s':", it->key);

	log_warn("  %s:", cfg_element_name(DISABLED));
	for (const struct SPmapIt *it = spmap_it(cfg->disableds); it; it = spmap_it_next(it))
		log_warn("    '%s':", it->key);

	// always add laptop display lid closed condition
	if (v1_laptop_display_prefix) {
		char *name_desc = sprintf_alloc("!^%s", v1_laptop_display_prefix);
		log_warn("Migrated %s to lid closed condition:", cfg_element_name(LAPTOP_DISPLAY_PREFIX));
		log_warn("  %s:", cfg_element_name(DISABLED));
		log_warn("    '%s':", name_desc);
		cfg_disabled_add_lid_default(cfg->disableds, name_desc);
		free(name_desc);
	} else {
		log_warn("Added default lid closed condition:");
		log_warn("  %s:", cfg_element_name(DISABLED));
		log_warn("    '%s':", DISABLED_LAPTOP_DISPLAY_NAME_DESC_DEFAULT);
		cfg_disabled_add_lid_default(cfg->disableds, DISABLED_LAPTOP_DISPLAY_NAME_DESC_DEFAULT);
	}

	log_warn(NULL);
	log_warn("Please upgrade your cfg.yaml:");
	log_warn("  way-displays --write");
}

struct Cfg *cfg_merge(struct Cfg *to, const struct Cfg *from, const enum IpcCommand command) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = NULL;

	switch (command) {
		case CFG_DEL:
			merged = cfg_merge_del(to, from);
			break;
		case CFG_TOGGLE:
			merged = cfg_merge_toggle(to, from);
			break;
		case CFG_SET:
			merged = cfg_merge_set(to, from);
			break;
		default:
			break;
	}

	if (merged) {
		cfg_validate_fix(merged);
		cfg_validate_warn(merged);

		if (cfg_equal(merged, to)) {
			cfg_free(merged);
			merged = NULL;
		}
	}

	return merged;
}

struct Cfg *cfg_merge_set(struct Cfg *to, const struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = cfg_clone(to);

	// upsert
	merged->align                = from->align                ? from->align                : merged->align;
	merged->arrange              = from->arrange              ? from->arrange              : merged->arrange;
	merged->auto_scale           = from->auto_scale           ? from->auto_scale           : merged->auto_scale;
	merged->scaling              = from->scaling              ? from->scaling              : merged->scaling;
	if (from->callback_cmd) {
		if (merged->callback_cmd) {
			free(merged->callback_cmd);
		}
		merged->callback_cmd = strdup(from->callback_cmd);
	}
	spmap_put_all_clone_free(merged->disableds,         from->disableds);
	spmap_put_all_clone_free(merged->modes,             from->modes);
	simap_put_all           (merged->scales,            from->scales);
	simap_put_all           (merged->transforms,        from->transforms);
	sset_add_all            (merged->adaptive_sync_off, from->adaptive_sync_off);

	// replace if present
	if (sset_size(from->order_name_desc) > 0) {
		sset_free(merged->order_name_desc);
		merged->order_name_desc = sset_clone(from->order_name_desc);
	}

	return merged;
}

struct Cfg *cfg_merge_del(struct Cfg *to, const struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = cfg_clone(to);

	spmap_remove_in_free(merged->disableds,         from->disableds);
	spmap_remove_in_free(merged->modes,             from->modes);
	simap_remove_in     (merged->scales,            from->scales);
	simap_remove_in     (merged->transforms,        from->transforms);
	sset_remove_in      (merged->adaptive_sync_off, from->adaptive_sync_off);

	// any string means no callback
	if (from->callback_cmd) {
		free(merged->callback_cmd);
		merged->callback_cmd = NULL;
	}

	return merged;
}

static enum OnOff on_off_invert(enum OnOff val) {
	return (val == ON) ? OFF : ON;
}

struct Cfg *cfg_merge_toggle(struct Cfg *to, const struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = cfg_clone(to);

	// the IpcRequest passes ON to indicate CFG_TOGGLE, regardless of state

	// SCALE
	if (from->scaling == ON) {
		merged->scaling = on_off_invert(merged->scaling);
	}

	// AUTO_SCALE
	if (from->auto_scale == ON) {
		merged->auto_scale = on_off_invert(merged->auto_scale);
	}

	// VRR_OFF
	for (const struct SsetIt *it = sset_it(from->adaptive_sync_off); it; it = sset_it_next(it)) {
		if (!sset_remove(merged->adaptive_sync_off, it->val)) {
			sset_add(merged->adaptive_sync_off, it->val);
		}
	}

	// DISABLED, conditionals toggled and filtered earlier
	for (const struct SPmapIt *it = spmap_it(from->disableds); it; it = spmap_it_next(it)) {
		if (spmap_put_if_absent_clone(merged->disableds, it->key, it->val)) {
			spmap_remove_free(merged->disableds, it->key);
		}
	}

	return merged;
}

void cfg_validate_warn(const struct Cfg * const cfg) {
	if (!cfg)
		return;

	warn_ambiguous_name_desc_simap(cfg->scales, SCALE);
	warn_ambiguous_name_desc_spmap(cfg->modes, MODE);

	warn_ambiguous_name_desc_simap(cfg->transforms, TRANSFORM);

	warn_ambiguous_name_desc_sset(cfg->order_name_desc, ORDER);
	warn_ambiguous_name_desc_sset(cfg->adaptive_sync_off, VRR_OFF);

	for (const struct SPmapIt *dit = spmap_it(cfg->disableds); dit; dit = spmap_it_next(dit)) {
		const struct CfgDisabled *disabled = (struct CfgDisabled*)dit->val;
		warn_ambiguous_name_desc(dit->key, DISABLED);

		for (const struct PsetIt *cit = pset_it(disabled->conditions); cit; cit = pset_it_next(cit)) {
			const struct CfgCondition *condition = (struct CfgCondition*)cit->val;
			warn_ambiguous_name_desc_sset(condition->plugged, PLUGGED);
			warn_ambiguous_name_desc_sset(condition->unplugged, UNPLUGGED);
		}
	}
}

void cfg_validate_fix(struct Cfg *cfg) {
	if (!cfg) {
		return;
	}

	// warn and fix bad ARRANGE ALIGN
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

	// warn and default bad AUTO_SCALE_DPI
	if (cfg->auto_scale_dpi <= AUTO_SCALE_DPI_MIN) {
		log_warn(NULL);
		log_warn("Ignoring AUTO_SCALE_DPI %d < %d. Using default %d.", cfg->auto_scale_dpi, AUTO_SCALE_DPI_MIN, AUTO_SCALE_DPI_DEFAULT);
		cfg->auto_scale_dpi = AUTO_SCALE_DPI_DEFAULT;
	}

	// warn and remove invalid MODE
	struct SPmapFilter f = { .key_val = (fn_pred_sp)mode_is_invalid, };
	for (const struct SPmapIt *it = spmap_filter_it(cfg->modes, f); it; it = spmap_it_next(it)) {
		spmap_it_remove_free(it);
	}
}


#include <stdbool.h>
#include <stdlib.h>
#include <yaml.h>

#include "yaml/marshal-types-cfg.h"

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "enum.h"
#include "mode.h"
#include "pset.h"
#include "simap.h"
#include "spmap.h"
#include "str.h"
#include "yaml/marshal-primitives.h"
#include "yaml/marshal.h"

bool yaml_root_from_cfg(struct MC *c, const struct Cfg* const cfg) {

	// creates a mapping node which is the root
	return yaml_map_from_cfg(c, cfg) != 0;
}

int yaml_map_from_cfg(struct MC *c, const struct Cfg* const cfg) {
	if (!cfg)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	// order is important
	yaml_map_add_enum    (c, cfg_element_name(ARRANGE),               cfg->arrange,               arrange_name,                                         map);
	yaml_map_add_enum    (c, cfg_element_name(ALIGN),                 cfg->align,                 align_name,                                           map);
	yaml_map_add_sset    (c, cfg_element_name(ORDER),                 cfg->order_name_desc,                                                             map);
	yaml_map_add_enum    (c, cfg_element_name(SCALING),               cfg->scaling,               on_off_name,                                          map);
	yaml_map_add_enum    (c, cfg_element_name(SCALE_ROUND_TO),        cfg->scale_round_to,        scale_round_to_name,                                  map);
	yaml_map_add_enum    (c, cfg_element_name(SCALE_ROUND_STRATEGY),  cfg->scale_round_strategy,  scale_round_strategy_name,                            map);
	yaml_map_add_enum    (c, cfg_element_name(AUTO_SCALE),            cfg->auto_scale,            on_off_name,                                          map);
	yaml_map_add_int_nz  (c, cfg_element_name(AUTO_SCALE_DPI),        cfg->auto_scale_dpi,                                                              map);
	yaml_map_add_float_nz(c, cfg_element_name(AUTO_SCALE_MIN),        cfg->auto_scale_min,                                                              map);
	yaml_map_add_float_nz(c, cfg_element_name(AUTO_SCALE_MAX),        cfg->auto_scale_max,                                                              map);
	yaml_map_add_node    (c, cfg_element_name(SCALE),                 yaml_map_from_scales(c, cfg->scales),                                             map);
	yaml_map_add_node    (c, cfg_element_name(MODE),                  yaml_map_from_cfg_modes(c, cfg->modes),                                           map);
	yaml_map_add_node    (c, cfg_element_name(TRANSFORM),             yaml_map_from_transforms(c, cfg->transforms),                                     map);
	yaml_map_add_sset    (c, cfg_element_name(VRR_OFF),               cfg->adaptive_sync_off,                                                           map);
	yaml_map_add_str     (c, cfg_element_name(CALLBACK_CMD),          cfg->callback_cmd,                                                                map);
	yaml_map_add_str     (c, cfg_element_name(LAPTOP_DISPLAY_PREFIX), cfg->laptop_display_prefix,                                                       map);
	yaml_map_add_enum    (c, cfg_element_name(LAPTOP_LID_MONITOR),    cfg->laptop_lid_monitor,    on_off_name,                                          map);
	yaml_map_add_enum    (c, cfg_element_name(LOG_THRESHOLD),         cfg->log_threshold,         log_threshold_name,                                   map);
	yaml_map_add_node    (c, cfg_element_name(DISABLED),              yaml_map_from_disableds(c, cfg->disableds),                                       map);

	return map;
}

int yaml_map_from_cfg_modes(struct MC *c, const struct SPmap* const modes) {
	int map_out;
	if (spmap_size(modes) < 1 || !(map_out = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE)))
		return 0;

	for (const struct SPmapIt *it = spmap_it(modes); it; it = spmap_it_next(it)) {
		const struct Mode *mode = it->val;

		int map_in = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
		if (!map_in)
			continue;

		if (mode->max) {
			yaml_map_add_bool(c, "MAX", mode->max, map_in);
		} else {
			yaml_map_add_int(c, "WIDTH", mode->width, map_in);
			yaml_map_add_int(c, "HEIGHT", mode->height, map_in);
			if (mode->refresh_mhz != -1) {
				char *hz = sprintf_alloc("%g", ((float)mode->refresh_mhz) / 1000);
				yaml_map_add_str(c, "HZ", hz, map_in);
				free(hz);
			}
		}

		yaml_map_add_node(c, it->key, map_in, map_out);
	}

	return map_out;
}

int yaml_map_from_scales(struct MC *c, const struct SImap* const scales) {
	int map;
	if (simap_size(scales) < 1 || !(map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE)))
		return 0;

	for (const struct SImapIt *it = simap_it(scales); it; it = simap_it_next(it)) {
		yaml_map_add_int(c, it->key, (double)it->val/1000, map);
	}

	return map;
}

int yaml_map_from_transforms(struct MC *c, const struct SImap* const transforms) {
	int map;
	if (simap_size(transforms) < 1 || !(map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE)))
		return 0;

	for (const struct SImapIt *it = simap_it(transforms); it; it = simap_it_next(it)) {
		yaml_map_add_str(c, it->key, transform_name(it->val), map);
	}

	return map;
}

int yaml_map_from_condition(struct MC *c, const struct CfgCondition* const condition) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_sset(c, "PLUGGED", condition->plugged, map);
	yaml_map_add_sset(c, "UNPLUGGED", condition->unplugged, map);
	yaml_map_add_enum(c, "LID", condition->lid, condition_lid_name, map);

	return map;
}

int yaml_map_from_disableds(struct MC *c, const struct SPmap* const disableds) {
	int map;
	if (spmap_size(disableds) < 1 || !(map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE)))
		return 0;

	for (const struct SPmapIt *it = spmap_it(disableds); it; it = spmap_it_next(it)) {
		const struct CfgDisabled *disabled = it->val;
		if (pset_size(disabled->conditions) > 0 ) {
			int map_if = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
			if (!map_if)
				continue;

			const struct Plist *conditions = pset_plist(disabled->conditions);
			yaml_map_add_plist(c, "IF", conditions, (fn_yaml_node_from_type)yaml_map_from_condition, map_if);
			plist_free(conditions);

			yaml_map_add_node(c, it->key, map_if, map);
		} else {
			yaml_map_add_str(c, it->key, "", map);
		}
	}

	return map;
}


#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>

#include "yaml-marshal.h"

#include "cfg.h"
#include "convert.h"
#include "conditions.h"
#include "mode.h"

static bool seq_user_scale(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct UserScale *user_scale = data;

	int map = yaml_document_add_mapping(ctx.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		map_key_to_str("NAME_DESC", user_scale->name_desc, map) &&
		map_key_to_float("SCALE", user_scale->scale, map) &&
		yaml_document_append_sequence_item(ctx.document, sequence, map);
}

static bool seq_user_mode(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct UserMode *user_mode = data;

	int map = yaml_document_add_mapping(ctx.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (!map || !map_key_to_str("NAME_DESC", user_mode->name_desc, map))
		return false;

	if (user_mode->max) {
		if (!map_key_to_bool("MAX", user_mode->max, map)) {
			return false;
		}
	} else {
		if (!map_key_to_int("WIDTH", user_mode->width, map) || !map_key_to_int("HEIGHT", user_mode->height, map))
			return false;
		if (user_mode->refresh_mhz != -1) {
			if (!map_key_to_str("HZ", mhz_to_hz_str(user_mode->refresh_mhz), map)) {
				return false;
			}
		}
	}

	return yaml_document_append_sequence_item(ctx.document, sequence, map);
}

static bool seq_user_transform(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct UserTransform *user_transform = data;

	int map = yaml_document_add_mapping(ctx.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		map_key_to_str("NAME_DESC", user_transform->name_desc, map) &&
		map_key_to_str("TRANSFORM", transform_name(user_transform->transform), map) &&
		yaml_document_append_sequence_item(ctx.document, sequence, map);
}

static bool seq_condition(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct Condition *condition = data;

	int map = yaml_document_add_mapping(ctx.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (condition->plugged)
		map_key_to_list("PLUGGED", condition->plugged, seq_str, map);

	if (condition->unplugged)
		map_key_to_list("UNPLUGGED", condition->unplugged, seq_str, map);

	return yaml_document_append_sequence_item(ctx.document, sequence, map);
}

static bool seq_disabled(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct Disabled *disabled = data;

	if (!disabled->name_desc)
		return false;

	if (disabled->conditions) {
		int map = yaml_document_add_mapping(ctx.document, NULL, YAML_BLOCK_MAPPING_STYLE);

		return map &&
			map_key_to_str("NAME_DESC", disabled->name_desc, map) &&
			map_key_to_list("IF", disabled->conditions, seq_condition, map) &&
			yaml_document_append_sequence_item(ctx.document, sequence, map);
	} else {
		return seq_str(disabled->name_desc, sequence);
	}
}

bool map_cfg(const void *data, int mapping) {
	if (!data)
		return false;

	const struct Cfg *cfg = data;

	map_key_to_enum (cfg_element_name(ARRANGE),               cfg->arrange,                     arrange_name,       mapping);
	map_key_to_enum (cfg_element_name(ALIGN),                 cfg->align,                       align_name,         mapping);
	map_key_to_list (cfg_element_name(ORDER),                 cfg->order_name_desc,             seq_str,            mapping);
	map_key_to_enum (cfg_element_name(SCALING),               cfg->scaling,                     on_off_name,        mapping);
	map_key_to_enum (cfg_element_name(AUTO_SCALE),            cfg->auto_scale,                  on_off_name,        mapping);
	map_key_to_float(cfg_element_name(AUTO_SCALE_MIN),        cfg->auto_scale_min,                                  mapping);
	map_key_to_float(cfg_element_name(AUTO_SCALE_MAX),        cfg->auto_scale_max,                                  mapping);
	map_key_to_list (cfg_element_name(SCALE),                 cfg->user_scales,                 seq_user_scale,     mapping);
	map_key_to_list (cfg_element_name(MODE),                  cfg->user_modes,                  seq_user_mode,      mapping);
	map_key_to_list (cfg_element_name(TRANSFORM),             cfg->user_transforms,             seq_user_transform, mapping);
	map_key_to_list (cfg_element_name(VRR_OFF),               cfg->adaptive_sync_off_name_desc, seq_str,            mapping);
	map_key_to_str  (cfg_element_name(CALLBACK_CMD),          cfg->callback_cmd,                                    mapping);
	map_key_to_str  (cfg_element_name(LAPTOP_DISPLAY_PREFIX), cfg->laptop_display_prefix,                           mapping);
	map_key_to_enum (cfg_element_name(LOG_THRESHOLD),         cfg->log_threshold,               log_threshold_name, mapping);
	map_key_to_list (cfg_element_name(DISABLED),              cfg->disabled,                    seq_disabled,       mapping);

	return true;
}

char *marshal_cfg_2(const struct Cfg *cfg) {
	return marshal_yaml(cfg, map_cfg, "cfg");
}

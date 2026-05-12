#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>

#include "yaml-marshal-cfg.h"

#include "cfg.h"
#include "convert.h"
#include "conditions.h"
#include "log.h"
#include "mode.h"
#include "yaml-marshal.h"
#include "yaml/marshal-primitives.h"

static bool seq_user_scale(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct UserScale *user_scale = data;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		yaml_map_add_str("NAME_DESC", user_scale->name_desc, map) &&
		yaml_map_add_float("SCALE", user_scale->scale, map) &&
		yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

static bool seq_user_mode(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct UserMode *user_mode = data;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (!map || !yaml_map_add_str("NAME_DESC", user_mode->name_desc, map))
		return false;

	if (user_mode->max) {
		if (!yaml_map_add_bool("MAX", user_mode->max, map)) {
			return false;
		}
	} else {
		if (!yaml_map_add_int("WIDTH", user_mode->width, map) || !yaml_map_add_int("HEIGHT", user_mode->height, map))
			return false;
		if (user_mode->refresh_mhz != -1) {
			if (!yaml_map_add_str("HZ", mhz_to_hz_str(user_mode->refresh_mhz), map)) {
				return false;
			}
		}
	}

	return yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

static bool seq_user_transform(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct UserTransform *user_transform = data;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		yaml_map_add_str("NAME_DESC", user_transform->name_desc, map) &&
		yaml_map_add_str("TRANSFORM", transform_name(user_transform->transform), map) &&
		yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

static bool seq_condition(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct Condition *condition = data;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (condition->plugged)
		yaml_map_add_seq("PLUGGED", condition->plugged, yaml_seq_append_str, map);

	if (condition->unplugged)
		yaml_map_add_seq("UNPLUGGED", condition->unplugged, yaml_seq_append_str, map);

	return yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

static bool seq_disabled(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct Disabled *disabled = data;

	if (!disabled->name_desc)
		return false;

	if (disabled->conditions) {
		int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

		return map &&
			yaml_map_add_str("NAME_DESC", disabled->name_desc, map) &&
			yaml_map_add_seq("IF", disabled->conditions, seq_condition, map) &&
			yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
	} else {
		return yaml_seq_append_str(disabled->name_desc, sequence);
	}
}

bool map_cfg(const void *data, int mapping) {
	if (!data)
		return false;

	const struct Cfg *cfg = data;

	// TODO tidy spacing
	yaml_map_add_enum (cfg_element_name(ARRANGE),               cfg->arrange,                     arrange_name,       mapping);
	yaml_map_add_enum (cfg_element_name(ALIGN),                 cfg->align,                       align_name,         mapping);
	yaml_map_add_seq (cfg_element_name(ORDER),                 cfg->order_name_desc,             yaml_seq_append_str,            mapping);
	yaml_map_add_enum (cfg_element_name(SCALING),               cfg->scaling,                     on_off_name,        mapping);
	yaml_map_add_enum (cfg_element_name(AUTO_SCALE),            cfg->auto_scale,                  on_off_name,        mapping);
	yaml_map_add_float(cfg_element_name(AUTO_SCALE_MIN),        cfg->auto_scale_min,                                  mapping);
	yaml_map_add_float(cfg_element_name(AUTO_SCALE_MAX),        cfg->auto_scale_max,                                  mapping);
	yaml_map_add_seq (cfg_element_name(SCALE),                 cfg->user_scales,                 seq_user_scale,     mapping);
	yaml_map_add_seq (cfg_element_name(MODE),                  cfg->user_modes,                  seq_user_mode,      mapping);
	yaml_map_add_seq (cfg_element_name(TRANSFORM),             cfg->user_transforms,             seq_user_transform, mapping);
	yaml_map_add_seq (cfg_element_name(VRR_OFF),               cfg->adaptive_sync_off_name_desc, yaml_seq_append_str,            mapping);
	yaml_map_add_str  (cfg_element_name(CALLBACK_CMD),          cfg->callback_cmd,                                    mapping);
	yaml_map_add_str  (cfg_element_name(LAPTOP_DISPLAY_PREFIX), cfg->laptop_display_prefix,                           mapping);
	yaml_map_add_enum (cfg_element_name(LOG_THRESHOLD),         cfg->log_threshold,               log_threshold_name, mapping);
	yaml_map_add_seq (cfg_element_name(DISABLED),              cfg->disabled,                    seq_disabled,       mapping);

	return true;
}

static bool marshal_cfg_fn(const void *data) {
	if (!data)
		return false;

	int mapping = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!mapping) {
		log_error("unable to marshal cfg: yaml_document_add_mapping for root failed");
		return false;
	}

	return map_cfg(data, mapping);
}

char *cfg_to_yaml(const struct Cfg *cfg) {
	return struct_to_yaml(cfg, marshal_cfg_fn, "cfg");
}

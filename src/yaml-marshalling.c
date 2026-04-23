#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <yaml.h>

#include "marshalling.h"

#include "cfg.h"
#include "convert.h"
#include "conditions.h"
#include "log.h"
#include "mode.h"
#include "slist.h"

struct MarshallingContext {
	yaml_document_t *document;
} ctx = { 0 };

typedef bool (*map_list_fn)(const void *data, int sequence);
static bool map_list(const char *k, const struct SList *list, map_list_fn fn, int mapping) {
	if (!k || !list || !fn || !mapping)
		return false;

	int key = yaml_document_add_scalar(ctx.document, NULL, (yaml_char_t *)k, -1, YAML_PLAIN_SCALAR_STYLE);
	int sequence = yaml_document_add_sequence(ctx.document, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!key || !sequence)
		return false;

	for (const struct SList *i = list; i; i = i->nex)
		fn(i->val, sequence);

	return yaml_document_append_mapping_pair(ctx.document, mapping, key, sequence);
}

static bool map_str(const char *k, const char *v, int mapping) {
	if (!k || !v || !mapping)
		return false;

	int key = yaml_document_add_scalar(ctx.document, (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG, (yaml_char_t *)k, -1, YAML_PLAIN_SCALAR_STYLE);
	int scalar = yaml_document_add_scalar(ctx.document, NULL, (yaml_char_t *)v, -1, YAML_PLAIN_SCALAR_STYLE);

	return key && scalar && yaml_document_append_mapping_pair(ctx.document, mapping, key, scalar);
}

static bool seq_str(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	int scalar = yaml_document_add_scalar(ctx.document, NULL, (yaml_char_t *)data, -1, YAML_PLAIN_SCALAR_STYLE);

	return scalar && yaml_document_append_sequence_item(ctx.document, sequence, scalar);
}

static bool map_int(const char *k, const int32_t v, int mapping) {
	if (!k || !mapping)
		return false;

	char v_str[20];
	snprintf(v_str, 20, "%d", v);

	return map_str(k, v_str, mapping);
}

static bool map_float(const char *k, const float v, int mapping) {
	if (!k || v == 0 || !mapping)
		return false;

	char v_str[100];
	snprintf(v_str, 100, "%g", v);

	return map_str(k, v_str, mapping);
}

static bool map_bool(const char *k, const bool v, int mapping) {
	if (!k || !mapping)
		return false;

	return map_str(k, (v ? "TRUE" : "FALSE"), mapping);
}

typedef const char* (*map_enum_fn_name)(unsigned int v);
static bool map_enum(const char *k, const int v, map_enum_fn_name fn_name, int mapping) {
	if (!k || !fn_name || !mapping)
		return false;

	const char *v_str = fn_name(v);
	if (!v_str)
		return false;

	// use T/F to obey schema
	if (fn_name == on_off_name)
		v_str = (v == ON) ? "TRUE" : "FALSE";

	return map_str(k, v_str, mapping);
}

static bool seq_user_scale(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct UserScale *user_scale = data;

	int map = yaml_document_add_mapping(ctx.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		map_str("NAME_DESC", user_scale->name_desc, map) &&
		map_float("SCALE", user_scale->scale, map) &&
		yaml_document_append_sequence_item(ctx.document, sequence, map);
}

static bool seq_user_mode(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct UserMode *user_mode = data;

	int map = yaml_document_add_mapping(ctx.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (!map || !map_str("NAME_DESC", user_mode->name_desc, map))
		return false;

	if (user_mode->max) {
		if (!map_bool("MAX", user_mode->max, map)) {
			return false;
		}
	} else {
		if (!map_int("WIDTH", user_mode->width, map) || !map_int("HEIGHT", user_mode->height, map))
			return false;
		if (user_mode->refresh_mhz != -1) {
			if (!map_str("HZ", mhz_to_hz_str(user_mode->refresh_mhz), map)) {
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
		map_str("NAME_DESC", user_transform->name_desc, map) &&
		map_str("TRANSFORM", transform_name(user_transform->transform), map) &&
		yaml_document_append_sequence_item(ctx.document, sequence, map);
}

static bool seq_condition(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct Condition *condition = data;

	int map = yaml_document_add_mapping(ctx.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (condition->plugged)
		map_list("PLUGGED", condition->plugged, seq_str, map);

	if (condition->unplugged)
		map_list("UNPLUGGED", condition->unplugged, seq_str, map);

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
			map_str("NAME_DESC", disabled->name_desc, map) &&
			map_list("IF", disabled->conditions, seq_condition, map) &&
			yaml_document_append_sequence_item(ctx.document, sequence, map);
	} else {
		return seq_str(disabled->name_desc, sequence);
	}
}

static bool map_cfg(const struct Cfg *cfg, int mapping) {
	map_enum (cfg_element_name(ARRANGE),               cfg->arrange,                     arrange_name,       mapping);
	map_enum (cfg_element_name(ALIGN),                 cfg->align,                       align_name,         mapping);
	map_list (cfg_element_name(ORDER),                 cfg->order_name_desc,             seq_str,            mapping);
	map_enum (cfg_element_name(SCALING),               cfg->scaling,                     on_off_name,        mapping);
	map_enum (cfg_element_name(AUTO_SCALE),            cfg->auto_scale,                  on_off_name,        mapping);
	map_float(cfg_element_name(AUTO_SCALE_MIN),        cfg->auto_scale_min,                                  mapping);
	map_float(cfg_element_name(AUTO_SCALE_MAX),        cfg->auto_scale_max,                                  mapping);
	map_list (cfg_element_name(SCALE),                 cfg->user_scales,                 seq_user_scale,     mapping);
	map_list (cfg_element_name(MODE),                  cfg->user_modes,                  seq_user_mode,      mapping);
	map_list (cfg_element_name(TRANSFORM),             cfg->user_transforms,             seq_user_transform, mapping);
	map_list (cfg_element_name(VRR_OFF),               cfg->adaptive_sync_off_name_desc, seq_str,            mapping);
	map_str  (cfg_element_name(CALLBACK_CMD),          cfg->callback_cmd,                                    mapping);
	map_str  (cfg_element_name(LAPTOP_DISPLAY_PREFIX), cfg->laptop_display_prefix,                           mapping);
	map_enum (cfg_element_name(LOG_THRESHOLD),         cfg->log_threshold,               log_threshold_name, mapping);
	map_list (cfg_element_name(DISABLED),              cfg->disabled,                    seq_disabled,       mapping);

	return true;
}

int write_handler(void *data, unsigned char *buffer, size_t size) {
	if (!data)
		return 0;

	char **yaml = (char**)(data);

	*yaml = calloc(1, size + 1);

	strncpy(*yaml, (char*)buffer, size);

	return 1;
}

static char *yaml_document_to_string(yaml_document_t *document) {
	char *yaml = NULL;

	yaml_emitter_t emitter;

	if (!yaml_emitter_initialize(&emitter)) {
		log_error("unable to marshal cfg: yaml_emitter_initialize failed");
		return NULL;
	}

	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);
	yaml_emitter_set_output(&emitter, write_handler, &yaml);

	if (!yaml_emitter_open(&emitter)) {
		log_error("unable to marshal cfg: yaml_emitter_open failed");
		goto err;
	}

	if (!yaml_emitter_dump(&emitter, document)) {
		log_error("unable to marshal cfg: yaml_emitter_dump failed");
		goto err;
	}

	if (!yaml_emitter_close(&emitter)) {
		log_warn("unable to marshal cfg: yaml_emitter_close failed");
		goto err;
	}

	goto end;

err:

	if (yaml) {
		free(yaml);
		yaml = NULL;
	}

end:
	yaml_emitter_delete(&emitter);

	return yaml;
}

// TODO sync errors with unmarshalling v1 and v2
char *marshal_cfg_2(struct Cfg *cfg) {
	if (!cfg) {
		return NULL;
	}

	char *yaml = NULL;

	yaml_document_t document;
	ctx.document = &document;

	if (!yaml_document_initialize(&document, NULL, NULL, NULL, 1, 1)) {
		log_error("unable to marshal cfg: yaml_document_initialize failed");
		return NULL;
	}

	int root;
	if (!(root = yaml_document_add_mapping(&document, NULL, YAML_BLOCK_MAPPING_STYLE))) {
		log_error("unable to marshal cfg: yaml_document_add_mapping for root failed");
		goto end;
	}

	if (!map_cfg(cfg, root)) {
		goto end;
	}

	yaml = yaml_document_to_string(&document);

end:
	yaml_document_delete(&document);

	return yaml;
}


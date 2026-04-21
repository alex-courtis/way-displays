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
} mc = { 0 };

static char *yaml_document_to_string(yaml_document_t *document) {

	// TODO reallocate on
	size_t BUFFER_SIZE = 1024 * 256;
	char *buffer = calloc(1, BUFFER_SIZE);
	size_t written = 0;

	yaml_emitter_t emitter;

	if (!yaml_emitter_initialize(&emitter)) {
		log_error("TODO err yaml_emitter_initialize");
		goto err;
	}

	// yaml_emitter_set_canonical(&emitter, 1);
	yaml_emitter_set_output_string(&emitter, (unsigned char*)buffer, BUFFER_SIZE, &written);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);
	yaml_emitter_set_indent(&emitter, 2);
	yaml_emitter_set_width(&emitter, -1);
	yaml_emitter_set_unicode(&emitter, 1);

	if (!yaml_emitter_open(&emitter)) {
		log_error("TODO err yaml_emitter_open");
		goto err;
	}

	if (!yaml_emitter_dump(&emitter, document)) {
		log_error("TODO err yaml_emitter_dump");
		goto err;
	}

	if (!yaml_emitter_flush(&emitter)) {
		log_error("TODO err yaml_emitter_flush");
		goto err;
	}

	if (!yaml_emitter_close(&emitter)) {
		log_error("TODO err yaml_emitter_close");
		goto err;
	}

	yaml_emitter_delete(&emitter);

	return buffer;

err:
	if (buffer)
		free(buffer);

	log_fatal("dump fail");

	return NULL;
}

typedef bool (*map_list_fn)(const void *data, int sequence);
static bool map_list(const char *key, const struct SList *list, map_list_fn fn, int mapping) {
	if (!list)
		return true;

	int k = yaml_document_add_scalar(mc.document, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);

	int sequence = yaml_document_add_sequence(mc.document, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	if (!sequence) {
		log_error("TODO fail: map_user_transform_list add_sequence");
		return false;
	}

	for (const struct SList *i = list; i; i = i->nex) {
		fn(i->val, sequence);
	}

	if (!yaml_document_append_mapping_pair(mc.document, mapping, k, sequence)) {
		log_error("TODO fail map_user_transform_list");
		return false;
	}

	return true;
}

static bool map_str(const char *key, const char *val, int mapping) {
	if (!key || !val)
		return false;

	int k = yaml_document_add_scalar(mc.document, (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int v = yaml_document_add_scalar(mc.document, NULL, (yaml_char_t *)val, -1, YAML_PLAIN_SCALAR_STYLE);

	if (!yaml_document_append_mapping_pair(mc.document, mapping, k, v)) {
		log_error("TODO fail: map_str");
		return false;
	}

	return true;
}

static bool seq_str(const void *data, int sequence) {
	const char *str = data;

	if (!str)
		return false;

	int v = yaml_document_add_scalar(mc.document, NULL, (yaml_char_t *)str, -1, YAML_PLAIN_SCALAR_STYLE);

	if (!yaml_document_append_sequence_item(mc.document, sequence, v)) {
		log_error("TODO fail: seq_str");
		return false;
	}

	return true;
}

static bool map_int(const char *key, const int32_t val, int mapping) {
	char val_str[20];
	snprintf(val_str, 20, "%d", val);

	return map_str(key, val_str, mapping);
}

static bool map_float(const char *key, const float val, int mapping) {
	if (val == 0)
		return false;

	char val_str[100];
	snprintf(val_str, 100, "%g", val);

	return map_str(key, val_str, mapping);
}

static bool map_bool(const char *key, const bool val, int mapping) {
	return map_str(key, (val ? "TRUE" : "FALSE"), mapping);
}

typedef const char* (*map_enum_fn_name)(unsigned int val);
static bool map_enum(const char *key, const int val, map_enum_fn_name fn_name, int mapping) {
	const char *val_str = fn_name(val);
	if (!val_str)
		return false;

	// use T/F to obey schema
	if (fn_name == on_off_name)
		val_str = (val == ON) ? "TRUE" : "FALSE";

	return map_str(key, val_str, mapping);
}

static bool seq_user_scale(const void *data, int sequence) {
	const struct UserScale *user_scale = data;

	int map = yaml_document_add_mapping(mc.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	map_str("NAME_DESC", user_scale->name_desc, map);
	map_float("SCALE", user_scale->scale, map);

	if (!yaml_document_append_sequence_item(mc.document, sequence, map)) {
		log_error("TODO cfg_to_yaml_document yaml_document_initialize");
		return false;
	}

	return true;
}

static bool seq_user_mode(const void *data, int sequence) {
	const struct UserMode *user_mode = data;

	int map = yaml_document_add_mapping(mc.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	map_str("NAME_DESC", user_mode->name_desc, map);

	if (user_mode->max) {
		map_bool("MAX", user_mode->max, map);
	} else {
		map_int("WIDTH", user_mode->width, map);
		map_int("HEIGHT", user_mode->height, map);
		if (user_mode->refresh_mhz != -1) {
			map_str("HZ", mhz_to_hz_str(user_mode->refresh_mhz), map);
		}
	}

	if (!yaml_document_append_sequence_item(mc.document, sequence, map)) {
		log_error("TODO cfg_to_yaml_document yaml_document_initialize");
		return false;
	}

	return true;
}

static bool seq_user_transform(const void *data, int sequence) {
	const struct UserTransform *user_transform = data;

	int map = yaml_document_add_mapping(mc.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	map_str("NAME_DESC", user_transform->name_desc, map);
	map_str("TRANSFORM", transform_name(user_transform->transform), map);

	if (!yaml_document_append_sequence_item(mc.document, sequence, map)) {
		log_error("TODO cfg_to_yaml_document yaml_document_initialize");
		return false;
	}

	return true;
}

static bool seq_condition(struct Condition *condition, int sequence) {
	int map = yaml_document_add_mapping(mc.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (condition->plugged)
		map_list("PLUGGED", condition->plugged, seq_str, map);

	if (condition->unplugged)
		map_list("UNPLUGGED", condition->unplugged, seq_str, map);

	if (!yaml_document_append_sequence_item(mc.document, sequence, map)) {
		log_error("TODO cfg_to_yaml_document yaml_document_initialize");
		return false;
	}

	return true;
}

static bool seq_disabled(const void *data, int sequence) {
	const struct Disabled *disabled = data;

	int map = yaml_document_add_mapping(mc.document, NULL, YAML_BLOCK_MAPPING_STYLE);

	int s = yaml_document_add_sequence(mc.document, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (disabled->conditions) {
		map_str("NAME_DESC", disabled->name_desc, map);

		int k = yaml_document_add_scalar(mc.document, NULL, (yaml_char_t *)"IF", -1, YAML_PLAIN_SCALAR_STYLE);

		for (const struct SList *i = disabled->conditions; i; i = i->nex) {
			seq_condition(i->val, s);
		}

		if (!yaml_document_append_mapping_pair(mc.document, map, k, s)) {
			log_error("TODO fail map_user_transform_list");
			return false;
		}

		if (!yaml_document_append_sequence_item(mc.document, sequence, map)) {
			log_error("TODO cfg_to_yaml_document yaml_document_initialize");
			return false;
		}
	} else {
		seq_str(disabled->name_desc, sequence);
	}

	return true;
}

static void map_cfg(const struct Cfg *cfg, int mapping) {
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
}

static bool cfg_to_yaml_document(yaml_document_t *document, const struct Cfg * const cfg) {

	mc.document = document;

	if (!yaml_document_initialize(document, NULL, NULL, NULL, 1, 1)) {
		log_error("TODO cfg_to_yaml_document yaml_document_initialize");
		return false;
	}

	int map;

	if (!(map = yaml_document_add_mapping(document, NULL, YAML_BLOCK_MAPPING_STYLE))) {
		yaml_document_delete(document);
		log_error("TODO cfg_to_yaml_document yaml_document_add_mapping");
		return false;
	}

	map_cfg(cfg, map);

	return true;
}

char *marshal_cfg_2(struct Cfg *cfg) {
	if (!cfg) {
		return NULL;
	}

	yaml_document_t document;

	if (!cfg_to_yaml_document(&document, cfg)) {
		log_error("TODO marshalling cfg request");
		return NULL;
	}


	return yaml_document_to_string(&document);
}


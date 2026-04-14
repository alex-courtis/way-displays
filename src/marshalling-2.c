#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/param.h>
#include <yaml.h>

#include "cfg.h"
#include "convert.h"
#include "stable.h"

// return a static string for the node type
static char* node_type_str(const yaml_node_type_t type) {
	switch (type) {
		case YAML_NO_NODE:
			return "empty";
		case YAML_MAPPING_NODE:
			return "map";
		case YAML_SEQUENCE_NODE:
			return "sequence";
		case YAML_SCALAR_NODE:
			return "scalar";
		default:
			return "???";
	}
}


// TODO replace with check_node_type_
// validate expected is of type actual, returning false and logging a warning if not
static bool check_node_type0(const yaml_node_type_t expected, const yaml_node_type_t actual, const enum CfgElement element, const char *def) {
	if (actual == expected)
		return true;

	if (def)
		log_warn("Ignoring invalid %s: expected %s, got %s, using default %s", cfg_element_name(element), node_type_str(expected), node_type_str(actual), def);
	else
		log_warn("Ignoring invalid %s: expected %s, got %s", cfg_element_name(element), node_type_str(expected), node_type_str(actual));

	return false;
}

// TODO replace
// unmarshal a scalar string to dst, freeing first, caller frees dst
static void unmarshal_string(char **dst, yaml_node_t *scalar, const enum CfgElement element, const char *def) {
	if (*dst)
		free(*dst);

	if (!check_node_type0(YAML_SCALAR_NODE, scalar->type, element, def))
		*dst = def ? strdup(def) : NULL;
	else
		*dst = strdup((char*)scalar->data.scalar.value);
}


// validate expected is of type actual, returning false and logging a warning if not
static bool check_node_type(const yaml_node_type_t expected, const yaml_node_type_t actual, const char *key, enum CfgElement element, const char *name_desc) {
	if (actual == expected)
		return true;

	// TODO add yaml node type to message
	log_warn("Ignoring invalid %s %s %s: expected %s, got %s", cfg_element_name(element), name_desc, key, node_type_str(expected), node_type_str(actual));

	return false;
}

// unmarshal a scalar string to dst, freeing first, caller frees
static bool scalar_to_string(char **dst, yaml_node_t *scalar, const char *key, enum CfgElement element, const char *name_desc) {
	if (!check_node_type(YAML_SCALAR_NODE, scalar->type, key, element, name_desc))
		return false;

	if (*dst)
		free(*dst);

	*dst = strdup((char*)scalar->data.scalar.value);

	return true;
}

// unmarshal a scalar number to dst
static bool scalar_to_int(int32_t *dst, yaml_node_t *scalar, const char *key, enum CfgElement element, const char *name_desc) {
	if (!check_node_type(YAML_SCALAR_NODE, scalar->type, key, element, name_desc))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%d", dst) != 1) {
		log_warn("Ignoring invalid %s %s %s %s", cfg_element_name(element), name_desc, key, scalar->data.scalar.value);
		return false;
	}

	return true;
}

// unmarshal a scalar number to dst
bool scalar_to_float(float *dst, yaml_node_t *scalar, const char *key, enum CfgElement element, const char *name_desc) {
	if (!check_node_type(YAML_SCALAR_NODE, scalar->type, key, element, name_desc))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%f", dst) != 1) {
		log_warn("Ignoring invalid %s %s %s %s", cfg_element_name(element), name_desc, key, scalar->data.scalar.value);
		return false;
	}

	return true;
}

// unmarshal a scalar bool to dst
bool scalar_to_boolean(bool *dst, yaml_node_t *scalar, const char *key, enum CfgElement element, const char *name_desc) {
	if (!check_node_type(YAML_SCALAR_NODE, scalar->type, key, element, name_desc))
		return false;

	// OnOff handles booleans
	int val = on_off_val((char*)scalar->data.scalar.value);
	if (!val) {
		log_warn("Ignoring invalid %s %s %s %s", cfg_element_name(element), name_desc, key, scalar->data.scalar.value);
		return false;
	}

	*dst = val == ON;
	return true;
}

// unmarshal a map of strings to nodes, caller frees
static const struct STable *map_to_table(const yaml_node_t *map_node, yaml_document_t *document, const enum CfgElement element) {

	if (!check_node_type0(YAML_MAPPING_NODE, map_node->type, element, NULL))
		return NULL;

	const struct STable *dst = stable_init(10, 10, false);

	for (yaml_node_pair_t *pair = map_node->data.mapping.pairs.start; pair < map_node->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		yaml_node_t *pair_key = yaml_document_get_node(document, pair->key);
		char *key = NULL;
		unmarshal_string(&key, pair_key, element, NULL);

		yaml_node_t *pair_value = yaml_document_get_node(document, pair->value);

		if (key && pair_value) {
			stable_put(dst, key, pair_value);
		}

		if (key)
			free(key);
	}

	return dst;
}

// unmarshal an enum and return it otherwise return def
typedef unsigned int (*unmarshal_enum_fn_val)(const char *name);
typedef const char* (*unmarshal_enum_fn_name)(unsigned int val);
static int unmarshal_enum(yaml_node_t *scalar, const enum CfgElement element, const int def, unmarshal_enum_fn_val fn_val, unmarshal_enum_fn_name fn_name) {
	if (!check_node_type0(YAML_SCALAR_NODE, scalar->type, element, fn_name(def)))
		return def;

	int val = fn_val((char*)scalar->data.scalar.value);
	if (val)
		return val;

	log_warn("Ignoring invalid %s '%s', using default %s", cfg_element_name(element), scalar->data.scalar.value, fn_name(def));
	return def;
}

// unmarshal a float and return it otherwise return def
static float unmarshal_float(const yaml_node_t *scalar, const enum CfgElement element, const float def, const char *def_str) {
	if (!check_node_type0(YAML_SCALAR_NODE, scalar->type, element, def_str))
		return def;

	float value;
	if (sscanf((char*)scalar->data.scalar.value, "%f", &value) == 1)
		return value;

	log_warn("Ignoring invalid %s '%s', using default %s", cfg_element_name(element), scalar->data.scalar.value, def_str);
	return def;
}

// TODO check regex
// unmarshal a sequence of strings, freeing first, removing duplicates, caller frees
static void seq_to_string_list(struct SList **dst, const yaml_node_t *seq, yaml_document_t *document, const enum CfgElement element) {
	if (*dst)
		slist_free_vals(dst, NULL);

	if (!check_node_type0(YAML_SEQUENCE_NODE, seq->type, element, NULL))
		return;

	const struct STable *map = stable_init(10, 10, false);

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		yaml_node_t *scalar = yaml_document_get_node(document, *item);
		if (!scalar)
			continue;

		char *val = NULL;
		unmarshal_string(&val, scalar, element, NULL);
		if (!val)
			continue;

		stable_put(map, val, NULL);

		free(val);
	}

	*dst = stable_keys_slist(map);

	stable_free(map);
}

// unmarshal ORDER into order_name_desc, freeing first, removing invalid patterns
static void unmarshal_order_name_desc(struct SList **order_name_desc, const yaml_node_t *seq, yaml_document_t *document) {
	seq_to_string_list(order_name_desc, seq, document, ORDER);

	slist_remove_all_free(order_name_desc, cfg_invalid_order_regex, NULL, NULL);
}

// unmarshal DISABLED into disabled, freeing first
static void unmarshal_disabled(struct SList **disabled, const yaml_node_t *seq, yaml_document_t *document) {
	if (*disabled)
		slist_free_vals(disabled, NULL);

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		yaml_node_t *node = yaml_document_get_node(document, *item);
		if (!node)
			continue;

		struct Disabled *d = (struct Disabled*)calloc(1, sizeof(struct Disabled));

		switch (node->type) {
			case YAML_SCALAR_NODE:
				if (scalar_to_string(&d->name_desc, node, "NAME_DESC", DISABLED, "")) {
					slist_append(disabled, d);
					continue;
				}
				break;

			case YAML_MAPPING_NODE:
				log_error("TODO disabled condition");
				break;

			default:
				log_warn("Ignoring invalid DISABLED: expected scalar or map, got %s", node_type_str(node->type));
				break;
		}

		// free on validation fail
		if (d)
			cfg_disabled_free(d);
	}
}

// unmarshal SCALE into user_scales, freeing first
static void unmarshal_scale(struct SList **user_scales, const yaml_node_t *seq, yaml_document_t *document) {
	if (*user_scales)
		slist_free_vals(user_scales, NULL);

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		yaml_node_t *node = yaml_document_get_node(document, *item);
		if (!node)
			continue;

		const struct STable *map = map_to_table(node, document, SCALE);
		if (!map)
			continue;

		struct UserScale *user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));

		if (stable_get(map, "NAME_DESC"))
			if (!scalar_to_string(&user_scale->name_desc, (yaml_node_t*)stable_get(map, "NAME_DESC"), "NAME_DESC", SCALE, ""))
				goto unmarshal_scale_done;

		if (stable_get(map, "SCALE"))
			if (!scalar_to_float(&user_scale->scale, (yaml_node_t*)stable_get(map, "SCALE"), "SCALE", SCALE, user_scale->name_desc))
				goto unmarshal_scale_done;

		// OK
		slist_append(user_scales, user_scale);
		user_scale = NULL;

unmarshal_scale_done:

		stable_free(map);

		// free on validation fail
		if (user_scale)
			cfg_user_scale_free(user_scale);
	}
}

// unmarshal MODE into user_modes, freeing first
static void unmarshal_modes(struct SList **user_modes, const yaml_node_t *seq, yaml_document_t *document) {
	if (*user_modes)
		slist_free_vals(user_modes, NULL);

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		yaml_node_t *node = yaml_document_get_node(document, *item);
		if (!node)
			continue;

		const struct STable *map = map_to_table(node, document, SCALE);
		if (!map)
			continue;

		struct UserMode *user_mode = cfg_user_mode_default();

		if (stable_get(map, "NAME_DESC"))
			if (!scalar_to_string(&user_mode->name_desc, (yaml_node_t*)stable_get(map, "NAME_DESC"), "NAME_DESC", MODE, ""))
				goto unmarshal_mode_done;
		// TODO missing and regex

		if (stable_get(map, "WIDTH"))
			if (!scalar_to_int(&user_mode->width, (yaml_node_t*)stable_get(map, "WIDTH"), "WIDTH", MODE, user_mode->name_desc))
				goto unmarshal_mode_done;

		if (stable_get(map, "HEIGHT"))
			if (!scalar_to_int(&user_mode->height, (yaml_node_t*)stable_get(map, "HEIGHT"), "HEIGHT", MODE, user_mode->name_desc))
				goto unmarshal_mode_done;

		if (stable_get(map, "HZ")) {
			float hz;
			if (!scalar_to_float(&hz, (yaml_node_t*)stable_get(map, "HZ"), "HZ", MODE, user_mode->name_desc))
				goto unmarshal_mode_done;

			user_mode->refresh_mhz = lround(hz * 1000);
		}

		if (stable_get(map, "MAX")) {
			if (!scalar_to_boolean(&user_mode->max, (yaml_node_t*)stable_get(map, "MAX"), "MAX", MODE, user_mode->name_desc))
				goto unmarshal_mode_done;
		}

		// OK
		slist_append(user_modes, user_mode);
		user_mode = NULL;

unmarshal_mode_done:

		stable_free(map);

		// free on validation fail
		if (user_mode)
			cfg_user_mode_free(user_mode);
	}
}

bool unmarshal_cfg(struct Cfg *cfg, yaml_document_t *document) {

	yaml_node_t *start_doc = document->nodes.start;
	if (start_doc->type != YAML_MAPPING_NODE) {
		// TODO error
		return false;
	}

	for (yaml_node_pair_t *pair = start_doc->data.mapping.pairs.start; pair < start_doc->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		yaml_node_t *key = yaml_document_get_node(document, pair->key);
		if (!key || key->type != YAML_SCALAR_NODE || !key->data.scalar.value)
			continue;

		yaml_node_t *value = yaml_document_get_node(document, pair->value);
		if (!value)
			continue;

		switch (cfg_element_val((char*)key->data.scalar.value)) {
			case ARRANGE:
				cfg->arrange = unmarshal_enum(value, ARRANGE, ARRANGE_DEFAULT, arrange_val_start, arrange_name);
				break;
			case ALIGN:
				cfg->align = unmarshal_enum(value, ALIGN, ALIGN_DEFAULT, align_val_start, align_name);
				break;
			case ORDER:
				unmarshal_order_name_desc(&cfg->order_name_desc, value, document);
				break;
			case SCALING:
				cfg->scaling = unmarshal_enum(value, SCALING, SCALING_DEFAULT, on_off_val, on_off_name);
				break;
			case AUTO_SCALE:
				cfg->auto_scale = unmarshal_enum(value, AUTO_SCALE, AUTO_SCALE_DEFAULT, on_off_val, on_off_name);
				break;
			case SCALE:
				unmarshal_scale(&cfg->user_scales, value, document);
				break;
			case MODE:
				unmarshal_modes(&cfg->user_modes, value, document);
				break;
			case TRANSFORM:
				break;
			case VRR_OFF:
				seq_to_string_list(&cfg->adaptive_sync_off_name_desc, value, document, VRR_OFF);
				break;
			case CALLBACK_CMD:
				unmarshal_string(&cfg->callback_cmd, value, CALLBACK_CMD, CALLBACK_CMD_DEFAULT);
				break;
			case LAPTOP_DISPLAY_PREFIX:
				unmarshal_string(&cfg->laptop_display_prefix, value, LAPTOP_DISPLAY_PREFIX, NULL);
				break;
			case MAX_PREFERRED_REFRESH:
				break;
			case LOG_THRESHOLD:
				cfg->log_threshold = unmarshal_enum(value, LOG_THRESHOLD, LOG_THRESHOLD_DEFAULT, log_threshold_val, log_threshold_name);
				break;
			case DISABLED:
				unmarshal_disabled(&cfg->disabled, value, document);
				break;
			case ARRANGE_ALIGN:
				break;
			case AUTO_SCALE_MIN:
				cfg->auto_scale_min = unmarshal_float(value, AUTO_SCALE_MIN, AUTO_SCALE_MIN_DEFAULT, AUTO_SCALE_MIN_DEFAULT_STR);
				break;
			case AUTO_SCALE_MAX:
				cfg->auto_scale_max = unmarshal_float(value, AUTO_SCALE_MAX, AUTO_SCALE_MAX_DEFAULT, AUTO_SCALE_MAX_DEFAULT_STR);
				break;
			default:
				// TODO maybe unknown key warning
				break;
		}
	}

	return true;
}

bool unmarshal_cfg_from_file_2(struct Cfg *cfg) {
	if (!cfg->file_path) {
		return false;
	}

	// TODO deal with the unsigned char

	yaml_parser_t parser;
	yaml_document_t document;

	// TODO validate
	FILE *input = fopen(cfg->file_path, "rb");

	if (!yaml_parser_initialize(&parser)) {

		// TODO all error handling and destruction
		fprintf(stderr, "Could not initialize the parser object\n");
		return 1;
	}

	yaml_parser_set_input_file(&parser, input);

	yaml_parser_load(&parser, &document);

	yaml_document_get_root_node(&document);

	unmarshal_cfg(cfg, &document);

	yaml_document_delete(&document);

	yaml_parser_delete(&parser);

	fclose(input);

	return true;
}


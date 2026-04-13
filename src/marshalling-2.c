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

// validate expected is of type actual, returning false and logging a warning if not
static bool check_node_type(const yaml_node_type_t expected, const yaml_node_type_t actual, const char *key, const char *def) {
	if (actual == expected)
		return true;

	if (def)
		log_warn("Ignoring invalid %s: expected %s, got %s, using default %s", key, node_type_str(expected), node_type_str(actual), def);
	else
		log_warn("Ignoring invalid %s: expected %s, got %s", key, node_type_str(expected), node_type_str(actual));

	return false;
}

// unmarshal an enum and return it otherwise return def
typedef unsigned int (*unmarshal_enum_fn_val)(const char *name);
typedef const char* (*unmarshal_enum_fn_name)(unsigned int val);
static int unmarshal_enum(yaml_node_t *scalar, const enum CfgElement element, const int def, unmarshal_enum_fn_val fn_val, unmarshal_enum_fn_name fn_name) {
	if (!check_node_type(YAML_SCALAR_NODE, scalar->type, cfg_element_name(element), fn_name(def)))
		return def;

	int val = fn_val((char*)scalar->data.scalar.value);
	if (val)
		return val;

	log_warn("Ignoring invalid %s '%s', using default %s", cfg_element_name(element), scalar->data.scalar.value, fn_name(def));
	return def;
}

// unmarshal a float and return it otherwise return def
static float unmarshal_float(const yaml_node_t *scalar, const enum CfgElement element, const float def, const char *def_str) {
	if (!check_node_type(YAML_SCALAR_NODE, scalar->type, cfg_element_name(element), AUTO_SCALE_MAX_DEFAULT_STR))
		return def;

	float value;
	if (sscanf((char*)scalar->data.scalar.value, "%f", &value) == 1)
		return value;

	log_warn("Ignoring invalid %s '%s', using default %s", cfg_element_name(element), scalar->data.scalar.value, AUTO_SCALE_MAX_DEFAULT_STR);
	return def;
}

// unmarshal a scalar string to dst, freeing first, caller frees dst
static void unmarshal_string(char **dst, yaml_node_t *scalar, const enum CfgElement element, const char *def) {
	if (*dst)
		free(*dst);

	if (!check_node_type(YAML_SCALAR_NODE, scalar->type, cfg_element_name(element), def))
		*dst = def ? strdup(def) : NULL;
	else
		*dst = strdup((char*)scalar->data.scalar.value);
}

// unmarshal a sequence of strings, freeing first, removing duplicates, caller frees
static void unmarshal_string_list(struct SList **dst, const yaml_node_t *seq, yaml_document_t *document, const enum CfgElement element) {
	if (*dst)
		slist_free_vals(dst, NULL);

	if (!check_node_type(YAML_SEQUENCE_NODE, seq->type, cfg_element_name(element), NULL))
		return;

	const struct STable *map = stable_init(10, 10, false);

	yaml_node_item_t *item;
	yaml_node_t *scalar;

	for (item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		if (!(scalar = yaml_document_get_node(document, *item)))
			continue;

		char *val = NULL;
		unmarshal_string(&val, scalar, ORDER, NULL);
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
	unmarshal_string_list(order_name_desc, seq, document, ORDER);

	slist_remove_all_free(order_name_desc, cfg_invalid_order_regex, NULL, NULL);
}

bool unmarshal_cfg(struct Cfg *cfg, yaml_document_t *document) {

	yaml_node_t *start_doc = document->nodes.start;
	if (start_doc->type != YAML_MAPPING_NODE) {
		// TODO error
		return false;
	}

	yaml_node_pair_t *pair;
	yaml_node_t *key;
	yaml_node_t *value;

	for (pair = start_doc->data.mapping.pairs.start; pair < start_doc->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		if (!(key = yaml_document_get_node(document, pair->key)) || key->type != YAML_SCALAR_NODE || !key->data.scalar.value)
			continue;

		if (!(value = yaml_document_get_node(document, pair->value)))
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
				break;
			case MODE:
				break;
			case TRANSFORM:
				break;
			case VRR_OFF:
				unmarshal_string_list(&cfg->adaptive_sync_off_name_desc, value, document, VRR_OFF);
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


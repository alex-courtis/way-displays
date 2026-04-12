#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/param.h>
#include <yaml.h>

#include "cfg.h"
#include "convert.h"

static char* node_type(yaml_node_type_t type) {
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

static bool check_node_type(const yaml_node_type_t expected, const yaml_node_type_t actual, const char *key, const char *def) {
	if (actual == expected)
		return true;

	log_warn("Ignoring invalid %s: expected %s, got %s, using default %s", key, node_type(expected), node_type(actual), def);

	return false;
}

static enum Arrange unmarshal_arrange(yaml_node_t *node) {
	if (!check_node_type(YAML_SCALAR_NODE, node->type, cfg_element_name(ARRANGE), arrange_name(ARRANGE_DEFAULT)))
		return ARRANGE_DEFAULT;

	enum Arrange arrange = arrange_val_start((char*)node->data.scalar.value);
	if (arrange)
		return arrange;

	log_warn("Ignoring invalid %s '%s', using default %s", cfg_element_name(ARRANGE), node->data.scalar.value, arrange_name(ARRANGE_DEFAULT));
	return ARRANGE_DEFAULT;
}

static enum Align unmarshal_align(yaml_node_t *node) {
	if (!check_node_type(YAML_SCALAR_NODE, node->type, cfg_element_name(ALIGN), align_name(ALIGN_DEFAULT)))
		return ALIGN_DEFAULT;

	enum Align align = align_val_start((char*)node->data.scalar.value);
	if (align)
		return align;

	log_warn("Ignoring invalid ALIGN '%s', using default %s", node->data.scalar.value, align_name(ALIGN_DEFAULT));
	return ALIGN_DEFAULT;
}

static enum LogThreshold unmarshal_log_threshold(yaml_node_t *node) {
	if (!check_node_type(YAML_SCALAR_NODE, node->type, cfg_element_name(LOG_THRESHOLD), log_threshold_name(LOG_THRESHOLD_DEFAULT)))
		return LOG_THRESHOLD_DEFAULT;

	enum LogThreshold log_threshold = log_threshold_val((char*)node->data.scalar.value);
	if (log_threshold)
		return log_threshold;

	log_warn("Ignoring invalid ALIGN '%s', using default %s", node->data.scalar.value, log_threshold_name(LOG_THRESHOLD_DEFAULT));
	return LOG_THRESHOLD_DEFAULT;
}

static enum OnOff unmarshal_on_off(yaml_node_t *node, const enum CfgElement element, const enum OnOff def) {
	if (!check_node_type(YAML_SCALAR_NODE, node->type, cfg_element_name(element), on_off_name(def)))
		return def;

	enum OnOff value = on_off_val((char*)node->data.scalar.value);
	if (value)
		return value;

	log_warn("Ignoring invalid %s '%s', using default %s", cfg_element_name(element), node->data.scalar.value, on_off_name(def));
	return def;
}

static void unmarshal_string(yaml_node_t *node, char **dst, const enum CfgElement element, const char *def) {
	if (*dst)
		free(*dst);

	if (!check_node_type(YAML_SCALAR_NODE, node->type, cfg_element_name(element), def))
		*dst = def ? strdup(def) : NULL;
	else
		*dst = strdup((char*)node->data.scalar.value);
}

static float unmarshal_float(const yaml_node_t *node, const enum CfgElement element, const float def, const char *def_str) {
	if (!check_node_type(YAML_SCALAR_NODE, node->type, cfg_element_name(element), AUTO_SCALE_MAX_DEFAULT_STR))
		return def;

	float value;
	if (sscanf((char*)node->data.scalar.value, "%f", &value) == 1)
		return value;

	log_warn("Ignoring invalid %s '%s', using default %s", cfg_element_name(element), node->data.scalar.value, AUTO_SCALE_MAX_DEFAULT_STR);
	return def;
}

bool unmarshal_cfg(struct Cfg *cfg, yaml_document_t *document) {

	yaml_node_t *start_doc = document->nodes.start;
	if (start_doc->type != YAML_MAPPING_NODE) {
		// TODO error
		return false;
	}

	yaml_node_pair_t *pair;
	for (pair = start_doc->data.mapping.pairs.start;
			pair < start_doc->data.mapping.pairs.top; pair ++) {

		yaml_node_t *key = yaml_document_get_node(document, pair->key);

		if (key->type == YAML_SCALAR_NODE && key->data.scalar.value) {
			yaml_node_t *val = yaml_document_get_node(document, pair->value);

			enum CfgElement element = cfg_element_val((char*)key->data.scalar.value);
			switch (element) {
				case ARRANGE:
					cfg->arrange = unmarshal_arrange(val);
					break;
				case ALIGN:
					cfg->align = unmarshal_align(val);
					break;
				case ORDER:
					break;
				case SCALING:
					cfg->scaling = unmarshal_on_off(val, SCALING, SCALING_DEFAULT);
					break;
				case AUTO_SCALE:
					cfg->auto_scale = unmarshal_on_off(val, AUTO_SCALE, AUTO_SCALE_DEFAULT);
					break;
				case SCALE:
				case MODE:
				case TRANSFORM:
				case VRR_OFF:
					break;
				case CALLBACK_CMD:
					unmarshal_string(val, &cfg->callback_cmd, CALLBACK_CMD, CALLBACK_CMD_DEFAULT);
					break;
				case LAPTOP_DISPLAY_PREFIX:
					unmarshal_string(val, &cfg->laptop_display_prefix, LAPTOP_DISPLAY_PREFIX, NULL);
					break;
				case MAX_PREFERRED_REFRESH:
					break;
				case LOG_THRESHOLD:
					cfg->log_threshold = unmarshal_log_threshold(val);
					break;
				case DISABLED:
				case ARRANGE_ALIGN:
					break;
				case AUTO_SCALE_MIN:
					cfg->auto_scale_min = unmarshal_float(val, AUTO_SCALE_MIN, AUTO_SCALE_MIN_DEFAULT, AUTO_SCALE_MIN_DEFAULT_STR);
					break;
				case AUTO_SCALE_MAX:
					cfg->auto_scale_max = unmarshal_float(val, AUTO_SCALE_MAX, AUTO_SCALE_MAX_DEFAULT, AUTO_SCALE_MAX_DEFAULT_STR);
					break;
				default:
					break;
			}
		}
	}

	return true;
}

bool unmarshal_cfg_from_file_2(struct Cfg *cfg) {
	if (!cfg->file_path) {
		return false;
	}

	yaml_parser_t parser;
	yaml_document_t document;

	// TODO validate
	FILE *input = fopen(cfg->file_path, "rb");

	if (!yaml_parser_initialize(&parser)) {
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


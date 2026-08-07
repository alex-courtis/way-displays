#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml/unmarshal-primitives.h"

#include "enum.h"
#include "spmap.h"
#include "yaml/unmarshal.h"

char *yaml_scalar_to_string(const yaml_node_t *scalar) {
	if (!yaml_check_is_scalar(scalar, "string"))
		return NULL;

	return(strdup((char*)scalar->data.scalar.value));
}

char *yaml_scalar_to_string_def(const char *def, const yaml_node_t *scalar) {
	yaml_unmarshal_log_def(def);

	char *str = yaml_scalar_to_string(scalar);

	if (!str)
		str = strdup(def);

	yaml_unmarshal_log_def(NULL);

	return str;
}

bool yaml_scalar_to_int(int32_t *dst, const yaml_node_t *scalar) {
	if (!yaml_check_is_scalar(scalar, "integer"))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%d", dst) == 1)
		return true;

	yaml_log_invalid_value(scalar->data.scalar.value, "integer");
	return false;
}

bool yaml_scalar_to_int_def(int32_t *dst, int32_t def, const yaml_node_t *scalar) {
	bool ok = true;

	char def_str[10];
	snprintf(def_str, sizeof(def_str) - 1, "%d", def);

	yaml_unmarshal_log_def(def_str);

	if (!(ok = yaml_scalar_to_int(dst, scalar)))
		*dst = def;

	yaml_unmarshal_log_def(NULL);

	return ok;
}

bool yaml_scalar_to_float(float *dst, const yaml_node_t *scalar) {
	if (!yaml_check_is_scalar(scalar, "number"))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%f", dst) == 1)
		return true;

	yaml_log_invalid_value(scalar->data.scalar.value, "number");
	return false;
}

bool yaml_scalar_to_float_def(float *dst, float def, const yaml_node_t *scalar) {
	bool ok = true;

	char def_str[10];
	snprintf(def_str, sizeof(def_str) - 1, "%.1f", def);

	yaml_unmarshal_log_def(def_str);

	if (!(ok = yaml_scalar_to_float(dst, scalar)))
		*dst = def;

	yaml_unmarshal_log_def(NULL);

	return ok;
}

int yaml_scalar_to_enum(const yaml_node_t *scalar, fn_enum_val val, fn_enum_names names) {
	yaml_unmarshal_log_enum_names(names);

	int ret = 0;

	if (yaml_check_is_scalar(scalar, "enum")) {
		ret = val((char*)scalar->data.scalar.value);
		if (!ret) {
			yaml_log_invalid_value(scalar->data.scalar.value, "enum");
		}
	}

	yaml_unmarshal_log_enum_names(NULL);

	return ret;
}

int yaml_scalar_to_enum_def(const int def, const yaml_node_t *scalar, fn_enum_val val, fn_enum_name name, fn_enum_names names) {
	yaml_unmarshal_log_def(name(def));

	int ret = yaml_scalar_to_enum(scalar, val, names);
	if (!ret)
		ret = def;

	yaml_unmarshal_log_def(NULL);

	return ret;
}

bool yaml_scalar_to_boolean(bool *dst, const yaml_node_t *scalar) {
	if (!yaml_check_is_scalar(scalar, "boolean")) {
		return false;
	}

	int val = on_off_val((char*)scalar->data.scalar.value);
	if (!val) {
		yaml_log_invalid_value(scalar->data.scalar.value, "boolean");
		return false;
	}

	*dst = val == ON;
	return true;
}

int yaml_scalar_to_on_off_def(const enum OnOff def, const yaml_node_t *scalar) {
	yaml_unmarshal_log_def(def == ON ? "true" : "false");

	int ret = def;

	bool val;
	if (yaml_scalar_to_boolean(&val, scalar)) {
		ret = val ? ON : OFF;
	}

	yaml_unmarshal_log_def(NULL);

	return ret;
}

bool yaml_seq_into_col(const yaml_node_t *seq, const void *col, fn_yaml_node_into_col fn) {
	if (!yaml_check_node_type(seq, YAML_SEQUENCE_NODE, 0) || !col)
		return false;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(&uc.d, *item);
		if (!node)
			continue;

		fn(col, node);
	}

	return true;
}

const struct SPmap *yaml_map_to_spmap(const yaml_node_t *map) {
	if (!yaml_check_node_type(map, YAML_MAPPING_NODE, 0))
		return NULL;

	const struct SPmap *nodes = spmap_init();

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *pair_key = yaml_document_get_node(&uc.d, pair->key);
		if (!pair_key)
			continue;

		char *key = yaml_scalar_to_string(pair_key);
		if (!key)
			continue;

		const yaml_node_t *pair_value = yaml_document_get_node(&uc.d, pair->value);

		if (pair_value)
			spmap_put(nodes, key, pair_value);

		free(key);
	}

	return nodes;
}

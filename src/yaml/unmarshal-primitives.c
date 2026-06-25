#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml/unmarshal-primitives.h"

#include "cfg.h"
#include "convert.h"
#include "smap.h"
#include "yaml/unmarshal.h"

char *yaml_scalar_to_string(struct UC *c, const yaml_node_t *scalar) {
	if (!yaml_check_node_type(c, scalar, YAML_SCALAR_NODE))
		return NULL;

	return(strdup((char*)scalar->data.scalar.value));
}

char *yaml_scalar_to_string_def(struct UC *c, const char *def, const yaml_node_t *scalar) {
	yaml_unmarshal_log_def(c, def);

	char *str = yaml_scalar_to_string(c, scalar);

	if (!str)
		str = strdup(def);

	yaml_unmarshal_log_def(c, NULL);

	return str;
}

bool yaml_scalar_to_int(struct UC *c, int32_t *dst, const yaml_node_t *scalar) {
	if (!yaml_check_node_type(c, scalar, YAML_SCALAR_NODE))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%d", dst) == 1)
		return true;

	yaml_unmarshal_log_invalid_value(c, scalar->data.scalar.value);
	return false;
}

bool yaml_scalar_to_int_def(struct UC *c, int32_t *dst, int32_t def, const yaml_node_t *scalar) {
	bool ok = true;

	char def_str[10];
	snprintf(def_str, sizeof(def_str) - 1, "%d", def);

	yaml_unmarshal_log_def(c, def_str);

	if (!(ok = yaml_scalar_to_int(c, dst, scalar)))
		*dst = def;

	yaml_unmarshal_log_def(c, NULL);

	return ok;
}

bool yaml_scalar_to_float(struct UC *c, float *dst, const yaml_node_t *scalar) {
	if (!yaml_check_node_type(c, scalar, YAML_SCALAR_NODE))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%f", dst) == 1)
		return true;

	yaml_unmarshal_log_invalid_value(c, scalar->data.scalar.value);
	return false;
}

bool yaml_scalar_to_float_def(struct UC *c, float *dst, float def, const yaml_node_t *scalar) {
	bool ok = true;

	char def_str[10];
	snprintf(def_str, sizeof(def_str) - 1, "%.1f", def);

	yaml_unmarshal_log_def(c, def_str);

	if (!(ok = yaml_scalar_to_float(c, dst, scalar)))
		*dst = def;

	yaml_unmarshal_log_def(c, NULL);

	return ok;
}

// cppcheck-suppress funcArgNamesDifferentUnnamed
int yaml_scalar_to_enum(struct UC *c, const yaml_node_t *scalar, fn_enum_val val, fn_enum_names names) {
	yaml_unmarshal_log_enum_names(c, names);

	int ret = 0;

	if (yaml_check_node_type(c, scalar, YAML_SCALAR_NODE)) {
		ret = val((char*)scalar->data.scalar.value);
		if (!ret) {
			yaml_unmarshal_log_invalid_value(c, scalar->data.scalar.value);
		}
	}

	yaml_unmarshal_log_enum_names(c, NULL);

	return ret;
}

// cppcheck-suppress funcArgNamesDifferentUnnamed
int yaml_scalar_to_enum_def(struct UC *c, const int def, const yaml_node_t *scalar, fn_enum_val val, fn_enum_name name, fn_enum_names names) {
	yaml_unmarshal_log_def(c, name(def));

	int ret = yaml_scalar_to_enum(c, scalar, val, names);
	if (!ret)
		ret = def;

	yaml_unmarshal_log_def(c, NULL);

	return ret;
}

bool yaml_scalar_to_boolean(struct UC *c, bool *dst, const yaml_node_t *scalar) {
	int val = yaml_scalar_to_enum(c, scalar, on_off_val, on_off_names);

	if (val) {
		*dst = val == ON;
		return true;
	}

	return false;
}

bool yaml_seq_into_col(struct UC *c, const yaml_node_t *seq, const void *col, fn_yaml_node_into_col fn) {
	if (!yaml_check_node_type(c, seq, YAML_SEQUENCE_NODE) || !col)
		return false;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(&c->d, *item);
		if (!node)
			continue;

		fn(c, col, node);
	}

	return true;
}

const struct SMap *yaml_map_to_node_table(struct UC *c, const yaml_node_t *map) {
	if (!yaml_check_node_type(c, map, YAML_MAPPING_NODE))
		return NULL;

	const struct SMap *table = smap_init();

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *pair_key = yaml_document_get_node(&c->d, pair->key);

		char *key = NULL;
		if (!(key = yaml_scalar_to_string(c, pair_key))) {
			smap_free(table);
			return NULL;
		}

		const yaml_node_t *pair_value = yaml_document_get_node(&c->d, pair->value);

		if (key && pair_value)
			smap_put(table, key, pair_value);

		if (key)
			free(key);
	}

	return table;
}

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml/unmarshal-primitives.h"

#include "cfg.h"
#include "convert.h"
#include "slist.h"
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

int yaml_scalar_to_enum(struct UC *c, const yaml_node_t *scalar, enum_val_fn val_fn, enum_names_fn names_fn) {
	yaml_unmarshal_log_valid_values_fn(c, names_fn);

	int ret = 0;

	if (yaml_check_node_type(c, scalar, YAML_SCALAR_NODE)) {
		ret = val_fn((char*)scalar->data.scalar.value);
		if (!ret) {
			yaml_unmarshal_log_invalid_value(c, scalar->data.scalar.value);
		}
	}

	yaml_unmarshal_log_valid_values_fn(c, NULL);

	return ret;
}

int yaml_scalar_to_enum_def(struct UC *c, const int def, const yaml_node_t *scalar, enum_val_fn val_fn, enum_name_fn name_fn, enum_names_fn names_fn) {
	yaml_unmarshal_log_def(c, name_fn(def));

	int ret = yaml_scalar_to_enum(c, scalar, val_fn, names_fn);
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

struct SList *yaml_seq_to_type_list(struct UC *c, const yaml_node_t *seq, yaml_node_to_type_fn fn) {
	if (!yaml_check_node_type(c, seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	void *val = NULL;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(&c->d, *item);
		if (!node)
			continue;

		if ((val = fn(c, node)))
			slist_append(&list, val);
	}

	return list;
}

bool yaml_seq_into_smap(struct UC *c, const yaml_node_t *seq, const struct SMap *smap, yaml_node_into_smap_fn fn) {
	if (!yaml_check_node_type(c, seq, YAML_SEQUENCE_NODE) || !smap)
		return false;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(&c->d, *item);
		if (!node)
			continue;

		fn(c, smap, node);
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

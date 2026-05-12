#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml-unmarshal-context.h"
#include "yaml-unmarshal-primitives.h"

#include "cfg.h"
#include "convert.h"
#include "stable.h"

// TODO move to yaml directory

const struct STable *yaml_map_to_node_table(const yaml_node_t *map) {
	if (!yaml_check_node_type(map, YAML_MAPPING_NODE))
		return NULL;

	const struct STable *table = stable_init(10, 10, false);

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *pair_key = yaml_document_get_node(ctx.document, pair->key);

		char *key = NULL;
		if (!(key = yaml_scalar_to_string(pair_key))) {
			stable_free(table);
			return NULL;
		}

		const yaml_node_t *pair_value = yaml_document_get_node(ctx.document, pair->value);

		if (key && pair_value)
			stable_put(table, key, pair_value);

		if (key)
			free(key);
	}

	return table;
}

char *yaml_scalar_to_string(const yaml_node_t *scalar) {
	if (!yaml_check_node_type(scalar, YAML_SCALAR_NODE))
		return NULL;

	return(strdup((char*)scalar->data.scalar.value));
}

bool yaml_scalar_to_int(int32_t *dst, const yaml_node_t *scalar) {
	if (!yaml_check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%d", dst) == 1)
		return true;

	yaml_log_invalid_value(scalar->data.scalar.value);
	return false;
}

bool yaml_scalar_to_float(float *dst, const yaml_node_t *scalar) {
	if (!yaml_check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%f", dst) == 1)
		return true;

	yaml_log_invalid_value(scalar->data.scalar.value);
	return false;
}

bool yaml_scalar_to_float_def(float *dst, float def, const yaml_node_t *scalar) {
	bool ok = true;

	char def_str[10];
	snprintf(def_str, 10, "%.1f", def);

	yaml_log_ctx_def(def_str);

	if (!(ok = yaml_scalar_to_float(dst, scalar)))
		*dst = def;

	yaml_log_ctx_def(NULL);

	return ok;
}

int yaml_scalar_to_enum(const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val) {
	if (!yaml_check_node_type(scalar, YAML_SCALAR_NODE))
		return 0;

	int val = fn_val((char*)scalar->data.scalar.value);
	if (val)
		return val;

	yaml_log_invalid_value(scalar->data.scalar.value);
	return 0;
}

int yaml_scalar_to_enum_def(const int def, const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val, scalar_to_enum_fn_name fn_name) {
	yaml_log_ctx_def(fn_name(def));

	int ret = yaml_scalar_to_enum(scalar, fn_val);
	if (!ret)
		ret = def;

	yaml_log_ctx_def(NULL);

	return ret;
}

bool yaml_scalar_to_boolean(bool *dst, const yaml_node_t *scalar) {
	int val = yaml_scalar_to_enum(scalar, on_off_val);

	if (val) {
		*dst = val == ON;
		return true;
	}

	return false;
}

struct SList *seq_to_type_list(const yaml_node_t *seq, node_to_type_fn fn) {
	if (!yaml_check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	void *val = NULL;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		if ((val = fn(node)))
			slist_append(&list, val);
	}

	return list;
}

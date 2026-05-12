#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>

#include "yaml/marshal-primitives.h"

#include "cfg.h"
#include "convert.h"
#include "slist.h"
#include "yaml-marshal.h"

bool yaml_map_add_str(const char *key, const char *str, int mapping) {
	if (!key || !str || !mapping)
		return false;

	int k = yaml_document_add_scalar(marshal_ctx.doc, (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int v = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)str, -1, YAML_PLAIN_SCALAR_STYLE);

	return k && v && yaml_document_append_mapping_pair(marshal_ctx.doc, mapping, k, v);
}

bool yaml_map_add_int(const char *key, const int32_t val, int mapping) {
	if (!key || !mapping)
		return false;

	char str[20];
	snprintf(str, 20, "%d", val);

	return yaml_map_add_str(key, str, mapping);
}

bool yaml_map_add_float(const char *key, const float val, int mapping) {
	if (!key || val == 0 || !mapping)
		return false;

	char str[100];
	snprintf(str, 100, "%g", val);

	return yaml_map_add_str(key, str, mapping);
}

bool yaml_map_add_bool(const char *key, const bool val, int mapping) {
	if (!key || !mapping)
		return false;

	return yaml_map_add_str(key, (val ? "TRUE" : "FALSE"), mapping);
}

bool yaml_map_add_enum(const char *key, const int val, enum_name_fn fn_name, int mapping) {
	if (!key || !fn_name || !mapping)
		return false;

	const char *str = fn_name(val);
	if (!str)
		return false;

	// use T/F to obey schema
	if (fn_name == on_off_name)
		str = (val == ON) ? "TRUE" : "FALSE";

	return yaml_map_add_str(key, str, mapping);
}

bool yaml_seq_append_str(const void *str, int sequence) {
	if (!str || !sequence)
		return false;

	int scalar = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)str, -1, YAML_PLAIN_SCALAR_STYLE);

	return scalar && yaml_document_append_sequence_item(marshal_ctx.doc, sequence, scalar);
}

bool yaml_map_add_map(const char *key, const void *data, yaml_map_populate_fn fn, int mapping) {
	if (!key || !fn || !mapping)
		return false;

	int k = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (!k || !map)
		return false;

	fn(data, map);

	return yaml_document_append_mapping_pair(marshal_ctx.doc, mapping, k, map);
}

bool yaml_map_add_seq(const char *key, const struct SList *list, yaml_seq_append_fn fn, int mapping) {
	if (!key || !list || !fn || !mapping)
		return false;

	int k = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(marshal_ctx.doc, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return false;

	for (const struct SList *i = list; i; i = i->nex)
		fn(i->val, seq);

	return yaml_document_append_mapping_pair(marshal_ctx.doc, mapping, k, seq);
}

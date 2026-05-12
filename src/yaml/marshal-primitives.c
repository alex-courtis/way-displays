#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>

#include "yaml/marshal-primitives.h"

#include "cfg.h"
#include "convert.h"
#include "slist.h"
#include "yaml-marshal.h"

bool yaml_map_add_str(const char *k, const char *v, int mapping) {
	if (!k || !v || !mapping)
		return false;

	int key = yaml_document_add_scalar(marshal_ctx.doc, (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG, (yaml_char_t *)k, -1, YAML_PLAIN_SCALAR_STYLE);
	int scalar = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)v, -1, YAML_PLAIN_SCALAR_STYLE);

	return key && scalar && yaml_document_append_mapping_pair(marshal_ctx.doc, mapping, key, scalar);
}

bool yaml_map_add_int(const char *k, const int32_t v, int mapping) {
	if (!k || !mapping)
		return false;

	char v_str[20];
	snprintf(v_str, 20, "%d", v);

	return yaml_map_add_str(k, v_str, mapping);
}

bool yaml_map_add_float(const char *k, const float v, int mapping) {
	if (!k || v == 0 || !mapping)
		return false;

	char v_str[100];
	snprintf(v_str, 100, "%g", v);

	return yaml_map_add_str(k, v_str, mapping);
}

bool yaml_map_add_bool(const char *k, const bool v, int mapping) {
	if (!k || !mapping)
		return false;

	return yaml_map_add_str(k, (v ? "TRUE" : "FALSE"), mapping);
}

bool yaml_map_add_enum(const char *k, const int v, enum_name_fn fn_name, int mapping) {
	if (!k || !fn_name || !mapping)
		return false;

	const char *v_str = fn_name(v);
	if (!v_str)
		return false;

	// use T/F to obey schema
	if (fn_name == on_off_name)
		v_str = (v == ON) ? "TRUE" : "FALSE";

	return yaml_map_add_str(k, v_str, mapping);
}

bool yaml_seq_append_str(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	int scalar = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)data, -1, YAML_PLAIN_SCALAR_STYLE);

	return scalar && yaml_document_append_sequence_item(marshal_ctx.doc, sequence, scalar);
}

bool yaml_map_add_map(const char *k, const void *data, yaml_map_add_fn fn, int mapping) {
	if (!k || !fn || !mapping)
		return false;

	int key = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)k, -1, YAML_PLAIN_SCALAR_STYLE);
	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (!key || !map)
		return false;

	fn(data, map);

	return yaml_document_append_mapping_pair(marshal_ctx.doc, mapping, key, map);
}

bool yaml_map_add_seq(const char *k, const struct SList *list, yaml_seq_append_fn fn, int mapping) {
	if (!k || !list || !fn || !mapping)
		return false;

	int key = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)k, -1, YAML_PLAIN_SCALAR_STYLE);
	int sequence = yaml_document_add_sequence(marshal_ctx.doc, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!key || !sequence)
		return false;

	for (const struct SList *i = list; i; i = i->nex)
		fn(i->val, sequence);

	return yaml_document_append_mapping_pair(marshal_ctx.doc, mapping, key, sequence);
}

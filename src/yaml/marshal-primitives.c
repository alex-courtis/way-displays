#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <yaml.h>

#include "yaml/marshal-primitives.h"

#include "enum.h"
#include "plist.h"
#include "sset.h"
#include "yaml/marshal.h"

void yaml_map_add_node(const char *key, int node, int mapping) {
	if (!key || !mapping || !node)
		return;

	int k = yaml_document_add_scalar(&mc.d, (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	if (k)
		yaml_document_append_mapping_pair(&mc.d, mapping, k, node);
}

void yaml_map_add_str(const char *key, const char *str, int mapping) {
	if (!key || !mapping || !str)
		return;

	int k = yaml_document_add_scalar(&mc.d, (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int v = yaml_document_add_scalar(&mc.d, NULL, (yaml_char_t *)str, -1, YAML_PLAIN_SCALAR_STYLE);

	if (k && v)
		yaml_document_append_mapping_pair(&mc.d, mapping, k, v);
}

void yaml_map_add_int(const char *key, const int32_t val, int mapping) {
	if (!key || !mapping)
		return;

	char str[20];
	snprintf(str, 20, "%d", val);

	yaml_map_add_str(key, str, mapping);
}

void yaml_map_add_int_nz(const char *key, const int32_t val, int mapping) {
	if (!key || !mapping || val == 0)
		return;

	char str[20];
	snprintf(str, 20, "%d", val);

	yaml_map_add_str(key, str, mapping);
}

void yaml_map_add_float_nz(const char *key, const float val, int mapping) {
	if (!key || !mapping || val == 0)
		return;

	char str[100];
	snprintf(str, 100, "%g", val);

	yaml_map_add_str(key, str, mapping);
}

void yaml_map_add_bool(const char *key, const bool val, int mapping) {
	if (!key || !mapping)
		return;

	yaml_map_add_str(key, (val ? "TRUE" : "FALSE"), mapping);
}

void yaml_map_add_enum(const char *key, const int val, fn_enum_name fn_name, int mapping) {
	if (!key || !fn_name || !mapping)
		return;

	if (fn_name == on_off_name && val) {
		yaml_map_add_bool(key, val == ON, mapping);
	} else {
		const char *str = fn_name(val);
		if (!str)
			return;
		yaml_map_add_str(key, str, mapping);
	}
}

void yaml_map_add_plist(const char *key, const struct Plist* const plist, fn_yaml_node_from_type fn, int mapping) {
	if (!key || plist_size(plist) == 0)
		return;

	int k = yaml_document_add_scalar(&mc.d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&mc.d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return;

	for (const struct PlistIt *it = plist_it(plist); it; it = plist_it_next(it)) {
		int n = fn(it->val);
		if (n)
			yaml_document_append_sequence_item(&mc.d, seq, n);
	}

	yaml_document_append_mapping_pair(&mc.d, mapping, k, seq);
}

void yaml_map_add_sset(const char *key, const struct Sset* const sset, int mapping) {
	if (!key || !sset)
		return;

	int k = yaml_document_add_scalar(&mc.d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	if (!k)
		return;

	int n = 0;

	if (sset_size(sset) == 0) {
		if (!(n = yaml_document_add_scalar(&mc.d, NULL, (yaml_char_t*)"", 0, YAML_PLAIN_SCALAR_STYLE)))
			return;
	} else {
		if (!(n = yaml_document_add_sequence(&mc.d, NULL, YAML_BLOCK_SEQUENCE_STYLE)))
			return;

		for (const struct SsetIt *it = sset_it(sset); it; it = sset_it_next(it)) {
			int scalar = yaml_document_add_scalar(&mc.d, NULL, (yaml_char_t *)it->val, -1, YAML_PLAIN_SCALAR_STYLE);
			if (scalar)
				yaml_document_append_sequence_item(&mc.d, n, scalar);
		}
	}

	yaml_document_append_mapping_pair(&mc.d, mapping, k, n);
}

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <yaml.h>

#include "yaml/marshal-primitives.h"

#include "enum.h"
#include "ppmap.h"
#include "pset.h"
#include "simap.h"
#include "spmap.h"
#include "sset.h"
#include "yaml/marshal.h"

void yaml_map_add_node(struct MC *c, const char *key, int node, int mapping) {
	if (!key || !mapping || !node)
		return;

	int k = yaml_document_add_scalar(&c->d, (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	if (k)
		yaml_document_append_mapping_pair(&c->d, mapping, k, node);
}

void yaml_map_add_str(struct MC *c, const char *key, const char *str, int mapping) {
	if (!key || !mapping || !str)
		return;

	int k = yaml_document_add_scalar(&c->d, (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int v = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)str, -1, YAML_PLAIN_SCALAR_STYLE);

	if (k && v)
		yaml_document_append_mapping_pair(&c->d, mapping, k, v);
}

void yaml_map_add_int(struct MC *c, const char *key, const int32_t val, int mapping) {
	if (!key || !mapping)
		return;

	char str[20];
	snprintf(str, 20, "%d", val);

	yaml_map_add_str(c, key, str, mapping);
}

void yaml_map_add_int_nz(struct MC *c, const char *key, const int32_t val, int mapping) {
	if (!key || !mapping || val == 0)
		return;

	char str[20];
	snprintf(str, 20, "%d", val);

	yaml_map_add_str(c, key, str, mapping);
}

void yaml_map_add_float_nz(struct MC *c, const char *key, const float val, int mapping) {
	if (!key || !mapping || val == 0)
		return;

	char str[100];
	snprintf(str, 100, "%g", val);

	yaml_map_add_str(c, key, str, mapping);
}

void yaml_map_add_bool(struct MC *c, const char *key, const bool val, int mapping) {
	if (!key || !mapping)
		return;

	yaml_map_add_str(c, key, (val ? "TRUE" : "FALSE"), mapping);
}

void yaml_map_add_enum(struct MC *c, const char *key, const int val, fn_enum_name fn_name, int mapping) {
	if (!key || !fn_name || !mapping)
		return;

	const char *str = fn_name(val);
	if (!str)
		return;

	// use T/F to obey schema
	if (fn_name == on_off_name)
		str = (val == ON) ? "TRUE" : "FALSE";

	yaml_map_add_str(c, key, str, mapping);
}

void yaml_map_add_sset(struct MC *c, const char *key, const struct Sset* const sset, int mapping) {
	if (!key || sset_size(sset) == 0)
		return;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return;

	for (const struct SsetIt *it = sset_it(sset); it; it = sset_it_next(it)) {
		int scalar = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)it->val, -1, YAML_PLAIN_SCALAR_STYLE);
		if (scalar)
			yaml_document_append_sequence_item(&c->d, seq, scalar);
	}

	yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

void yaml_map_add_pset(struct MC *c, const char *key, const struct Pset* const pset, fn_yaml_node_from_type fn, int mapping) {
	if (!key || pset_size(pset) == 0)
		return;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return;

	for (const struct PsetIt *it = pset_it(pset); it; it = pset_it_next(it)) {
		int n = fn(c, it->val);
		if (n)
			yaml_document_append_sequence_item(&c->d, seq, n);
	}

	yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

void yaml_map_add_spmap(struct MC *c, const char *key, const struct SPmap* const spmap, fn_yaml_node_from_key_type fn, int mapping) {
	if (!key || spmap_size(spmap) == 0)
		return;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return;

	for (const struct SPmapIt *it = spmap_it(spmap); it; it = spmap_it_next(it)) {
		int n = fn(c, it->key, it->val);
		if (n)
			yaml_document_append_sequence_item(&c->d, seq, n);
	}

	yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

void yaml_map_add_ppmap(struct MC *c, const char *key, const struct PPmap* const ppmap, fn_yaml_node_from_key_type fn, int mapping) {
	if (!key || ppmap_size(ppmap) == 0)
		return;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return;

	for (const struct PPmapIt *it = ppmap_it(ppmap); it; it = ppmap_it_next(it)) {
		int n = fn(c, it->key, it->val);
		if (n)
			yaml_document_append_sequence_item(&c->d, seq, n);
	}

	yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

void yaml_map_add_simap(struct MC *c, const char *key, const struct SImap* const simap, fn_node_from_yaml_key_size_t fn, int mapping) {
	if (!key || simap_size(simap) == 0)
		return;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return;

	for (const struct SImapIt *it = simap_it(simap); it; it = simap_it_next(it)) {
		int n = fn(c, it->key, it->val);
		if (n)
			yaml_document_append_sequence_item(&c->d, seq, n);
	}

	yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}


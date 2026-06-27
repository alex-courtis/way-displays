#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>

#include "yaml/marshal-primitives.h"

#include "cfg.h"
#include "convert.h"
#include "slist.h"
#include "pset.h"
#include "smap.h"
#include "smapi.h"
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

void yaml_map_add_list(struct MC *c, const char *key, const struct SList *list, fn_yaml_v_to_node fn, int mapping) {
	if (slist_length(list) == 0)
		return;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return;

	for (const struct SList *i = list; i; i = i->nex) {
		int n = fn(c, i->val);
		if (n)
			yaml_document_append_sequence_item(&c->d, seq, n);
	}

	yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

void yaml_map_add_sset(struct MC *c, const char *key, const struct SSet *sset, int mapping) {
	if (sset_size(sset) == 0)
		return;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return;

	for (const struct SSetIt *it = sset_it(sset); it; it = sset_it_next(it)) {
		int scalar = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)it->val, -1, YAML_PLAIN_SCALAR_STYLE);
		if (scalar)
			yaml_document_append_sequence_item(&c->d, seq, scalar);
	}

	yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

void yaml_map_add_pset(struct MC *c, const char *key, const struct PSet *pset, fn_yaml_v_to_node fn, int mapping) {
	if (pset_size(pset) == 0)
		return;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return;

	for (const struct PSetIt *it = pset_it(pset); it; it = pset_it_next(it)) {
		int n = fn(c, it->val);
		if (n)
			yaml_document_append_sequence_item(&c->d, seq, n);
	}

	yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

void yaml_map_add_smap(struct MC *c, const char *key, const struct SMap* smap, fn_yaml_kv_to_node fn, int mapping) {
	if (smap_size(smap) == 0)
		return;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return;

	for (const struct SMapIt *it = smap_it(smap); it; it = smap_it_next(it)) {
		int n = fn(c, it->key, it->val);
		if (n)
			yaml_document_append_sequence_item(&c->d, seq, n);
	}

	yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

void yaml_map_add_smapi(struct MC *c, const char *key, const struct SMapI* smapi, fn_yaml_ki_to_node fn, int mapping) {
	if (smapi_size(smapi) == 0)
		return;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return;

	for (const struct SMapIIt *it = smapi_it(smapi); it; it = smapi_it_next(it)) {
		int n = fn(c, it->key, it->val);
		if (n)
			yaml_document_append_sequence_item(&c->d, seq, n);
	}

	yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}


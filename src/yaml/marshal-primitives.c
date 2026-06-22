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
#include "sset.h"
#include "yaml/marshal.h"

bool yaml_map_add_str(struct MC *c, const char *key, const char *str, int mapping) {
	if (!key || !mapping)
		return false;

	if (!str)
		return true;

	int k = yaml_document_add_scalar(&c->d, (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int v = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)str, -1, YAML_PLAIN_SCALAR_STYLE);

	return k && v && yaml_document_append_mapping_pair(&c->d, mapping, k, v);
}

bool yaml_map_add_int(struct MC *c, const char *key, const int32_t val, int mapping) {
	if (!key || !mapping)
		return false;

	char str[20];
	snprintf(str, 20, "%d", val);

	return yaml_map_add_str(c, key, str, mapping);
}

bool yaml_map_add_int_nz(struct MC *c, const char *key, const int32_t val, int mapping) {
	if (!key || !mapping)
		return false;

	if (val == 0)
		return true;

	char str[20];
	snprintf(str, 20, "%d", val);

	return yaml_map_add_str(c, key, str, mapping);
}

bool yaml_map_add_float_nz(struct MC *c, const char *key, const float val, int mapping) {
	if (!key || !mapping)
		return false;

	if (val == 0)
		return true;

	char str[100];
	snprintf(str, 100, "%g", val);

	return yaml_map_add_str(c, key, str, mapping);
}

bool yaml_map_add_bool(struct MC *c, const char *key, const bool val, int mapping) {
	if (!key || !mapping)
		return false;

	return yaml_map_add_str(c, key, (val ? "TRUE" : "FALSE"), mapping);
}

bool yaml_map_add_enum(struct MC *c, const char *key, const int val, enum_name_fn fn_name, int mapping) {
	if (!key || !fn_name || !mapping)
		return false;

	const char *str = fn_name(val);
	if (!str)
		return true;

	// use T/F to obey schema
	if (fn_name == on_off_name)
		str = (val == ON) ? "TRUE" : "FALSE";

	return yaml_map_add_str(c, key, str, mapping);
}

bool yaml_seq_append_str(struct MC *c, const void *str, int sequence) {
	if (!sequence)
		return false;

	if (!str)
		return true;

	int scalar = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)str, -1, YAML_PLAIN_SCALAR_STYLE);

	return scalar && yaml_document_append_sequence_item(&c->d, sequence, scalar);
}

bool yaml_map_add_map(struct MC *c, const char *key, const void *data, yaml_map_populate_fn fn, int mapping) {
	if (!key || !fn || !mapping)
		return false;

	if (!data)
		return true;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (!k || !map)
		return false;

	return fn(c, data, map) && yaml_document_append_mapping_pair(&c->d, mapping, k, map);
}

bool yaml_map_add_seq_list(struct MC *c, const char *key, const struct SList *list, yaml_seq_append_val_fn fn, int mapping) {
	if (!key || !fn || !mapping)
		return false;

	if (!list)
		return true;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return false;

	for (const struct SList *i = list; i; i = i->nex) {
		if (!fn(c, i->val, seq))
			return false;
	}

	return yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

bool yaml_map_add_seq_sset(struct MC *c, const char *key, const struct SSet *sset, yaml_seq_append_val_fn fn, int mapping) {
	if (!key || !fn || !mapping)
		return false;

	if (!sset || sset_size(sset) == 0)
		return true;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return false;

	for (const struct SSetIter *it = sset_iter(sset); it; it = sset_iter_next(it)) {
		if (!fn(c, it->val, seq))
			return false;
	}

	return yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

bool yaml_map_add_seq_pset(struct MC *c, const char *key, const struct PSet *pset, yaml_seq_append_val_fn fn, int mapping) {
	if (!key || !fn || !mapping)
		return false;

	if (!pset || pset_size(pset) == 0)
		return true;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return false;

	for (const struct PSetIter *it = pset_iter(pset); it; it = pset_iter_next(it)) {
		if (!fn(c, it->val, seq))
			return false;
	}

	return yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

bool yaml_map_add_seq_smap(struct MC *c, const char *key, const struct SMap* smap, yaml_seq_append_key_val_fn fn, int mapping) {
	if (!key || !fn || !mapping)
		return false;

	if (!smap || smap_size(smap) == 0)
		return true;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return false;

	for (const struct SMapIter *it = smap_iter(smap); it; it = smap_iter_next(it)) {
		if (!fn(c, it->key, it->val, seq))
			return false;
	}

	return yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

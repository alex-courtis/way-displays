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

bool yaml_map_add_node(struct MC *c, const char *key, int node, int mapping) {
	if (!key || !mapping)
		return false;

	if (!node)
		return true;

	int k = yaml_document_add_scalar(&c->d, (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);

	return k && yaml_document_append_mapping_pair(&c->d, mapping, k, node);
}

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

bool yaml_map_add_enum(struct MC *c, const char *key, const int val, fn_enum_name fn_name, int mapping) {
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

bool yaml_map_add_map(struct MC *c, const char *key, const void *data, fn_yaml_map_pop fn, int mapping) {
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

bool yaml_map_add_list(struct MC *c, const char *key, const struct SList *list, fn_yaml_node_from_v fn, int mapping) {
	if (slist_length(list) == 0)
		return true;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return false;

	for (const struct SList *i = list; i; i = i->nex) {
		int n = fn(c, i->val);
		if (!n || !yaml_document_append_sequence_item(&c->d, seq, n)) {
			return false;
		}
	}

	return yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

bool yaml_map_add_sset(struct MC *c, const char *key, const struct SSet *sset, int mapping) {
	if (sset_size(sset) == 0)
		return true;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return false;

	for (const struct SSetIt *it = sset_it(sset); it; it = sset_it_next(it)) {
		int scalar = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)it->val, -1, YAML_PLAIN_SCALAR_STYLE);
		if (scalar)
			yaml_document_append_sequence_item(&c->d, seq, scalar);
		else
			return false;
	}

	return yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

bool yaml_map_add_pset(struct MC *c, const char *key, const struct PSet *pset, fn_yaml_node_from_v fn, int mapping) {
	if (pset_size(pset) == 0)
		return true;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return false;

	for (const struct PSetIt *it = pset_it(pset); it; it = pset_it_next(it)) {
		int n = fn(c, it->val);
		if (!n || !yaml_document_append_sequence_item(&c->d, seq, n)) {
			return false;
		}
	}

	return yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

bool yaml_map_add_smap(struct MC *c, const char *key, const struct SMap* smap, fn_yaml_node_from_kv fn, int mapping) {
	if (smap_size(smap) == 0)
		return true;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return false;

	for (const struct SMapIt *it = smap_it(smap); it; it = smap_it_next(it)) {
		int n = fn(c, it->key, it->val);
		if (!n || !yaml_document_append_sequence_item(&c->d, seq, n)) {
			return false;
		}
	}

	return yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}

bool yaml_map_add_smapi(struct MC *c, const char *key, const struct SMapI* smapi, fn_yaml_node_from_ki fn, int mapping) {
	if (smapi_size(smapi) == 0)
		return true;

	int k = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)key, -1, YAML_PLAIN_SCALAR_STYLE);
	int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);

	if (!k || !seq)
		return false;

	for (const struct SMapIIt *it = smapi_it(smapi); it; it = smapi_it_next(it)) {
		int n = fn(c, it->key, it->val);
		if (!n || !yaml_document_append_sequence_item(&c->d, seq, n)) {
			return false;
		}
	}

	return yaml_document_append_mapping_pair(&c->d, mapping, k, seq);
}


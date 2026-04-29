#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <yaml.h>

#include "yaml-marshal.h"

#include "cfg.h"
#include "convert.h"
#include "log.h"
#include "slist.h"

struct MarshalCtx marshal_ctx = { 0 };

typedef bool (*map_fn)(const void *data, int mapping);
bool map_key_to_map(const char *k, const void *data, map_fn fn, int mapping) {
	if (!k || !fn || !mapping)
		return false;

	int key = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)k, -1, YAML_PLAIN_SCALAR_STYLE);
	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (!key || !map)
		return false;

	fn(data, map);

	return yaml_document_append_mapping_pair(marshal_ctx.doc, mapping, key, map);
}

typedef bool (*seq_fn)(const void *list, int sequence);
bool map_key_to_list(const char *k, const struct SList *list, seq_fn fn, int mapping) {
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

bool map_key_to_str(const char *k, const char *v, int mapping) {
	if (!k || !v || !mapping)
		return false;

	int key = yaml_document_add_scalar(marshal_ctx.doc, (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG, (yaml_char_t *)k, -1, YAML_PLAIN_SCALAR_STYLE);
	int scalar = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)v, -1, YAML_PLAIN_SCALAR_STYLE);

	return key && scalar && yaml_document_append_mapping_pair(marshal_ctx.doc, mapping, key, scalar);
}

bool map_key_to_int(const char *k, const int32_t v, int mapping) {
	if (!k || !mapping)
		return false;

	char v_str[20];
	snprintf(v_str, 20, "%d", v);

	return map_key_to_str(k, v_str, mapping);
}

bool map_key_to_float(const char *k, const float v, int mapping) {
	if (!k || v == 0 || !mapping)
		return false;

	char v_str[100];
	snprintf(v_str, 100, "%g", v);

	return map_key_to_str(k, v_str, mapping);
}

bool map_key_to_bool(const char *k, const bool v, int mapping) {
	if (!k || !mapping)
		return false;

	return map_key_to_str(k, (v ? "TRUE" : "FALSE"), mapping);
}

typedef const char* (*key_fn)(unsigned int v);
bool map_key_to_enum(const char *k, const int v, key_fn fn_name, int mapping) {
	if (!k || !fn_name || !mapping)
		return false;

	const char *v_str = fn_name(v);
	if (!v_str)
		return false;

	// use T/F to obey schema
	if (fn_name == on_off_name)
		v_str = (v == ON) ? "TRUE" : "FALSE";

	return map_key_to_str(k, v_str, mapping);
}

bool seq_str(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	int scalar = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)data, -1, YAML_PLAIN_SCALAR_STYLE);

	return scalar && yaml_document_append_sequence_item(marshal_ctx.doc, sequence, scalar);
}

static int write_handler(void *data, unsigned char *buffer, size_t size) {
	if (!data)
		return 0;

	char **yaml = (char**)(data);

	*yaml = calloc(1, size + 1);

	strncpy(*yaml, (char*)buffer, size);

	return 1;
}

static char *yaml_document_to_string(yaml_document_t *document, const char *name) {
	char *yaml = NULL;

	yaml_emitter_t emitter;

	if (!yaml_emitter_initialize(&emitter)) {
		log_error("unable to marshal %s: yaml_emitter_initialize failed", name);
		return NULL;
	}

	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);
	yaml_emitter_set_output(&emitter, write_handler, &yaml);

	if (!yaml_emitter_open(&emitter)) {
		log_error("unable to marshal %s: yaml_emitter_open failed", name);
		goto err;
	}

	if (!yaml_emitter_dump(&emitter, document)) {
		log_error("unable to marshal %s: yaml_emitter_dump failed", name);
		goto err;
	}

	if (!yaml_emitter_close(&emitter)) {
		log_warn("unable to marshal %s: yaml_emitter_close failed", name);
		goto err;
	}

	goto end;

err:
	if (yaml) {
		free(yaml);
		yaml = NULL;
	}

end:
	yaml_emitter_delete(&emitter);

	return yaml;
}

char *marshal_yaml(const void *data, marshal_fn fn, const char *name) {
	if (!data) {
		return NULL;
	}

	char *yaml = NULL;

	yaml_document_t document;
	marshal_ctx.doc = &document;

	if (!yaml_document_initialize(&document, NULL, NULL, NULL, 1, 1)) {
		log_error("unable to marshal %s: yaml_document_initialize failed", name);
		return NULL;
	}

	if (!fn(data))
		goto end;

	yaml = yaml_document_to_string(&document, name);

end:
	yaml_document_delete(&document);

	marshal_ctx.doc = NULL;

	return yaml;
}

// TODO use marshal_yaml
char *marshal_yaml_map(const void *data, map_fn fn, const char *name) {
	if (!data) {
		return NULL;
	}

	char *yaml = NULL;

	yaml_document_t document;
	marshal_ctx.doc = &document;

	if (!yaml_document_initialize(&document, NULL, NULL, NULL, 1, 1)) {
		log_error("unable to marshal %s: yaml_document_initialize failed", name);
		return NULL;
	}

	int root;
	if (!(root = yaml_document_add_mapping(&document, NULL, YAML_BLOCK_MAPPING_STYLE))) {
		log_error("unable to marshal %s: yaml_document_add_mapping for root failed", name);
		goto end;
	}

	if (!fn(data, root))
		goto end;

	yaml = yaml_document_to_string(&document, name);

end:
	yaml_document_delete(&document);

	marshal_ctx.doc = NULL;

	return yaml;
}

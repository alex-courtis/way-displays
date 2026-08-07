#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml/unmarshal.h"

#include "enum.h"
#include "log.h"
#include "regx.h"
#include "spmap.h"
#include "str.h"

struct UC uc = { 0 };

static void log_error_parser(const yaml_parser_t *parser, const char *prefix) {
	char *err = strdup(prefix);

	if (parser && parser->problem) {

		if (parser->problem_mark.line)
			err = sprintf_append(err, " line %zu", parser->problem_mark.line);

		if (parser->problem_mark.column)
			err = sprintf_append(err, " column %zu", parser->problem_mark.column);

		err = sprintf_append(err, ": %s", parser->problem);

		if (parser->context)
			err = sprintf_append(err, " (%s)", parser->context);
	}

	log_error(NULL);
	log_error("%s", err);

	free(err);
}

static void log_error_yaml(const char *yaml) {
	if (yaml)
		log_error("========================================\n%s\n----------------------------------------", yaml);
}

void *yaml_unmarshal_file(const char *path, fn_yaml_root_to_type fn) {
	if (!path)
		return NULL;

	FILE *input = fopen(path, "rb");
	if (!input) {
		log_error(NULL);
		log_error("%s: inexistent", path);
		return NULL;
	}

	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser)) {
		log_error(NULL);
		log_error("%s: yaml_parser_initialize failed", path);
		fclose(input);
		return NULL;
	}

	yaml_parser_set_input_file(&parser, input);

	memset(&uc, 0, sizeof(struct UC));
	yaml_unmarshal_log_ctx_top("document");

	// basename modifies path
	char *tmp = strdup(path);
	yaml_unmarshal_log_prefix(basename(tmp));
	free(tmp);

	if (!yaml_parser_load(&parser, &uc.d)) {
		log_error_parser(&parser, path);

		yaml_parser_delete(&parser);
		fclose(input);
		return NULL;
	}

	void *out = NULL;

	out = fn(yaml_document_get_root_node(&uc.d));

	yaml_document_delete(&uc.d);

	yaml_parser_delete(&parser);

	memset(&uc, 0, sizeof(struct UC));

	fclose(input);

	// false flag resulting from function pointer call
	// cppcheck-suppress returnDanglingLifetime
	return out;
}

void *yaml_unmarshal_str(const char *yaml, fn_yaml_root_to_type fn, char *human) {
	if (!yaml || !human)
		return NULL;

	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser)) {
		log_error(NULL);
		log_error("%s: yaml_parser_initialize failed", human);
		return NULL;
	}

	yaml_parser_set_input_string(&parser, (yaml_char_t*)yaml, strlen(yaml));

	memset(&uc, 0, sizeof(struct UC));
	yaml_unmarshal_log_ctx_top("document");
	yaml_unmarshal_log_prefix(human);

	if (!yaml_parser_load(&parser, &uc.d)) {
		log_error_parser(&parser, human);
		log_error_yaml(yaml);
		yaml_parser_delete(&parser);
		return NULL;
	}

	const yaml_node_t *root;

	void *out = NULL;

	if (!(root = yaml_document_get_root_node(&uc.d))) {
		log_error(NULL);
		log_error("%s: empty request", human);
		log_error_yaml(yaml);
		goto end;
	}

	if (!(out = fn(root)))
		log_error_yaml(yaml);

end:
	yaml_document_delete(&uc.d);

	yaml_parser_delete(&parser);

	memset(&uc, 0, sizeof(struct UC));

	return out;
}

void yaml_unmarshal_log_prefix(const char *prefix) {
	strncpy(uc.prefix, prefix ? prefix : "", sizeof(uc.prefix) - 1);
}

void yaml_unmarshal_log_def(const char *def) {
	strncpy(uc.def, def ? def : "", sizeof(uc.def) - 1);
}

void yaml_unmarshal_log_ctx_key(const char *key) {
	strncpy(uc.key, key ? key : "", sizeof(uc.key) - 1);
}

void yaml_unmarshal_log_ctx_name_desc(const char *name_desc) {
	strncpy(uc.name_desc, name_desc ? name_desc : "", sizeof(uc.name_desc) - 1);
}

void yaml_unmarshal_log_ctx_top(const char *top) {
	strncpy(uc.top, top ? top : "", sizeof(uc.top) - 1);
}

void yaml_unmarshal_log_enum_names(fn_enum_names fn) {
	uc.enum_names = fn;
}

void yaml_log_invalid_value(const yaml_char_t *value, const char *expected) {

	char *msg = NULL;

	if (*uc.prefix)
		msg = sprintf_append(msg, "%s:", uc.prefix);
	else
		msg = sprintf_append(msg, "Ignoring");

	if (*uc.top)
		msg = sprintf_append(msg, " invalid %s", uc.top);
	if (*uc.name_desc)
		msg = sprintf_append(msg, " '%s'", uc.name_desc);
	if (*uc.key)
		msg = sprintf_append(msg, " %s", uc.key);
	if (value)
		msg = sprintf_append(msg, " %s", value);
	if (expected) {
		msg = sprintf_append(msg, ", expected %s", expected);
		if (uc.enum_names) {
			char *valids = uc.enum_names();
			if (valids) {
				msg = sprintf_append(msg, " %s", valids);
				free(valids);
			}
		}
	}
	if (*uc.def)
		msg = sprintf_append(msg, ", using default %s", uc.def);

	log_(uc.t, "%s", msg);
	free(msg);
}

bool yaml_check_is_scalar(const yaml_node_t *node, const char *expected) {
	if (node && node->type == YAML_SCALAR_NODE)
		return true;

	yaml_log_invalid_value(NULL, expected);

	return false;
}

bool yaml_check_node_type(const yaml_node_t *node_actual, const yaml_node_type_t type1, const yaml_node_type_t type2) {
	if (node_actual && (node_actual->type == type1 || node_actual->type == type2))
		return true;

	char *expected = sprintf_alloc("%s", yaml_node_type_str(type1));
	if (type2)
		expected = sprintf_append(expected, " or %s", yaml_node_type_str(type2));
	expected = sprintf_append(expected, ", got %s", yaml_node_type_str(node_actual ? node_actual->type : YAML_NO_NODE));

	yaml_log_invalid_value(NULL, expected);

	free(expected);

	return false;
}

void yaml_log_unknown_keys(const struct SPmap *m, const char *expected) {
	for (const struct SPmapIt *it = spmap_it(m); it; it = spmap_it_next(it)) {
		yaml_log_invalid_value((yaml_char_t*)it->key, expected);
	}
}

bool yaml_valid_name_desc(const char *pattern) {
	if (!pattern || strlen(pattern) < 2 || pattern[0] != '!')
		return true;

	char *err = regex_compiles(pattern + 1);

	if (err) {
		char *msg = sprintf_alloc("regex '%s': %s", pattern + 1, err);
		yaml_log_invalid_value((yaml_char_t*)msg, NULL);
		free(msg);
		free(err);
		return false;
	}

	return true;
}

char *yaml_node_type_str(const yaml_node_type_t type) {
	switch (type) {
		case YAML_NO_NODE:
			return "empty";
		case YAML_MAPPING_NODE:
			return "map";
		case YAML_SEQUENCE_NODE:
			return "sequence";
		case YAML_SCALAR_NODE:
			return "scalar";
		default:
			return "???";
	}
}

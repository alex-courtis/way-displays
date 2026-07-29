#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml/unmarshal.h"

#include "enum.h"
#include "log.h"
#include "regx.h"
#include "str.h"

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
	if (!path) {
		return NULL;
	}

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

	struct UC c = { 0 };

	if (!yaml_parser_load(&parser, &c.d)) {
		log_error_parser(&parser, path);

		yaml_parser_delete(&parser);
		fclose(input);
		return NULL;
	}

	const yaml_node_t *root;

	void *out = NULL;

	if (!(root = yaml_document_get_root_node(&c.d))) {
		log_error(NULL);
		log_error("%s: no root node", path);
		goto end;
	}

	yaml_unmarshal_log_ctx_top(&c, path);

	out = fn(&c, root);

end:
	yaml_document_delete(&c.d);

	yaml_parser_delete(&parser);
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

	struct UC c = { 0 };
	yaml_unmarshal_log_prefix(&c, human);

	if (!yaml_parser_load(&parser, &c.d)) {
		log_error_parser(&parser, human);
		log_error_yaml(yaml);
		yaml_parser_delete(&parser);
		return NULL;
	}

	const yaml_node_t *root;

	void *out = NULL;

	if (!(root = yaml_document_get_root_node(&c.d))) {
		log_error(NULL);
		log_error("%s: empty request", human);
		log_error_yaml(yaml);
		goto end;
	}

	if (!(out = fn(&c, root)))
		log_error_yaml(yaml);

end:
	yaml_document_delete(&c.d);

	yaml_parser_delete(&parser);

	return out;
}

void yaml_unmarshal_log_prefix(struct UC *c, const char *prefix) {
	strncpy(c->prefix, prefix ? prefix : "", sizeof(c->prefix) - 1);
}

void yaml_unmarshal_log_def(struct UC *c, const char *def) {
	strncpy(c->def, def ? def : "", sizeof(c->def) - 1);
}

void yaml_unmarshal_log_ctx_key(struct UC *c, const char *key) {
	strncpy(c->key, key ? key : "", sizeof(c->key) - 1);
}

void yaml_unmarshal_log_ctx_name_desc(struct UC *c, const char *name_desc) {
	strncpy(c->name_desc, name_desc ? name_desc : "", sizeof(c->name_desc) - 1);
}

void yaml_unmarshal_log_ctx_top(struct UC *c, const char *top) {
	strncpy(c->top, top ? top : "", sizeof(c->top) - 1);
}

void yaml_unmarshal_log_enum_names(struct UC *c, fn_enum_names fn) {
	c->enum_names = fn;
}

static void yaml_log(struct UC *c, const char *value, const char *expected) {

	char *msg = NULL;

	if (*c->prefix)
		msg = sprintf_append(msg, "%s:", c->prefix);
	else
		msg = sprintf_append(msg, "Ignoring");

	if (*c->top)
		msg = sprintf_append(msg, " invalid %s", c->top);
	if (*c->name_desc)
		msg = sprintf_append(msg, " '%s'", c->name_desc);
	if (*c->key)
		msg = sprintf_append(msg, " %s", c->key);
	if (value)
		msg = sprintf_append(msg, " %s", value);
	if (expected) {
		msg = sprintf_append(msg, ", expected %s", expected);
		if (c->enum_names) {
			char *valids = c->enum_names();
			if (valids) {
				msg = sprintf_append(msg, ", valid values: %s", valids);
				free(valids);
			}
		}
	}
	if (*c->def)
		msg = sprintf_append(msg, ", using default %s", c->def);

	log_(c->t, "%s", msg);
	free(msg);
}

static void yaml_log_misssing(struct UC *c) {

	char *msg = NULL;

	if (*c->prefix)
		msg = sprintf_append(msg, "%s: missing %s", c->prefix, c->top);
	else
		msg = sprintf_append(msg, "%s: Ignoring missing", c->top);

	if (*c->key)
		msg = sprintf_append(msg, " %s", c->key);

	if (*c->name_desc)
		msg = sprintf_append(msg, " for '%s'", c->name_desc);

	if (msg) {
		log_(c->t, "%s", msg);
		free(msg);
	}
}

void yaml_unmarshal_log_invalid_value(struct UC *c, const yaml_char_t *value, const char *expected) {
	yaml_log(c, (char*)value, expected);
}

bool yaml_check_is_scalar(struct UC *c, const yaml_node_t *node, const char *expected) {
	if (node && node->type == YAML_SCALAR_NODE)
		return true;

	yaml_unmarshal_log_invalid_value(c, NULL, expected);

	return false;
}

bool yaml_check_node_type(struct UC *c, const yaml_node_t *node_actual, const yaml_node_type_t type1, const yaml_node_type_t type2) {
	if (node_actual && (node_actual->type == type1 || node_actual->type == type2))
		return true;

	char *expected = sprintf_alloc("%s", yaml_node_type_str(type1));
	if (type2)
		expected = sprintf_append(expected, " or %s", yaml_node_type_str(type2));
	expected = sprintf_append(expected, ", got %s", yaml_node_type_str(node_actual ? node_actual->type : YAML_NO_NODE));

	yaml_log(c, NULL, expected);

	free(expected);

	return false;
}

bool yaml_check_mandatory(struct UC *c, const yaml_node_t *node) {
	if (node)
		return true;

	yaml_log_misssing(c);

	return false;
}

bool yaml_valid_regex(struct UC *c, const char *pattern) {
	if (!pattern || strlen(pattern) < 2 || pattern[0] != '!')
		return true;

	char *err = regex_compiles(pattern + 1);

	if (err) {
		char *msg = sprintf_alloc("regex '%s': %s", pattern + 1, err);
		yaml_log(c, msg, NULL);
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

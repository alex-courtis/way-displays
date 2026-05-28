#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml/unmarshal.h"

#include "log.h"
#include "yaml/context.h"
#include "yaml/unmarshal-log.h"

static void log_error_parser(const yaml_parser_t *parser, const char *prefix) {
	char *err = sprintf_alloc("parsing %s", prefix);

	if (parser && parser->problem) {

		if (parser->problem_mark.line)
			err = sprintf_append(err, " line %zu", parser->problem_mark.line);

		if (parser->problem_mark.column)
			err = sprintf_append(err, " column %zu", parser->problem_mark.column);

		err = sprintf_append(err, ": %s", parser->problem);

		if (parser->context)
			err = sprintf_append(err, " (%s)", parser->context);
	}

	log_error("");
	log_error("%s", err);

	free(err);
}

static void log_error_yaml(const char *yaml) {
	if (!yaml)
		return;

	if (yaml) {
		log_error("========================================\n%s\n----------------------------------------", yaml);
	}
}

void *yaml_unmarshal_file(const char *path, yaml_root_to_type_fn fn) {
	yaml_unmarshal_log_ctx_reset();

	if (!path) {
		return NULL;
	}

	FILE *input = fopen(path, "rb");
	if (!input) {
		log_error("");
		log_error("parsing %s: inexistent", path);
		return NULL;
	}

	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser)) {
		log_error("");
		log_error("parsing %s: yaml_parser_initialize failed", path);
		fclose(input);
		return NULL;
	}

	yaml_parser_set_input_file(&parser, input);

	yaml_document_t document;

	if (!yaml_parser_load(&parser, &document)) {
		log_error_parser(&parser, path);

		yaml_parser_delete(&parser);
		fclose(input);
		return NULL;
	}

	const yaml_node_t *root;

	void *out = NULL;

	if (!(root = yaml_document_get_root_node(&document))) {
		log_error("");
		log_error("parsing %s no root node", path);
		goto end;
	}

	yaml_document = &document;

	yaml_unmarshal_log_ctx_top(path);

	out = fn(root);

end:
	yaml_unmarshal_log_ctx_reset();
	yaml_document = NULL;

	yaml_document_delete(&document);

	yaml_parser_delete(&parser);
	fclose(input);

	return out;
}

void *yaml_unmarshal_str(const char *yaml, yaml_root_to_type_fn fn, char *human) {
	yaml_unmarshal_log_ctx_reset();

	if (!yaml || !human)
		return NULL;

	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser)) {
		log_error("");
		log_error("parsing %s: yaml_parser_initialize failed", human);
		return NULL;
	}

	yaml_parser_set_input_string(&parser, (yaml_char_t*)yaml, strlen(yaml));

	yaml_document_t document;

	if (!yaml_parser_load(&parser, &document)) {
		log_error_parser(&parser, human);
		log_error_yaml(yaml);
		yaml_parser_delete(&parser);
		return NULL;
	}

	const yaml_node_t *root;

	void *out = NULL;

	if (!(root = yaml_document_get_root_node(&document))) {
		log_error("");
		log_error("parsing %s: empty request", human);
		log_error_yaml(yaml);
		goto end;
	}

	yaml_document = &document;

	char *prefix = sprintf_alloc("parsing %s", human);
	yaml_unmarshal_log_ctx_prefix(prefix);
	free(prefix);

	if (!(out = fn(root)))
		log_error_yaml(yaml);

end:
	yaml_unmarshal_log_ctx_reset();
	yaml_document = NULL;

	yaml_document_delete(&document);

	yaml_parser_delete(&parser);

	return out;
}

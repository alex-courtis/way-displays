#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <yaml.h>

#include "yaml/unmarshal.h"

#include "log.h"
#include "yaml/context.h"
#include "yaml/unmarshal-log.h"

void *yaml_unmarshal_file(const char *path, yaml_root_to_type_fn fn) {
	yaml_unmarshal_log_ctx_reset();

	if (!path) {
		return NULL;
	}

	FILE *input = fopen(path, "rb");
	if (!input) {
		log_error("\nparsing file %s: inexistent", path);
		return NULL;
	}

	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser)) {
		log_error("\nparsing file %s: yaml_parser_initialize failed", path);
		fclose(input);
		return NULL;
	}

	yaml_parser_set_input_file(&parser, input);

	yaml_document_t document;

	if (!yaml_parser_load(&parser, &document)) {
		log_error("\nparsing file %s: yaml_parser_load failed", path);
		yaml_parser_delete(&parser);
		fclose(input);
		return false;
	}

	const yaml_node_t *root;

	void *out = NULL;

	if (!(root = yaml_document_get_root_node(&document))) {
		log_error("\nparsing file %s no root node", path);
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

void *yaml_unmarshal_str(const char *yaml, yaml_root_to_type_fn fn, char *name) {
	yaml_unmarshal_log_ctx_reset();

	if (!yaml || !name)
		return NULL;

	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser)) {
		log_error("\n%s: yaml_parser_initialize failed", name);
		return NULL;
	}

	yaml_parser_set_input_string(&parser, (yaml_char_t*)yaml, strlen(yaml));

	yaml_document_t document;

	if (!yaml_parser_load(&parser, &document)) {
		log_error("\n%s: yaml_parser_load failed", name);
		yaml_parser_delete(&parser);
		log_error("========================================\n%s\n----------------------------------------", yaml);
		return NULL;
	}

	const yaml_node_t *root;

	void *out = NULL;

	if (!(root = yaml_document_get_root_node(&document))) {
		log_error("\n%s: empty request", name);
		goto err;
	}

	yaml_document = &document;
	yaml_unmarshal_log_ctx_prefix(name);

	if ((out = fn(root)))
		goto end;

err:
	log_error("========================================\n%s\n----------------------------------------", yaml);

end:
	yaml_unmarshal_log_ctx_reset();
	yaml_document = NULL;

	yaml_document_delete(&document);

	yaml_parser_delete(&parser);

	return out;
}

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <yaml.h>

#include "cfg.h"
#include "log.h"
#include "yaml/context.h"
#include "yaml/unmarshal-log.h"
#include "yaml/unmarshal-types.h"

bool yaml_file_into_cfg(struct Cfg *cfg) {
	yaml_unmarshal_log_ctx_reset();

	if (!cfg->file_path) {
		return false;
	}

	FILE *input = fopen(cfg->file_path, "rb");
	if (!input) {
		log_error("\nparsing file %s: inexistent", cfg->file_path);
		return false;
	}

	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser)) {
		log_error("\nparsing file %s: yaml_parser_initialize failed", cfg->file_path);
		fclose(input);
		return false;
	}

	yaml_parser_set_input_file(&parser, input);

	yaml_document_t document;
	yaml_document = &document;

	if (!yaml_parser_load(&parser, &document)) {
		log_error("\nparsing file %s: yaml_parser_load failed", cfg->file_path);
		yaml_parser_delete(&parser);
		fclose(input);
		return false;
	}

	bool ok = true;

	const yaml_node_t *root;

	if (!(root = yaml_document_get_root_node(yaml_document))) {
		log_error("\nparsing file %s no root node", cfg->file_path);
		ok = false;
		goto end;
	}

	if (root->type != YAML_MAPPING_NODE) {
		log_error("\nparsing file %s empty cfg, expected map", cfg->file_path);
		ok = false;
		goto end;
	}

	ok = yaml_map_to_cfg(cfg, root);

end:
	yaml_unmarshal_log_ctx_reset();
	yaml_document = NULL;

	yaml_document_delete(&document);

	yaml_parser_delete(&parser);
	fclose(input);

	return ok;
}

// marshal a yaml string to data via fn, logs use action
typedef void *(*yaml_root_to_struct_fn)(const yaml_node_t *root);
static void *yaml_to_struct(const char *yaml, yaml_root_to_struct_fn fn, char *action) {
	yaml_unmarshal_log_ctx_reset();

	if (!yaml || !action)
		return NULL;

	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser)) {
		log_error("\n%s: yaml_parser_initialize failed", action);
		return NULL;
	}

	yaml_parser_set_input_string(&parser, (yaml_char_t*)yaml, strlen(yaml));

	yaml_document_t document;

	if (!yaml_parser_load(&parser, &document)) {
		log_error("\n%s: yaml_parser_load failed", action);
		yaml_parser_delete(&parser);
		log_error("========================================\n%s\n----------------------------------------", yaml);
		return NULL;
	}

	const yaml_node_t *root;

	void *out = NULL;

	if (!(root = yaml_document_get_root_node(&document))) {
		log_error("\n%s: empty request", action);
		goto err;
	}

	yaml_document = &document;
	yaml_unmarshal_log_ctx_prefix(action);

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

struct IpcRequest *yaml_to_ipc_request(char *yaml) {
	return yaml_to_struct(yaml, yaml_root_to_ipc_request, "unmarshalling ipc request");
}

struct SList *yaml_to_ipc_responses(const char *yaml) {
	return yaml_to_struct(yaml, yaml_root_to_ipc_response_list, "unmarshalling ipc response");
}

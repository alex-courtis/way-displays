#include <math.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <yaml.h>

#include "yaml-unmarshal.h"

#include "cfg.h"
#include "convert.h"
#include "conditions.h"
#include "log.h"
#include "slist.h"
#include "stable.h"

static char *unmarshalling_yaml = NULL;

static struct IpcRequest *doc_to_ipc_request(yaml_document_t *document) {
	if (!yaml_document_get_root_node(document)) {
		log_error("\nTODO");
		return NULL;
	}

	const yaml_node_t *start_doc = document->nodes.start;
	if (!start_doc || start_doc->type != YAML_MAPPING_NODE) {
		log_error("\nTODO");
		return NULL;
	}

	struct IpcRequest *ipc_request = (struct IpcRequest*)calloc(1, sizeof(struct IpcRequest));

	for (const yaml_node_pair_t *pair = start_doc->data.mapping.pairs.start; pair < start_doc->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *key = yaml_document_get_node(document, pair->key);
		if (!key || key->type != YAML_SCALAR_NODE || !key->data.scalar.value)
			continue;

		const yaml_node_t *value = yaml_document_get_node(document, pair->value);
		if (!value)
			continue;

		const char *k = (char*)key->data.scalar.value;

		if (strcmp(k, "OP") == 0) {
			if (!scalar_to_enum((int*)&ipc_request->command, value, ipc_command_val)) {
				goto err;
			}
		} else if (strcmp(k, "LOG_THRESHOLD") == 0) {
			scalar_to_enum((int*)&ipc_request->log_threshold, value, log_threshold_val);
		}
	}

	goto end;

err:
	// TODO some context
	log_error("\nunmarshalling ipc request: invalid OP '%s'", "aoeu");
	log_error("========================================\n%s\n----------------------------------------", unmarshalling_yaml);

	ipc_request_free(ipc_request);
	ipc_request = NULL;

end:
	return ipc_request;
}

struct IpcRequest *yaml_to_ipc_request(char *yaml) {
	if (!yaml) {
		return NULL;
	}

	char *name = "ipc request";

	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser)) {
		log_error("unable to unmarshal %s: yaml_parser_initialize failed", name);
		return NULL;
	}

	yaml_parser_set_input_string(&parser, (yaml_char_t*)yaml, strlen(yaml));

	yaml_document_t document;

	if (!yaml_parser_load(&parser, &document)) {
		log_error("unable to unmarshal %s: yaml_parser_load failed", name);
		yaml_parser_delete(&parser);
		return NULL;
	}


	unmarshal_ctx_clear();
	unmarshal_ctx.document = &document;
	unmarshalling_yaml = yaml;

	struct IpcRequest *ipc_request = doc_to_ipc_request(&document);

	unmarshal_ctx_clear();
	unmarshalling_yaml = NULL;

	yaml_document_delete(&document);
	yaml_parser_delete(&parser);

	return ipc_request;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <yaml.h>

#include "yaml-unmarshal-context.h"
#include "yaml-unmarshal-primitives.h"
#include "yaml-unmarshal-types.h"

#include "cfg.h"
#include "convert.h"
#include "ipc.h"
#include "log.h"
#include "slist.h"
#include "stable.h"

bool yaml_file_into_cfg(struct Cfg *cfg) {
	yaml_log_ctx_reset();

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
	ctx.document = &document;

	if (!yaml_parser_load(&parser, &document)) {
		log_error("\nparsing file %s: yaml_parser_load failed", cfg->file_path);
		yaml_parser_delete(&parser);
		fclose(input);
		return false;
	}

	bool ok = true;

	const yaml_node_t *root;

	if (!(root = yaml_document_get_root_node(ctx.document))) {
		log_error("\nparsing file %s no root node", cfg->file_path);
		ok = false;
		goto end;
	}

	if (root->type != YAML_MAPPING_NODE) {
		log_error("\nparsing file %s empty cfg, expected map", cfg->file_path);
		ok = false;
		goto end;
	}

	ok = map_to_cfg(cfg, root);

end:
	yaml_log_ctx_reset();
	ctx.document = NULL;

	yaml_document_delete(&document);

	yaml_parser_delete(&parser);
	fclose(input);

	return ok;
}

static void *root_to_ipc_request(const yaml_node_t *root) {

	// log exceptions and fail for required fields
	yaml_log_ctx_t(ERROR);

	const struct STable *table = yaml_map_to_node_table(root);
	if (!table)
		return NULL;

	struct IpcRequest *ipc_request = (struct IpcRequest*)calloc(1, sizeof(struct IpcRequest));
	ipc_request->cfg = cfg_init();

	yaml_log_ctx_top("OP");
	const yaml_node_t *op = stable_get(table, "OP");
	if (!yaml_check_mandatory(op) || !(ipc_request->command = yaml_scalar_to_enum(op, ipc_command_val)))
		goto err;

	// log warnings for remainder
	yaml_log_ctx_t(WARNING);
	yaml_log_ctx_prefix(NULL);

	yaml_log_ctx_top("LOG_THRESHOLD");
	ipc_request->log_threshold = yaml_scalar_to_enum(stable_get(table, "LOG_THRESHOLD"), log_threshold_val);

	yaml_log_ctx_top("CFG");
	map_to_cfg(ipc_request->cfg, stable_get(table, "CFG"));

	goto end;

err:
	ipc_request_free(ipc_request);
	ipc_request = NULL;

end:
	stable_free(table);

	return ipc_request;
}

static struct IpcResponse *map_to_ipc_response(const yaml_node_t *map) {

	// log exceptions and fail for required fields
	yaml_log_ctx_t(ERROR);

	const struct STable *table = yaml_map_to_node_table(map);
	if (!table)
		return NULL;

	struct IpcResponse *ipc_response = (struct IpcResponse*)calloc(1, sizeof(struct IpcResponse));

	yaml_log_ctx_top("DONE");
	const yaml_node_t *done = stable_get(table, "DONE");
	if (!yaml_check_mandatory(done) || !yaml_scalar_to_boolean(&ipc_response->status.done, done))
		goto err;

	yaml_log_ctx_top("RC");
	const yaml_node_t *rc = stable_get(table, "RC");
	if (!yaml_check_mandatory(rc) || !yaml_scalar_to_int(&ipc_response->status.rc, rc))
		goto err;

	// suppress validation failures for remainder
	yaml_log_ctx_t(0);

	yaml_log_ctx_top("CFG");
	const yaml_node_t *cfg = stable_get(table, "CFG");
	if (cfg) {
		ipc_response->cfg = cfg_init();
		map_to_cfg(ipc_response->cfg, cfg);
	}

	yaml_log_ctx_top("STATE");
	const yaml_node_t *state = stable_get(table, "STATE");
	if (state) {
		const struct STable *table_state = yaml_map_to_node_table(state);
		if (table_state) {

			ipc_response->lid =	map_to_lid(stable_get(table_state, "LID"));

			ipc_response->heads = seq_to_type_list(stable_get(table_state, "HEADS"), map_to_head);

			stable_free(table_state);
		}
	}

	yaml_log_ctx_top("MESSAGES");
	const yaml_node_t *messages = stable_get(table, "MESSAGES");
	if (messages) {
		ipc_response->log_cap_lines = seq_to_log_cap_lines(messages);
	}

	goto end;

err:
	ipc_response_free(ipc_response);
	ipc_response = NULL;

end:
	stable_free(table);

	return ipc_response;
}

static void *root_to_ipc_response_list(const yaml_node_t *root) {

	if (!root)
		return NULL;

	struct SList *ipc_responses = NULL;

	if (root->type != YAML_MAPPING_NODE && root->type != YAML_SEQUENCE_NODE) {
		log_error("\nunmarshalling ipc response: expected %s or %s, got %s", yaml_node_type_str(YAML_MAPPING_NODE), yaml_node_type_str(YAML_SEQUENCE_NODE), yaml_node_type_str(root->type));
		goto err;
	}

	if (root->type == YAML_SEQUENCE_NODE) {
		for (const yaml_node_item_t *item = root->data.sequence.items.start; item < root->data.sequence.items.top; item ++) {
			const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
			if (!node)
				continue;

			struct IpcResponse *ipc_response = map_to_ipc_response(node);
			if (ipc_response)
				slist_append(&ipc_responses, ipc_response);
		}
	} else {
		struct IpcResponse *ipc_response = map_to_ipc_response(root);
		if (ipc_response)
			slist_append(&ipc_responses, ipc_response);
	}

	goto end;

err:
	slist_free_vals(&ipc_responses, ipc_response_free);
	ipc_responses = NULL;

end:
	return ipc_responses;
}

// marshal a yaml string to data via fn, logs use action
typedef void *(*root_to_struct_fn)(const yaml_node_t *root);
static void *yaml_to_struct(const char *yaml, root_to_struct_fn fn, char *action) {
	yaml_log_ctx_reset();

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

	ctx.document = &document;
	yaml_log_ctx_prefix(action);

	if ((out = fn(root)))
		goto end;

err:
	log_error("========================================\n%s\n----------------------------------------", yaml);

end:
	yaml_log_ctx_reset();
	ctx.document = NULL;

	yaml_document_delete(&document);

	yaml_parser_delete(&parser);

	return out;
}

struct IpcRequest *yaml_to_ipc_request(char *yaml) {
	return yaml_to_struct(yaml, root_to_ipc_request, "unmarshalling ipc request");
}

struct SList *yaml_to_ipc_responses(const char *yaml) {
	return yaml_to_struct(yaml, root_to_ipc_response_list, "unmarshalling ipc response");
}

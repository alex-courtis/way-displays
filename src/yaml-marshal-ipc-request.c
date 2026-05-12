#include <stdbool.h>
#include <stddef.h>
#include <yaml.h>

#include "yaml-marshal.h"

#include "convert.h"
#include "ipc.h"
#include "log.h"
#include "yaml/marshal-primitives.h"
#include "yaml/marshal-types.h"

static bool marshal_ipc_request_fn(const void *data) {
	if (!data)
		return false;

	const struct IpcRequest *ipc_request = data;

	int root = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!root) {
		log_error("unable to marshal ipc request: yaml_document_add_mapping for root failed");
		return false;
	}

	const char *op_name = ipc_command_name(ipc_request->command);
	if (!op_name) {
		log_error("unable to marshal ipc request: missing OP");
		return false;
	}

	yaml_map_add_str("OP", ipc_command_name(ipc_request->command), root);

	if (ipc_request->log_threshold)
		yaml_map_add_str("LOG_THRESHOLD", log_threshold_name(ipc_request->log_threshold), root);

	yaml_map_add_map("CFG", ipc_request->cfg, yaml_map_populate_cfg, root);

	return true;
}

char *ipc_request_to_yaml(const struct IpcRequest *ipc_request) {
	return struct_to_yaml(ipc_request, marshal_ipc_request_fn, "ipc request");
}

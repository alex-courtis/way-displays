#include <stdbool.h>
#include <stddef.h>
#include <yaml.h>

#include "yaml-marshal.h"

#include "convert.h"
#include "ipc.h"
#include "log.h"
#include "yaml-marshal-cfg.h"

bool marshal_ipc_request_fn(const void *data) {
	int root = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!root) {
		log_error("unable to marshal ipc request: yaml_document_add_mapping for root failed");
		return false;
	}

	const struct IpcRequest *ipc_request = data;

	const char *op_name = ipc_command_name(ipc_request->command);
	if (!op_name) {
		log_error("unable to marshal ipc request: missing OP");
		return false;
	}

	map_key_to_str("OP", ipc_command_name(ipc_request->command), root);

	if (ipc_request->log_threshold)
		map_key_to_str("LOG_THRESHOLD", log_threshold_name(ipc_request->log_threshold), root);

	map_key_to_map("CFG", ipc_request->cfg, map_cfg, root);

	return true;
}


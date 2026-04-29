#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>

#include "yaml-marshal.h"

#include "convert.h"
#include "ipc.h"
#include "log.h"

static bool map_ipc_request(const struct IpcRequest *request, int mapping) {
	if (!request)
		return false;

	const char *op_name = ipc_command_name(request->command);
	if (!op_name) {
		log_error("unable to marshal ipc request: missing OP");
		return false;
	}

	map_key_to_str("OP", ipc_command_name(request->command), mapping);

	if (request->log_threshold)
		map_key_to_str("LOG_THRESHOLD", log_threshold_name(request->log_threshold), mapping);

	map_key_to_map("CFG", request->cfg, map_cfg, mapping);

	return true;
}

char *marshal_ipc_request_2(const struct IpcRequest *request) {
	if (!request)
		return NULL;

	char *yaml = NULL;

	yaml_document_t document;
	ctx.document = &document;

	if (!yaml_document_initialize(&document, NULL, NULL, NULL, 1, 1)) {
		log_error("unable to marshal ipc request: yaml_document_initialize failed");
		return NULL;
	}

	int root;
	if (!(root = yaml_document_add_mapping(&document, NULL, YAML_BLOCK_MAPPING_STYLE))) {
		log_error("unable to marshal ipc request: yaml_document_add_mapping for root failed");
		goto end;
	}

	if (!map_ipc_request(request, root))
		goto end;

	yaml = yaml_document_to_string(&document);

end:
	yaml_document_delete(&document);

	return yaml;
}


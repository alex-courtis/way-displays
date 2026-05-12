#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>

#include "yaml-marshal.h"

#include "convert.h"
#include "global.h"
#include "head.h"
#include "ipc.h"
#include "log.h"
#include "slist.h"
#include "yaml/marshal-primitives.h"
#include "yaml/marshal-types.h"

// yaml_map_populate_fn
static bool marshal_ipc_response_fn(const void *data) {
	if (!data)
		return false;

	// map_messages mutates operation->rc based on max line level
	struct IpcOperation *ipc_operation = (struct IpcOperation*)data;

	// root is a sequence when not GET
	int seq = 0;
	if (ipc_operation->request->command != GET) {
		seq = yaml_document_add_sequence(marshal_ctx.doc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
		if (!seq) {
			log_error("unable to marshal ipc response: yaml_document_add_sequence for root failed");
			return false;
		}
	}

	// map will be root (GET) or appended to seq
	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map) {
		log_error("unable to marshal ipc response: yaml_document_add_mapping for root failed");
		return false;
	}
	if (!yaml_map_populate_ipc_operation(ipc_operation, map))
		return false;

	if (seq)
		return yaml_document_append_sequence_item(marshal_ctx.doc, seq, map);
	else
		return true;
}

char *ipc_response_to_yaml(struct IpcOperation *ipc_operation) {
	return struct_to_yaml(ipc_operation, marshal_ipc_response_fn, "ipc response");
}

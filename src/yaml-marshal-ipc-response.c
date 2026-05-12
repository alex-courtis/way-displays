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
static bool yaml_map_populate_state(const void *data, int mapping) {
	yaml_map_add_map("LID", NULL, yaml_map_populate_lid, mapping);
	yaml_map_add_seq("HEADS", heads, yaml_seq_append_head, mapping);

	return true;
}

static bool yaml_seq_append_log_cap_line(struct LogCapLine *line, int sequence) {
	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return false;

	if (!yaml_map_add_str(log_threshold_name(line->threshold), line->line, map))
		return false;

	return yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

static bool yaml_map_populate_messages(struct IpcOperation *ipc_operation, int mapping) {
	bool lines_added = false;

	int seq_lines = yaml_document_add_sequence(marshal_ctx.doc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	if (!seq_lines)
		return false;

	for (struct SList *i = ipc_operation->log_cap_lines; i; i = i->nex) {
		struct LogCapLine *cap_line = (struct LogCapLine*)i->val;

		if (!cap_line || !cap_line->line || cap_line->threshold < ipc_operation->request->log_threshold)
			continue;

		lines_added = yaml_seq_append_log_cap_line(cap_line, seq_lines) || lines_added;

		if (cap_line->threshold == WARNING && ipc_operation->rc < IPC_RC_WARN)
			ipc_operation->rc = IPC_RC_WARN;
		if (cap_line->threshold == ERROR && ipc_operation->rc < IPC_RC_ERROR)
			ipc_operation->rc = IPC_RC_ERROR;
	}

	if (lines_added) {
		int key = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)"MESSAGES", -1, YAML_PLAIN_SCALAR_STYLE);
		if (key)
			yaml_document_append_mapping_pair(marshal_ctx.doc, mapping, key, seq_lines);
	}

	return true;
}

static bool yaml_map_populate_ipc_response(struct IpcOperation *ipc_operation, int mapping) {
	yaml_map_add_bool("DONE", ipc_operation->done, mapping);

	if (ipc_operation->send_state) {
		if (cfg)
			yaml_map_add_map("CFG", cfg, yaml_map_populate_cfg, mapping);
		if (lid || heads)
			yaml_map_add_map("STATE", ipc_operation, yaml_map_populate_state, mapping);
	}

	yaml_map_populate_messages(ipc_operation, mapping);

	yaml_map_add_int("RC", ipc_operation->rc, mapping);

	return true;
}

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
	if (!yaml_map_populate_ipc_response(ipc_operation, map))
		return false;

	if (seq)
		return yaml_document_append_sequence_item(marshal_ctx.doc, seq, map);
	else
		return true;
}

char *ipc_response_to_yaml(struct IpcOperation *ipc_operation) {
	return struct_to_yaml(ipc_operation, marshal_ipc_response_fn, "ipc response");
}

#include <stdio.h>
#include <stdbool.h>
#include <wayland-util.h>
#include <yaml.h>

#include "yaml-marshal.h"

#include "convert.h"
#include "global.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml-marshal-cfg.h"
#include "yaml/marshal-primitives.h"

static bool map_lid(const void *data, int mapping) {
	yaml_map_add_bool("CLOSED",      lid->closed,      mapping);
	yaml_map_add_str ("DEVICE_PATH", lid->device_path, mapping);

	return true;
}

static bool map_mode(const void *data, int mapping) {
	if (!data)
		return false;

	const struct Mode *mode = data;

	yaml_map_add_int("WIDTH",       mode->width,       mapping);
	yaml_map_add_int("HEIGHT",      mode->height,      mapping);
	yaml_map_add_int("REFRESH_MHZ", mode->refresh_mhz, mapping);
	yaml_map_add_bool("PREFERRED",  mode->preferred,   mapping);

	return true;
}

static bool map_head_state(const void *data, int mapping) {
	if (!data)
		return false;

	const struct HeadState *head_state = data;

	yaml_map_add_float("SCALE",    wl_fixed_to_double(head_state->scale),                                          mapping);
	yaml_map_add_bool ("ENABLED",  head_state->enabled,                                                            mapping);
	yaml_map_add_int  ("X",        head_state->x,                                                                  mapping);
	yaml_map_add_int  ("Y",        head_state->y,                                                                  mapping);
	yaml_map_add_bool ("VRR",      (head_state->adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED), mapping);
	yaml_map_add_enum("TRANSFORM", head_state->transform,                                          transform_name, mapping);

	yaml_map_add_map("MODE", head_state->mode, map_mode, mapping);

	return true;
}

static bool map_head_overrides(const void *data, int mapping) {
	if (!data)
		return false;

	const struct Head *head = data;

	return (head->overrided_enabled != NoOverride) && yaml_map_add_bool("DISABLED", head->overrided_enabled == OverrideTrue, mapping);
}

static bool seq_mode(const void *data, int sequence) {
	if (!data)
		return false;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		map_mode(data, map) &&
		yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

static bool seq_head(const void *data, int sequence) {
	if (!data || !sequence)
		return false;

	const struct Head *head = data;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return false;

	if (head->name)          yaml_map_add_str("NAME",          head->name,          map);
	if (head->description)   yaml_map_add_str("DESCRIPTION",   head->description,   map);
	if (head->make)          yaml_map_add_str("MAKE",          head->make,          map);
	if (head->model)         yaml_map_add_str("MODEL",         head->model,         map);
	if (head->serial_number) yaml_map_add_str("SERIAL_NUMBER", head->serial_number, map);
	yaml_map_add_int(                         "WIDTH_MM",      head->width_mm,      map);
	yaml_map_add_int(                         "HEIGHT_MM",     head->height_mm,     map);

	yaml_map_add_map ("CURRENT",   &head->current, map_head_state,     map);
	yaml_map_add_map ("DESIRED",   &head->desired, map_head_state,     map);
	yaml_map_add_map ("OVERRIDES", head,           map_head_overrides, map);
	yaml_map_add_seq("MODES",     head->modes,    seq_mode,           map);

	return yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

static bool map_state(const void *data, int mapping) {
	yaml_map_add_map("LID", NULL, map_lid, mapping);
	yaml_map_add_seq("HEADS", heads, seq_head, mapping);

	return true;
}

static bool seq_log_cap_line(struct LogCapLine *line, int sequence) {
	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return false;

	if (!yaml_map_add_str(log_threshold_name(line->threshold), line->line, map))
		return false;

	return yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

static bool map_messages(struct IpcOperation *ipc_operation, int mapping) {
	bool lines_added = false;

	int seq_lines = yaml_document_add_sequence(marshal_ctx.doc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
	if (!seq_lines)
		return false;

	for (struct SList *i = ipc_operation->log_cap_lines; i; i = i->nex) {
		struct LogCapLine *cap_line = (struct LogCapLine*)i->val;

		if (!cap_line || !cap_line->line || cap_line->threshold < ipc_operation->request->log_threshold)
			continue;

		lines_added = seq_log_cap_line(cap_line, seq_lines) || lines_added;

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

static bool map_ipc_response(struct IpcOperation *ipc_operation, int mapping) {
	yaml_map_add_bool("DONE", ipc_operation->done, mapping);

	if (ipc_operation->send_state) {
		if (cfg)
			yaml_map_add_map("CFG", cfg, map_cfg, mapping);
		if (lid || heads)
			yaml_map_add_map("STATE", ipc_operation, map_state, mapping);
	}

	map_messages(ipc_operation, mapping);

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
	if (!map_ipc_response(ipc_operation, map))
		return false;

	if (seq)
		return yaml_document_append_sequence_item(marshal_ctx.doc, seq, map);
	else
		return true;
}

char *ipc_response_to_yaml(struct IpcOperation *ipc_operation) {
	return struct_to_yaml(ipc_operation, marshal_ipc_response_fn, "ipc response");
}

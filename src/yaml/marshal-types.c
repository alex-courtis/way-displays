#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <wayland-util.h>
#include <yaml.h>

#include "yaml/marshal-types.h"

#include "cfg.h"
#include "conditions.h"
#include "convert.h"
#include "global.h"
#include "head.h"
#include "lid.h"
#include "mode.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml/marshal.h"
#include "yaml/marshal-primitives.h"

bool yaml_doc_cfg(const void *data) {
	if (!data)
		return true;

	int mapping = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	return mapping && yaml_map_populate_cfg(data, mapping);
}

bool yaml_doc_ipc_operation(const void *data) {
	if (!data)
		return true;

	// map_messages mutates operation->rc based on max line level
	struct IpcOperation *ipc_operation = (struct IpcOperation*)data;

	// root sequence when not GET
	int seq = 0;
	if (ipc_operation->request->command != GET) {
		if (!(seq = yaml_document_add_sequence(marshal_ctx.doc, NULL, YAML_BLOCK_SEQUENCE_STYLE)))
			return false;
	}

	// root map for GET, otherwise append the map to seq
	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map || !yaml_map_populate_ipc_operation(ipc_operation, map))
		return false;

	if (seq)
		return yaml_document_append_sequence_item(marshal_ctx.doc, seq, map);
	else
		return true;
}

bool yaml_doc_ipc_request(const void *data) {
	if (!data)
		return true;

	const struct IpcRequest *ipc_request = data;

	if (!ipc_command_name(ipc_request->command)) {
		log_error("unable to marshal ipc request: missing OP");
		return false;
	}

	int root = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	return root && yaml_map_populate_ipc_request(ipc_request, root);
}

bool yaml_map_populate_cfg(const void *data, int mapping) {
	if (!mapping)
		return false;

	if (!data)
		return false;

	const struct Cfg *cfg = data;

	return
		yaml_map_add_enum (cfg_element_name(ARRANGE),               cfg->arrange,                     arrange_name,                   mapping) &&
		yaml_map_add_enum (cfg_element_name(ALIGN),                 cfg->align,                       align_name,                     mapping) &&
		yaml_map_add_seq  (cfg_element_name(ORDER),                 cfg->order_name_desc,             yaml_seq_append_str,            mapping) &&
		yaml_map_add_enum (cfg_element_name(SCALING),               cfg->scaling,                     on_off_name,                    mapping) &&
		yaml_map_add_enum (cfg_element_name(AUTO_SCALE),            cfg->auto_scale,                  on_off_name,                    mapping) &&
		yaml_map_add_float(cfg_element_name(AUTO_SCALE_MIN),        cfg->auto_scale_min,                                              mapping) &&
		yaml_map_add_float(cfg_element_name(AUTO_SCALE_MAX),        cfg->auto_scale_max,                                              mapping) &&
		yaml_map_add_seq  (cfg_element_name(SCALE),                 cfg->user_scales,                 yaml_seq_append_user_scale,     mapping) &&
		yaml_map_add_seq  (cfg_element_name(MODE),                  cfg->user_modes,                  yaml_seq_append_user_mode,      mapping) &&
		yaml_map_add_seq  (cfg_element_name(TRANSFORM),             cfg->user_transforms,             yaml_seq_append_user_transform, mapping) &&
		yaml_map_add_seq  (cfg_element_name(VRR_OFF),               cfg->adaptive_sync_off_name_desc, yaml_seq_append_str,            mapping) &&
		yaml_map_add_str  (cfg_element_name(CALLBACK_CMD),          cfg->callback_cmd,                                                mapping) &&
		yaml_map_add_str  (cfg_element_name(LAPTOP_DISPLAY_PREFIX), cfg->laptop_display_prefix,                                       mapping) &&
		yaml_map_add_enum (cfg_element_name(LOG_THRESHOLD),         cfg->log_threshold,               log_threshold_name,             mapping) &&
		yaml_map_add_seq  (cfg_element_name(DISABLED),              cfg->disabled,                    yaml_seq_append_disabled,       mapping);
}

bool yaml_map_populate_ipc_operation(void *data, int mapping) {
	if (!mapping)
		return false;

	if (!data)
		return true;

	struct IpcOperation *ipc_operation = data;

	if (!yaml_map_add_bool("DONE", ipc_operation->done, mapping))
		return false;

	if (ipc_operation->send_state) {
		if (!yaml_map_add_map("CFG", cfg, yaml_map_populate_cfg, mapping))
			return false;
		if (!yaml_map_add_map("STATE", NULL, yaml_map_populate_state, mapping))
			return false;
	}

	return
		yaml_map_populate_messages(ipc_operation, mapping) &&
		yaml_map_add_int("RC", ipc_operation->rc, mapping);
}

bool yaml_map_populate_ipc_request(const void *data, int mapping) {
	if (!mapping)
		return false;

	if (!data)
		return true;

	const struct IpcRequest *ipc_request = data;

	yaml_map_add_str("OP", ipc_command_name(ipc_request->command), mapping);

	if (ipc_request->log_threshold)
		yaml_map_add_str("LOG_THRESHOLD", log_threshold_name(ipc_request->log_threshold), mapping);

	yaml_map_add_map("CFG", ipc_request->cfg, yaml_map_populate_cfg, mapping);

	return true;
}

bool yaml_map_populate_mode(const void *data, int mapping) {
	if (!mapping)
		return false;

	if (!data)
		return true;

	const struct Mode *mode = data;

	return
		yaml_map_add_int ("WIDTH",       mode->width,       mapping) &&
		yaml_map_add_int ("HEIGHT",      mode->height,      mapping) &&
		yaml_map_add_int ("REFRESH_MHZ", mode->refresh_mhz, mapping) &&
		yaml_map_add_bool("PREFERRED",   mode->preferred,   mapping);
}

bool yaml_map_populate_head_state(const void *data, int mapping) {
	if (!mapping)
		return false;

	if (!data)
		return true;

	const struct HeadState *head_state = data;

	return
		yaml_map_add_float("SCALE",     wl_fixed_to_double(head_state->scale),                                          mapping) &&
		yaml_map_add_bool ("ENABLED",   head_state->enabled,                                                            mapping) &&
		yaml_map_add_int  ("X",         head_state->x,                                                                  mapping) &&
		yaml_map_add_int  ("Y",         head_state->y,                                                                  mapping) &&
		yaml_map_add_bool ("VRR",       (head_state->adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED), mapping) &&
		yaml_map_add_enum ("TRANSFORM", head_state->transform,                                          transform_name, mapping) &&
		yaml_map_add_map  ("MODE",      head_state->mode,                                       yaml_map_populate_mode, mapping);
}

bool yaml_map_populate_head_overrides(const void *data, int mapping) {
	if (!mapping)
		return false;

	if (!data)
		return true;

	const struct Head *head = data;

	if (head->overrided_enabled != NoOverride)
		return yaml_map_add_bool("DISABLED", head->overrided_enabled == OverrideTrue, mapping);
	else
		return true;
}

bool yaml_map_populate_lid(const void *data, int mapping) {
	if (!mapping)
		return false;

	if (!data)
		return true;

	const struct Lid *lid = data;

	return
		yaml_map_add_bool("CLOSED",      lid->closed,      mapping) &&
		yaml_map_add_str ("DEVICE_PATH", lid->device_path, mapping);
}

bool yaml_map_populate_messages(void *data, int mapping) {
	if (!mapping)
		return false;

	if (!data)
		return true;

	struct IpcOperation *ipc_operation = data;

	int seq_lines = 0;

	for (struct SList *i = ipc_operation->log_cap_lines; i; i = i->nex) {
		struct LogCapLine *cap_line = (struct LogCapLine*)i->val;

		if (!cap_line || !cap_line->line || cap_line->threshold < ipc_operation->request->log_threshold)
			continue;

		if (!seq_lines && !(seq_lines = yaml_document_add_sequence(marshal_ctx.doc, NULL, YAML_BLOCK_SEQUENCE_STYLE)))
			return false;

		if (!yaml_seq_append_log_cap_line(cap_line, seq_lines))
			return false;

		// mutate rc here as this is the only place we are processing lines
		if (cap_line->threshold == WARNING && ipc_operation->rc < IPC_RC_WARN)
			ipc_operation->rc = IPC_RC_WARN;
		if (cap_line->threshold == ERROR && ipc_operation->rc < IPC_RC_ERROR)
			ipc_operation->rc = IPC_RC_ERROR;
	}

	if (seq_lines) {
		int key = yaml_document_add_scalar(marshal_ctx.doc, NULL, (yaml_char_t *)"MESSAGES", -1, YAML_PLAIN_SCALAR_STYLE);
		return key && yaml_document_append_mapping_pair(marshal_ctx.doc, mapping, key, seq_lines);
	} else {
		return true;
	}
}

bool yaml_map_populate_state(const void *unused, int mapping) {
	if (!mapping)
		return false;

	return
		yaml_map_add_map("LID",   lid,   yaml_map_populate_lid, mapping) &&
		yaml_map_add_seq("HEADS", heads, yaml_seq_append_head,  mapping);
}

bool yaml_seq_append_user_scale(const void *data, int sequence) {
	if (!sequence)
		return false;

	if (!data)
		return true;

	const struct UserScale *user_scale = data;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		yaml_map_add_str("NAME_DESC", user_scale->name_desc, map) &&
		yaml_map_add_float("SCALE", user_scale->scale, map) &&
		yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

bool yaml_seq_append_user_mode(const void *data, int sequence) {
	if (!sequence)
		return false;

	if (!data)
		return true;

	const struct UserMode *user_mode = data;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (!map || !yaml_map_add_str("NAME_DESC", user_mode->name_desc, map))
		return false;

	if (user_mode->max) {
		if (!yaml_map_add_bool("MAX", user_mode->max, map)) {
			return false;
		}
	} else {
		if (!yaml_map_add_int("WIDTH", user_mode->width, map) || !yaml_map_add_int("HEIGHT", user_mode->height, map))
			return false;
		if (user_mode->refresh_mhz != -1) {
			if (!yaml_map_add_str("HZ", mhz_to_hz_str(user_mode->refresh_mhz), map)) {
				return false;
			}
		}
	}

	return yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

bool yaml_seq_append_user_transform(const void *data, int sequence) {
	if (!sequence)
		return false;

	if (!data)
		return true;

	const struct UserTransform *user_transform = data;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		yaml_map_add_str("NAME_DESC", user_transform->name_desc, map) &&
		yaml_map_add_str("TRANSFORM", transform_name(user_transform->transform), map) &&
		yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

bool yaml_seq_append_condition(const void *data, int sequence) {
	if (!sequence)
		return false;

	if (!data)
		return true;

	const struct Condition *condition = data;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	return
		map &&
		yaml_map_add_seq("PLUGGED", condition->plugged, yaml_seq_append_str, map) &&
		yaml_map_add_seq("UNPLUGGED", condition->unplugged, yaml_seq_append_str, map) &&
		yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

bool yaml_seq_append_disabled(const void *data, int sequence) {
	if (!sequence)
		return false;

	if (!data)
		return true;

	const struct Disabled *disabled = data;

	if (disabled->conditions) {
		int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

		return map &&
			yaml_map_add_str("NAME_DESC", disabled->name_desc, map) &&
			yaml_map_add_seq("IF", disabled->conditions, yaml_seq_append_condition, map) &&
			yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
	} else {
		return yaml_seq_append_str(disabled->name_desc, sequence);
	}
}

bool yaml_seq_append_mode(const void *data, int sequence) {
	if (!sequence)
		return false;

	if (!data)
		return true;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		yaml_map_populate_mode(data, map) &&
		yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

bool yaml_seq_append_head(const void *data, int sequence) {
	if (!sequence)
		return false;

	if (!data)
		return true;

	const struct Head *head = data;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	return
		map &&
		yaml_map_add_str("NAME",          head->name,                                        map) &&
		yaml_map_add_str("DESCRIPTION",   head->description,                                 map) &&
		yaml_map_add_str("MAKE",          head->make,                                        map) &&
		yaml_map_add_str("MODEL",         head->model,                                       map) &&
		yaml_map_add_str("SERIAL_NUMBER", head->serial_number,                               map) &&
		yaml_map_add_int("WIDTH_MM",      head->width_mm,                                    map) &&
		yaml_map_add_int("HEIGHT_MM",     head->height_mm,                                   map) &&
		yaml_map_add_map("CURRENT",       &head->current,  yaml_map_populate_head_state,     map) &&
		yaml_map_add_map("DESIRED",       &head->desired,  yaml_map_populate_head_state,     map) &&
		yaml_map_add_map("OVERRIDES",     head,            yaml_map_populate_head_overrides, map) &&
		yaml_map_add_seq("MODES",         head->modes,     yaml_seq_append_mode,             map) &&
		yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}

bool yaml_seq_append_log_cap_line(const void *data, int sequence) {
	if (!sequence)
		return false;

	if (!data)
		return true;

	const struct LogCapLine *line = data;

	int map = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		yaml_map_add_str(log_threshold_name(line->threshold), line->line, map) &&
		yaml_document_append_sequence_item(marshal_ctx.doc, sequence, map);
}


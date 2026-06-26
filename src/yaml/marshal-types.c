#include <stdbool.h>
#include <stddef.h>
#include <wayland-util.h>
#include <yaml.h>

#include "yaml/marshal-types.h"

#include "cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "cfg/user-mode.h"
#include "convert.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml/marshal-primitives.h"
#include "yaml/marshal.h"

bool yaml_doc_cfg(struct MC *c, const struct Cfg* const cfg) {
	if (!cfg)
		return true;

	int mapping = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

	return mapping && yaml_map_populate_cfg(c, cfg, mapping);
}

bool yaml_doc_ipc_operation(struct MC *c, struct IpcOperation* const ipc_operation) {
	if (!ipc_operation)
		return true;

	// root sequence when not GET
	int seq = 0;
	if (ipc_operation->request->command != GET) {
		if (!(seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE)))
			return false;
	}

	// root map for GET, otherwise append the map to seq
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (!map || !yaml_map_populate_ipc_operation(c, ipc_operation, map))
		return false;

	if (seq)
		return yaml_document_append_sequence_item(&c->d, seq, map);
	else
		return true;
}

bool yaml_doc_ipc_request  (struct MC *c, const struct IpcRequest* const ipc_request) {
	if (!ipc_request)
		return true;

	if (!ipc_command_name(ipc_request->command)) {
		log_error("unable to marshal ipc request: missing OP");
		return false;
	}

	int root = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

	return root && yaml_map_populate_ipc_request(c, ipc_request, root);
}

bool yaml_map_populate_cfg(struct MC *c, const struct Cfg* const cfg, int mapping) {
	if (!mapping)
		return false;

	if (!cfg)
		return true;

	return
		yaml_map_add_enum     (c, cfg_element_name(ARRANGE),               cfg->arrange,               arrange_name,                                       mapping) &&
		yaml_map_add_enum     (c, cfg_element_name(ALIGN),                 cfg->align,                 align_name,                                         mapping) &&

		yaml_map_add_seq_sset (c, cfg_element_name(ORDER),                 cfg->order_name_desc,       yaml_seq_append_str,                                mapping) &&

		yaml_map_add_enum     (c, cfg_element_name(SCALING),               cfg->scaling,               on_off_name,                                        mapping) &&
		yaml_map_add_enum     (c, cfg_element_name(SCALE_ROUND_TO),        cfg->scale_round_to,        scale_round_to_name,                                mapping) &&
		yaml_map_add_enum     (c, cfg_element_name(SCALE_ROUND_STRATEGY),  cfg->scale_round_strategy,  scale_round_strategy_name,                          mapping) &&
		yaml_map_add_enum     (c, cfg_element_name(AUTO_SCALE),            cfg->auto_scale,            on_off_name,                                        mapping) &&
		yaml_map_add_int_nz   (c, cfg_element_name(AUTO_SCALE_DPI),        cfg->auto_scale_dpi,                                                            mapping) &&
		yaml_map_add_float_nz (c, cfg_element_name(AUTO_SCALE_MIN),        cfg->auto_scale_min,                                                            mapping) &&
		yaml_map_add_float_nz (c, cfg_element_name(AUTO_SCALE_MAX),        cfg->auto_scale_max,                                                            mapping) &&

		yaml_map_add_seq_smapi(c, cfg_element_name(SCALE),                 cfg->user_scales,           (fn_yaml_seq_app_ki)yaml_seq_append_user_scale,     mapping) &&
		yaml_map_add_seq_smap (c, cfg_element_name(MODE),                  cfg->user_modes,            (fn_yaml_seq_app_kv)yaml_seq_append_user_mode,      mapping) &&
		yaml_map_add_seq_smapi(c, cfg_element_name(TRANSFORM),             cfg->user_transforms,       (fn_yaml_seq_app_ki)yaml_seq_append_user_transform, mapping) &&
		yaml_map_add_seq_sset (c, cfg_element_name(VRR_OFF),               cfg->adaptive_sync_off,     yaml_seq_append_str,                                mapping) &&

		yaml_map_add_str      (c, cfg_element_name(CALLBACK_CMD),          cfg->callback_cmd,                                                              mapping) &&
		yaml_map_add_str      (c, cfg_element_name(LAPTOP_DISPLAY_PREFIX), cfg->laptop_display_prefix,                                                     mapping) &&
		yaml_map_add_enum     (c, cfg_element_name(LAPTOP_LID_MONITOR),    cfg->laptop_lid_monitor,    on_off_name,                                        mapping) &&
		yaml_map_add_enum     (c, cfg_element_name(LOG_THRESHOLD),         cfg->log_threshold,         log_threshold_name,                                 mapping) &&

		yaml_map_add_seq_pset (c, cfg_element_name(DISABLED),              cfg->disableds,             (fn_yaml_seq_app_v)yaml_seq_append_disabled,        mapping);
}

bool yaml_map_populate_ipc_operation(struct MC *c, struct IpcOperation* const ipc_operation, int mapping) {
	if (!mapping)
		return false;

	if (!ipc_operation)
		return true;

	if (!yaml_map_add_bool(c, "DONE", ipc_operation->done, mapping))
		return false;

	if (ipc_operation->send_state) {
		if (!yaml_map_add_map(c, "CFG", g_cfg, (fn_yaml_map_pop)yaml_map_populate_cfg, mapping))
			return false;
		if (!yaml_map_add_map(c, "STATE", g_heads, yaml_map_populate_state, mapping))
			return false;
	}

	return
		yaml_map_populate_messages(c, (struct IpcOperation*)ipc_operation, mapping) &&
		yaml_map_add_int(c, "RC", ipc_operation->rc, mapping);
}

bool yaml_map_populate_ipc_request(struct MC *c, const struct IpcRequest* const ipc_request, int mapping) {
	if (!mapping)
		return false;

	if (!ipc_request)
		return true;

	yaml_map_add_str(c, "OP", ipc_command_name(ipc_request->command), mapping);

	if (ipc_request->log_threshold)
		yaml_map_add_str(c, "LOG_THRESHOLD", log_threshold_name(ipc_request->log_threshold), mapping);

	yaml_map_add_map(c, "CFG", ipc_request->cfg, (fn_yaml_map_pop)yaml_map_populate_cfg, mapping);

	return true;
}

bool yaml_map_populate_mode(struct MC *c, const struct Mode* const mode, int mapping) {
	if (!mapping)
		return false;

	if (!mode)
		return true;

	return
		yaml_map_add_int (c, "WIDTH",       mode->width,       mapping) &&
		yaml_map_add_int (c, "HEIGHT",      mode->height,      mapping) &&
		yaml_map_add_int (c, "REFRESH_MHZ", mode->refresh_mhz, mapping) &&
		yaml_map_add_bool(c, "PREFERRED",   mode->preferred,   mapping);
}

bool yaml_map_populate_head_state(struct MC *c, const struct HeadState* const head_state, int mapping) {
	if (!mapping)
		return false;

	if (!head_state)
		return true;

	bool adaptive_sync_enabled = head_state->adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	return
		yaml_map_add_float_nz(c, "SCALE",     wl_fixed_to_double(head_state->scale), mapping) &&
		yaml_map_add_bool    (c, "ENABLED",   head_state->enabled,                   mapping) &&
		yaml_map_add_int     (c, "X",         head_state->x,                         mapping) &&
		yaml_map_add_int     (c, "Y",         head_state->y,                         mapping) &&
		yaml_map_add_bool    (c, "VRR",       adaptive_sync_enabled,                 mapping) &&
		yaml_map_add_enum    (c, "TRANSFORM", head_state->transform, transform_name, mapping) &&

		yaml_map_add_map     (c, "MODE", head_state->mode, (fn_yaml_map_pop)yaml_map_populate_mode, mapping);
}

bool yaml_map_populate_head_overrides(struct MC *c, const struct Head* const head, int mapping) {
	if (!mapping)
		return false;

	if (!head)
		return true;

	if (head->overrided_enabled != NoOverride)
		return yaml_map_add_bool(c, "DISABLED", head->overrided_enabled == OverrideTrue, mapping);
	else
		return true;
}

bool yaml_map_populate_lid(struct MC *c, const struct Lid* const lid, int mapping) {
	if (!mapping)
		return false;

	if (!lid)
		return true;

	return
		yaml_map_add_bool(c, "CLOSED",      lid->closed,      mapping) &&
		yaml_map_add_str (c, "DEVICE_PATH", lid->device_path, mapping);
}

bool yaml_map_populate_messages(struct MC *c, struct IpcOperation* const ipc_operation, int mapping) {
	if (!mapping)
		return false;

	if (!ipc_operation)
		return true;

	int seq_lines = 0;

	for (struct SList *i = ipc_operation->log_cap_lines; i; i = i->nex) {
		const struct LogCapLine *cap_line = (struct LogCapLine*)i->val;

		if (!cap_line || !cap_line->line || cap_line->threshold < ipc_operation->request->log_threshold)
			continue;

		if (!seq_lines && !(seq_lines = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE)))
			return false;

		if (!yaml_seq_append_log_cap_line(c, cap_line, seq_lines))
			return false;

		// mutate rc here as this is the only place we are processing lines
		if (cap_line->threshold == WARNING && ipc_operation->rc < IPC_RC_WARN)
			ipc_operation->rc = IPC_RC_WARN;
		if (cap_line->threshold == ERROR && ipc_operation->rc < IPC_RC_ERROR)
			ipc_operation->rc = IPC_RC_ERROR;
	}

	if (seq_lines) {
		int key = yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)"MESSAGES", -1, YAML_PLAIN_SCALAR_STYLE);
		return key && yaml_document_append_mapping_pair(&c->d, mapping, key, seq_lines);
	} else {
		return true;
	}
}

bool yaml_map_populate_state(struct MC *c, const void* const unused, int mapping) {
	if (!mapping)
		return false;

	if (g_lid && !yaml_map_add_map(c, "LID", g_lid, (fn_yaml_map_pop)yaml_map_populate_lid, mapping))
		return false;

	if (g_heads && !yaml_map_add_seq_list(c, "HEADS", g_heads, (fn_yaml_seq_app_v)yaml_seq_append_head,  mapping))
		return false;

	return true;
}

bool yaml_seq_append_user_scale(struct MC *c, const char* const name_desc, const size_t user_scale, int sequence) {
	if (!sequence)
		return false;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		yaml_map_add_str(c, "NAME_DESC", name_desc, map) &&
		yaml_map_add_float_nz(c, "SCALE", (double)user_scale/1000, map) &&
		yaml_document_append_sequence_item(&c->d, sequence, map);
}

bool yaml_seq_append_user_mode (struct MC *c, const char* const name_desc, const struct UserMode* const user_mode, int sequence) {
	if (!sequence)
		return false;

	if (!user_mode)
		return true;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

	if (!map || !yaml_map_add_str(c, "NAME_DESC", name_desc, map))
		return false;

	if (user_mode->max) {
		if (!yaml_map_add_bool(c, "MAX", user_mode->max, map)) {
			return false;
		}
	} else {
		if (!yaml_map_add_int(c, "WIDTH", user_mode->width, map) || !yaml_map_add_int(c, "HEIGHT", user_mode->height, map))
			return false;
		if (user_mode->refresh_mhz != -1) {
			if (!yaml_map_add_str(c, "HZ", mhz_to_hz_str(user_mode->refresh_mhz), map)) {
				return false;
			}
		}
	}

	return yaml_document_append_sequence_item(&c->d, sequence, map);
}

bool yaml_seq_append_user_transform(struct MC *c, const char* const name_desc, const size_t transform, int sequence) {
	if (!sequence)
		return false;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		yaml_map_add_str(c, "NAME_DESC", name_desc, map) &&
		yaml_map_add_str(c, "TRANSFORM", transform_name(transform), map) &&
		yaml_document_append_sequence_item(&c->d, sequence, map);
}

bool yaml_seq_append_condition(struct MC *c, const struct Condition* const condition, int sequence) {
	if (!sequence)
		return false;

	if (!condition)
		return true;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

	return
		map &&
		yaml_map_add_seq_sset(c, "PLUGGED", condition->plugged, yaml_seq_append_str, map) &&
		yaml_map_add_seq_sset(c, "UNPLUGGED", condition->unplugged, yaml_seq_append_str, map) &&
		yaml_map_add_enum(c, "LID", condition->lid, condition_lid_name, map) &&
		yaml_document_append_sequence_item(&c->d, sequence, map);
}

bool yaml_seq_append_disabled(struct MC *c, const struct Disabled* const disabled, int sequence) {
	if (!sequence)
		return false;

	if (!disabled)
		return true;

	if (pset_size(disabled->conditions) > 0) {
		int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

		return map &&
			yaml_map_add_str(c, "NAME_DESC", disabled->name_desc, map) &&
			yaml_map_add_seq_pset(c, "IF", disabled->conditions, (fn_yaml_seq_app_v)yaml_seq_append_condition, map) &&
			yaml_document_append_sequence_item(&c->d, sequence, map);
	} else {
		return yaml_seq_append_str(c,disabled->name_desc, sequence);
	}
}

bool yaml_seq_append_mode(struct MC *c, const struct Mode* const mode, int sequence) {
	if (!sequence)
		return false;

	if (!mode)
		return true;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		yaml_map_populate_mode(c, mode, map) &&
		yaml_document_append_sequence_item(&c->d, sequence, map);
}

bool yaml_seq_append_head(struct MC *c, const struct Head* const head, int sequence) {
	if (!sequence)
		return false;

	if (!head)
		return true;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

	return
		map &&
		yaml_map_add_str(c, "NAME",          head->name,          map) &&
		yaml_map_add_str(c, "DESCRIPTION",   head->description,   map) &&
		yaml_map_add_str(c, "MAKE",          head->make,          map) &&
		yaml_map_add_str(c, "MODEL",         head->model,         map) &&
		yaml_map_add_str(c, "SERIAL_NUMBER", head->serial_number, map) &&
		yaml_map_add_int(c, "WIDTH_MM",      head->width_mm,      map) &&
		yaml_map_add_int(c, "HEIGHT_MM",     head->height_mm,     map) &&

		yaml_map_add_map     (c, "CURRENT",   &head->current,  (fn_yaml_map_pop)yaml_map_populate_head_state,     map) &&
		yaml_map_add_map     (c, "DESIRED",   &head->desired,  (fn_yaml_map_pop)yaml_map_populate_head_state,     map) &&
		yaml_map_add_map     (c, "OVERRIDES", head,            (fn_yaml_map_pop)yaml_map_populate_head_overrides, map) &&
		yaml_map_add_seq_list(c, "MODES",     head->modes,     (fn_yaml_map_pop)yaml_seq_append_mode,             map) &&

		yaml_document_append_sequence_item(&c->d, sequence, map);
}

bool yaml_seq_append_log_cap_line(struct MC *c, const struct LogCapLine* const log_cap_line, int sequence) {
	if (!sequence)
		return false;

	if (!log_cap_line)
		return true;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);

	return map &&
		yaml_map_add_str(c, log_threshold_name(log_cap_line->threshold), log_cap_line->line, map) &&
		yaml_document_append_sequence_item(&c->d, sequence, map);
}


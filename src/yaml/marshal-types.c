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

bool yaml_cfg_to_root(struct MC *c, const struct Cfg* const cfg) {

	// creates a mapping node which is the root
	return yaml_cfg_to_map(c, cfg) != 0;
}

bool yaml_ipc_operation_to_root(struct MC *c, const struct IpcOperation* const ipc_operation) {
	if (!ipc_operation)
		return false;

	if (ipc_operation->request->command == GET) {

		// creates a mapping node which is the root
		return yaml_ipc_operation_to_map(c, ipc_operation) != 0;

	} else {

		// create a root sequence with one map item
		int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);
		if (!seq)
			return false;

		int map = yaml_ipc_operation_to_map(c, ipc_operation);
		if (!map)
			return false;

		return yaml_document_append_sequence_item(&c->d, seq, map) != 0;
	}
}

bool yaml_ipc_request_to_root(struct MC *c, const struct IpcRequest* const ipc_request) {
	if (!ipc_request)
		return true;

	if (!ipc_command_name(ipc_request->command)) {
		log_error("unable to marshal ipc request: missing OP");
		return false;
	}

	// creates a mapping node which is the root
	return yaml_ipc_request_to_map(c, ipc_request) != 0;
}

int yaml_cfg_to_map(struct MC *c, const struct Cfg* const cfg) {
	if (!cfg)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	// order is important
	yaml_map_add_enum    (c, cfg_element_name(ARRANGE),               cfg->arrange,               arrange_name,                              map);
	yaml_map_add_enum    (c, cfg_element_name(ALIGN),                 cfg->align,                 align_name,                                map);
	yaml_map_add_sset    (c, cfg_element_name(ORDER),                 cfg->order_name_desc,                                                  map);
	yaml_map_add_enum    (c, cfg_element_name(SCALING),               cfg->scaling,               on_off_name,                               map);
	yaml_map_add_enum    (c, cfg_element_name(SCALE_ROUND_TO),        cfg->scale_round_to,        scale_round_to_name,                       map);
	yaml_map_add_enum    (c, cfg_element_name(SCALE_ROUND_STRATEGY),  cfg->scale_round_strategy,  scale_round_strategy_name,                 map);
	yaml_map_add_enum    (c, cfg_element_name(AUTO_SCALE),            cfg->auto_scale,            on_off_name,                               map);
	yaml_map_add_int_nz  (c, cfg_element_name(AUTO_SCALE_DPI),        cfg->auto_scale_dpi,                                                   map);
	yaml_map_add_float_nz(c, cfg_element_name(AUTO_SCALE_MIN),        cfg->auto_scale_min,                                                   map);
	yaml_map_add_float_nz(c, cfg_element_name(AUTO_SCALE_MAX),        cfg->auto_scale_max,                                                   map);
	yaml_map_add_smapi   (c, cfg_element_name(SCALE),                 cfg->scales,                yaml_scale_to_map,                         map);
	yaml_map_add_smap    (c, cfg_element_name(MODE),                  cfg->user_modes,            (fn_yaml_kv_to_node)yaml_user_mode_to_map, map);
	yaml_map_add_smapi   (c, cfg_element_name(TRANSFORM),             cfg->transforms,            yaml_transform_to_map,                     map);
	yaml_map_add_sset    (c, cfg_element_name(VRR_OFF),               cfg->adaptive_sync_off,                                                map);
	yaml_map_add_str     (c, cfg_element_name(CALLBACK_CMD),          cfg->callback_cmd,                                                     map);
	yaml_map_add_str     (c, cfg_element_name(LAPTOP_DISPLAY_PREFIX), cfg->laptop_display_prefix,                                            map);
	yaml_map_add_enum    (c, cfg_element_name(LAPTOP_LID_MONITOR),    cfg->laptop_lid_monitor,    on_off_name,                               map);
	yaml_map_add_enum    (c, cfg_element_name(LOG_THRESHOLD),         cfg->log_threshold,         log_threshold_name,                        map);
	yaml_map_add_pset    (c, cfg_element_name(DISABLED),              cfg->disableds,             (fn_yaml_v_to_node)yaml_disabled_to_node,  map);

	return map;
}

int yaml_ipc_operation_to_map(struct MC *c, const struct IpcOperation* const ipc_operation) {
	if (!ipc_operation)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_bool(c, "DONE", ipc_operation->done, map);

	if (ipc_operation->send_state) {
		yaml_map_add_node(c, "CFG", yaml_cfg_to_map(c, g_cfg), map);
		yaml_map_add_node(c, "STATE", yaml_state_to_map(c), map);
	}

	yaml_map_add_node(c, "MESSAGES", yaml_messages_to_seq(c, ipc_operation), map);
	yaml_map_add_int(c, "RC", ipc_operation->rc, map);

	return map;
}

int yaml_ipc_request_to_map(struct MC *c, const struct IpcRequest* const ipc_request) {
	if (!ipc_request)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str(c, "OP", ipc_command_name(ipc_request->command), map);

	if (ipc_request->log_threshold)
		yaml_map_add_str(c, "LOG_THRESHOLD", log_threshold_name(ipc_request->log_threshold), map);

	yaml_map_add_node(c, "CFG", yaml_cfg_to_map(c, ipc_request->cfg), map);

	return map;
}

int yaml_head_state_to_map(struct MC *c, const struct HeadState* const head_state) {
	if (!head_state)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	bool adaptive_sync_enabled = head_state->adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	yaml_map_add_float_nz(c, "SCALE",     wl_fixed_to_double(head_state->scale),           map);
	yaml_map_add_bool    (c, "ENABLED",   head_state->enabled,                             map);
	yaml_map_add_int     (c, "X",         head_state->x,                                   map);
	yaml_map_add_int     (c, "Y",         head_state->y,                                   map);
	yaml_map_add_bool    (c, "VRR",       adaptive_sync_enabled,                           map);
	yaml_map_add_enum    (c, "TRANSFORM", head_state->transform, transform_name,           map);
	yaml_map_add_node    (c, "MODE",      yaml_wlr_mode_to_map(c, NULL, head_state->wlr_mode), map);

	return map;
}

int yaml_head_overrides_to_map(struct MC *c, const struct Head* const head) {
	if (!head)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	if (head->overrided_enabled != NoOverride) {
		yaml_map_add_bool(c, "DISABLED", head->overrided_enabled == OverrideTrue, map);
	}

	return map;
}

int yaml_lid_to_map(struct MC *c, const struct Lid* const lid) {
	if (!lid)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_bool(c, "CLOSED", lid->closed, map);
	yaml_map_add_str (c, "DEVICE_PATH", lid->device_path, map);

	return map;
}

int yaml_messages_to_seq(struct MC *c, const struct IpcOperation* const ipc_operation) {
	if (!ipc_operation)
		return 0;

	int seq = 0;
	int map = 0;

	for (struct SList *i = ipc_operation->log_cap_lines; i; i = i->nex) {
		const struct LogCapLine *cap_line = (struct LogCapLine*)i->val;

		if (!cap_line || !cap_line->line || cap_line->threshold < ipc_operation->request->log_threshold)
			continue;

		if (!seq && !(seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE)))
			return 0;

		if (!(map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE)))
			return 0;

		yaml_map_add_str(c, log_threshold_name(cap_line->threshold), cap_line->line, map);

		if (!yaml_document_append_sequence_item(&c->d, seq, map))
			return 0;
	}

	return seq;
}

int yaml_state_to_map(struct MC *c) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	if (g_lid)
		yaml_map_add_node(c, "LID", yaml_lid_to_map(c, g_lid), map);

	if (g_heads)
		yaml_map_add_list(c, "HEADS", g_heads, (fn_yaml_v_to_node)yaml_head_to_map, map);

	return map;
}

int yaml_scale_to_map(struct MC *c, const char* const name_desc, const size_t scale) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str(c, "NAME_DESC", name_desc, map);
	yaml_map_add_float_nz(c, "SCALE", (double)scale/1000, map);

	return map;
}

int yaml_user_mode_to_map(struct MC *c, const char* const name_desc, const struct UserMode* const user_mode) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str(c, "NAME_DESC", name_desc, map);

	if (user_mode->max) {
		yaml_map_add_bool(c, "MAX", user_mode->max, map);
	} else {
		yaml_map_add_int(c, "WIDTH", user_mode->width, map);
		yaml_map_add_int(c, "HEIGHT", user_mode->height, map);
		if (user_mode->refresh_mhz != -1) {
			yaml_map_add_str(c, "HZ", mhz_to_hz_str(user_mode->refresh_mhz), map);
		}
	}

	return map;
}

int yaml_transform_to_map(struct MC *c, const char* const name_desc, const size_t transform) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str(c, "NAME_DESC", name_desc, map);
	yaml_map_add_str(c, "TRANSFORM", transform_name(transform), map);

	return map;
}

int yaml_condition_to_map(struct MC *c, const struct Condition* const condition) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_sset(c, "PLUGGED", condition->plugged, map);
	yaml_map_add_sset(c, "UNPLUGGED", condition->unplugged, map);
	yaml_map_add_enum(c, "LID", condition->lid, condition_lid_name, map);

	return map;
}

int yaml_disabled_to_node(struct MC *c, const struct Disabled* const disabled) {
	if (!disabled || !disabled->name_desc)
		return 0;

	if (pset_size(disabled->conditions) > 0) {
		int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
		if (!map)
			return 0;

		yaml_map_add_str(c, "NAME_DESC", disabled->name_desc, map);
		yaml_map_add_pset(c, "IF", disabled->conditions, (fn_yaml_v_to_node)yaml_condition_to_map, map);

		return map;
	} else {
		return yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)disabled->name_desc, -1, YAML_PLAIN_SCALAR_STYLE);
	}
}

int yaml_wlr_mode_to_map (struct MC *c, const void* const unused, const struct WlrMode* const wlr_mode) {
	if (!wlr_mode)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_int (c, "WIDTH",       wlr_mode->width,       map);
	yaml_map_add_int (c, "HEIGHT",      wlr_mode->height,      map);
	yaml_map_add_int (c, "REFRESH_MHZ", wlr_mode->refresh_mhz, map);
	yaml_map_add_bool(c, "PREFERRED",   wlr_mode->preferred,   map);

	return map;
}

int yaml_head_to_map(struct MC *c, const struct Head* const head) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str(c, "NAME",          head->name,          map);
	yaml_map_add_str(c, "DESCRIPTION",   head->description,   map);
	yaml_map_add_str(c, "MAKE",          head->make,          map);
	yaml_map_add_str(c, "MODEL",         head->model,         map);
	yaml_map_add_str(c, "SERIAL_NUMBER", head->serial_number, map);
	yaml_map_add_int(c, "WIDTH_MM",      head->width_mm,      map);
	yaml_map_add_int(c, "HEIGHT_MM",     head->height_mm,     map);

	yaml_map_add_node(c, "CURRENT",   yaml_head_state_to_map(c, &head->current), map);
	yaml_map_add_node(c, "DESIRED",   yaml_head_state_to_map(c, &head->desired), map);
	yaml_map_add_node(c, "OVERRIDES", yaml_head_overrides_to_map(c, head),       map);

	yaml_map_add_pmap(c, "MODES", head->wlr_modes, (fn_yaml_kv_to_node)yaml_wlr_mode_to_map, map);

	return map;
}


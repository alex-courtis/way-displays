#include <stdbool.h>
#include <stdlib.h>
#include <wayland-util.h>
#include <yaml.h>

#include "yaml/marshal-types.h"

#include "cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "enum.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "pslist.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml/marshal-primitives.h"
#include "yaml/marshal.h"

bool yaml_root_from_cfg(struct MC *c, const struct Cfg* const cfg) {

	// creates a mapping node which is the root
	return yaml_map_from_cfg(c, cfg) != 0;
}

bool yaml_root_from_ipc_operation(struct MC *c, const struct IpcOperation* const ipc_operation) {
	if (!ipc_operation)
		return false;

	if (ipc_operation->request->command == GET) {

		// creates a mapping node which is the root
		return yaml_map_from_ipc_operation(c, ipc_operation) != 0;

	} else {

		// create a root sequence with one map item
		int seq = yaml_document_add_sequence(&c->d, NULL, YAML_BLOCK_SEQUENCE_STYLE);
		if (!seq)
			return false;

		int map = yaml_map_from_ipc_operation(c, ipc_operation);
		if (!map)
			return false;

		return yaml_document_append_sequence_item(&c->d, seq, map) != 0;
	}
}

bool yaml_root_from_ipc_request(struct MC *c, const struct IpcRequest* const ipc_request) {
	if (!ipc_request)
		return true;

	if (!ipc_command_name(ipc_request->command)) {
		log_error("unable to marshal ipc request: missing OP");
		return false;
	}

	// creates a mapping node which is the root
	return yaml_map_from_ipc_request(c, ipc_request) != 0;
}

int yaml_map_from_cfg(struct MC *c, const struct Cfg* const cfg) {
	if (!cfg)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	// order is important
	yaml_map_add_enum    (c, cfg_element_name(ARRANGE),               cfg->arrange,               arrange_name,                                         map);
	yaml_map_add_enum    (c, cfg_element_name(ALIGN),                 cfg->align,                 align_name,                                           map);
	yaml_map_add_sset    (c, cfg_element_name(ORDER),                 cfg->order_name_desc,                                                             map);
	yaml_map_add_enum    (c, cfg_element_name(SCALING),               cfg->scaling,               on_off_name,                                          map);
	yaml_map_add_enum    (c, cfg_element_name(SCALE_ROUND_TO),        cfg->scale_round_to,        scale_round_to_name,                                  map);
	yaml_map_add_enum    (c, cfg_element_name(SCALE_ROUND_STRATEGY),  cfg->scale_round_strategy,  scale_round_strategy_name,                            map);
	yaml_map_add_enum    (c, cfg_element_name(AUTO_SCALE),            cfg->auto_scale,            on_off_name,                                          map);
	yaml_map_add_int_nz  (c, cfg_element_name(AUTO_SCALE_DPI),        cfg->auto_scale_dpi,                                                              map);
	yaml_map_add_float_nz(c, cfg_element_name(AUTO_SCALE_MIN),        cfg->auto_scale_min,                                                              map);
	yaml_map_add_float_nz(c, cfg_element_name(AUTO_SCALE_MAX),        cfg->auto_scale_max,                                                              map);
	yaml_map_add_simap   (c, cfg_element_name(SCALE),                 cfg->scales,                yaml_map_from_scale,                                  map);
	yaml_map_add_spmap   (c, cfg_element_name(MODE),                  cfg->modes,                 (fn_yaml_node_from_key_type)yaml_map_from_named_mode, map);
	yaml_map_add_simap   (c, cfg_element_name(TRANSFORM),             cfg->transforms,            yaml_map_from_transform,                              map);
	yaml_map_add_sset    (c, cfg_element_name(VRR_OFF),               cfg->adaptive_sync_off,                                                           map);
	yaml_map_add_str     (c, cfg_element_name(CALLBACK_CMD),          cfg->callback_cmd,                                                                map);
	yaml_map_add_str     (c, cfg_element_name(LAPTOP_DISPLAY_PREFIX), cfg->laptop_display_prefix,                                                       map);
	yaml_map_add_enum    (c, cfg_element_name(LAPTOP_LID_MONITOR),    cfg->laptop_lid_monitor,    on_off_name,                                          map);
	yaml_map_add_enum    (c, cfg_element_name(LOG_THRESHOLD),         cfg->log_threshold,         log_threshold_name,                                   map);
	yaml_map_add_pset    (c, cfg_element_name(DISABLED),              cfg->disableds,             (fn_yaml_node_from_type)yaml_node_from_disabled,      map);

	return map;
}

int yaml_map_from_ipc_operation(struct MC *c, const struct IpcOperation* const ipc_operation) {
	if (!ipc_operation)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_bool(c, "DONE", ipc_operation->done, map);

	if (ipc_operation->send_state) {
		yaml_map_add_node(c, "CFG", yaml_map_from_cfg(c, g_cfg), map);
		yaml_map_add_node(c, "STATE", yaml_map_from_state(c), map);
	}

	yaml_map_add_node(c, "MESSAGES", yaml_seq_from_messages(c, ipc_operation), map);
	yaml_map_add_int(c, "RC", ipc_operation->rc, map);

	return map;
}

int yaml_map_from_ipc_request(struct MC *c, const struct IpcRequest* const ipc_request) {
	if (!ipc_request)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str(c, "OP", ipc_command_name(ipc_request->command), map);

	if (ipc_request->log_threshold)
		yaml_map_add_str(c, "LOG_THRESHOLD", log_threshold_name(ipc_request->log_threshold), map);

	yaml_map_add_node(c, "CFG", yaml_map_from_cfg(c, ipc_request->cfg), map);

	return map;
}

int yaml_map_from_head_state(struct MC *c, const struct HeadState* const head_state) {
	if (!head_state)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	bool adaptive_sync_enabled = head_state->adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	yaml_map_add_float_nz(c, "SCALE",     wl_fixed_to_double(head_state->scale),   map);
	yaml_map_add_bool    (c, "ENABLED",   head_state->enabled,                     map);
	yaml_map_add_int     (c, "X",         head_state->x,                           map);
	yaml_map_add_int     (c, "Y",         head_state->y,                           map);
	yaml_map_add_bool    (c, "VRR",       adaptive_sync_enabled,                   map);
	yaml_map_add_enum    (c, "TRANSFORM", head_state->transform, transform_name,   map);
	yaml_map_add_node    (c, "MODE",      yaml_map_from_mode(c, head_state->mode), map);

	return map;
}

int yaml_map_from_head_overrides(struct MC *c, const struct Head* const head) {
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

int yaml_map_from_lid(struct MC *c, const struct Lid* const lid) {
	if (!lid)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_bool(c, "CLOSED", lid->closed, map);
	yaml_map_add_str (c, "DEVICE_PATH", lid->device_path, map);

	return map;
}

int yaml_seq_from_messages(struct MC *c, const struct IpcOperation* const ipc_operation) {
	if (!ipc_operation)
		return 0;

	int seq = 0;
	int map = 0;

	for (struct Pslist *i = ipc_operation->log_cap_lines; i; i = i->nex) {
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

int yaml_map_from_state(struct MC *c) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	if (g_lid)
		yaml_map_add_node(c, "LID", yaml_map_from_lid(c, g_lid), map);

	if (g_heads)
		yaml_map_add_pslist(c, "HEADS", g_heads, (fn_yaml_node_from_type)yaml_map_from_head, map);

	return map;
}

int yaml_map_from_scale(struct MC *c, const char* const name_desc, const size_t scale) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str(c, "NAME_DESC", name_desc, map);
	yaml_map_add_float_nz(c, "SCALE", (double)scale/1000, map);

	return map;
}

int yaml_map_from_named_mode(struct MC *c, const char* const name_desc, const struct Mode* const mode) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str(c, "NAME_DESC", name_desc, map);

	if (mode->max) {
		yaml_map_add_bool(c, "MAX", mode->max, map);
	} else {
		yaml_map_add_int(c, "WIDTH", mode->width, map);
		yaml_map_add_int(c, "HEIGHT", mode->height, map);
		if (mode->refresh_mhz != -1) {
			char *hz = sprintf_alloc("%g", ((float)mode->refresh_mhz) / 1000);
			yaml_map_add_str(c, "HZ", hz, map);
			free(hz);
		}
	}

	return map;
}

int yaml_map_from_transform(struct MC *c, const char* const name_desc, const size_t transform) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str(c, "NAME_DESC", name_desc, map);
	yaml_map_add_str(c, "TRANSFORM", transform_name(transform), map);

	return map;
}

int yaml_map_from_condition(struct MC *c, const struct Condition* const condition) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_sset(c, "PLUGGED", condition->plugged, map);
	yaml_map_add_sset(c, "UNPLUGGED", condition->unplugged, map);
	yaml_map_add_enum(c, "LID", condition->lid, condition_lid_name, map);

	return map;
}

int yaml_node_from_disabled(struct MC *c, const struct Disabled* const disabled) {
	if (!disabled || !disabled->name_desc)
		return 0;

	if (pset_size(disabled->conditions) > 0) {
		int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
		if (!map)
			return 0;

		yaml_map_add_str(c, "NAME_DESC", disabled->name_desc, map);
		yaml_map_add_pset(c, "IF", disabled->conditions, (fn_yaml_node_from_type)yaml_map_from_condition, map);

		return map;
	} else {
		return yaml_document_add_scalar(&c->d, NULL, (yaml_char_t *)disabled->name_desc, -1, YAML_PLAIN_SCALAR_STYLE);
	}
}

int yaml_map_from_mode(struct MC *c, const struct Mode* const mode) {
	if (!mode)
		return 0;

	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_int (c, "WIDTH",       mode->width,       map);
	yaml_map_add_int (c, "HEIGHT",      mode->height,      map);
	yaml_map_add_int (c, "REFRESH_MHZ", mode->refresh_mhz, map);

	return map;
}

int yaml_map_from_head(struct MC *c, const struct Head* const head) {
	int map = yaml_document_add_mapping(&c->d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str(c, "NAME",            head->name,                                                     map);
	yaml_map_add_str(c, "DESCRIPTION",     head->description,                                              map);
	yaml_map_add_str(c, "MAKE",            head->make,                                                     map);
	yaml_map_add_str(c, "MODEL",           head->model,                                                    map);
	yaml_map_add_str(c, "SERIAL_NUMBER",   head->serial_number,                                            map);
	yaml_map_add_int(c, "WIDTH_MM",        head->width_mm,                                                 map);
	yaml_map_add_int(c, "HEIGHT_MM",       head->height_mm,                                                map);

	yaml_map_add_node(c, "CURRENT",        yaml_map_from_head_state(c, &head->current),                    map);
	yaml_map_add_node(c, "DESIRED",        yaml_map_from_head_state(c, &head->desired),                    map);
	yaml_map_add_node(c, "OVERRIDES",      yaml_map_from_head_overrides(c, head),                          map);
	yaml_map_add_node(c, "MODE_PREFERRED", yaml_map_from_mode(c, head->mode_preferred),                    map);

	yaml_map_add_pset(c, "MODES",          head->modes,        (fn_yaml_node_from_type)yaml_map_from_mode, map);
	yaml_map_add_pset(c, "MODES_FAILED",   head->modes_failed, (fn_yaml_node_from_type)yaml_map_from_mode, map);

	return map;
}


#include <stdbool.h>
#include <stdlib.h>
#include <wayland-util.h>
#include <yaml.h>

#include "yaml/marshal-types.h"

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "displ.h"
#include "enum.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "plist.h"
#include "ppmap.h"
#include "pset.h"
#include "simap.h"
#include "spmap.h"
#include "sset.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml/marshal-primitives.h"
#include "yaml/marshal.h"

bool yaml_root_from_cfg(const struct Cfg* const cfg) {

	// creates a mapping node which is the root
	return yaml_map_from_cfg(cfg) != 0;
}

bool yaml_root_from_ipc_operation(const struct IpcOperation* const ipc_operation) {
	if (!ipc_operation)
		return false;

	if (ipc_operation->request->command == GET) {

		// creates a mapping node which is the root
		return yaml_map_from_ipc_operation(ipc_operation) != 0;

	} else {

		// create a root sequence with one map item
		int seq = yaml_document_add_sequence(&mc.d, NULL, YAML_BLOCK_SEQUENCE_STYLE);
		if (!seq)
			return false;

		int map = yaml_map_from_ipc_operation(ipc_operation);
		if (!map)
			return false;

		return yaml_document_append_sequence_item(&mc.d, seq, map) != 0;
	}
}

bool yaml_root_from_ipc_request(const struct IpcRequest* const ipc_request) {
	if (!ipc_request)
		return true;

	if (!ipc_command_name(ipc_request->command)) {
		log_error("unable to marshal ipc request: missing OP");
		return false;
	}

	// creates a mapping node which is the root
	return yaml_map_from_ipc_request(ipc_request) != 0;
}

int yaml_map_from_ipc_operation(const struct IpcOperation* const ipc_operation) {
	if (!ipc_operation)
		return 0;

	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_bool("DONE", ipc_operation->done, map);

	if (ipc_operation->send_state) {
		yaml_map_add_node("CFG", yaml_map_from_cfg(g_cfg), map);
		yaml_map_add_node("STATE", yaml_map_from_state(), map);
	}

	yaml_map_add_node("MESSAGES", yaml_seq_from_messages(ipc_operation), map);
	yaml_map_add_int("RC", ipc_operation->rc, map);

	return map;
}

int yaml_map_from_ipc_request(const struct IpcRequest* const ipc_request) {
	if (!ipc_request)
		return 0;

	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str("OP", ipc_command_name(ipc_request->command), map);

	if (ipc_request->log_threshold)
		yaml_map_add_str("LOG_THRESHOLD", log_threshold_name(ipc_request->log_threshold), map);

	yaml_map_add_node("CFG", yaml_map_from_cfg(ipc_request->cfg), map);

	return map;
}

int yaml_map_from_cfg(const struct Cfg* const cfg) {
	if (!cfg)
		return 0;

	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	// order is important
	yaml_map_add_enum    (cfg_element_name(ARRANGE),              cfg->arrange,               arrange_name,              map);
	yaml_map_add_enum    (cfg_element_name(ALIGN),                cfg->align,                 align_name,                map);
	yaml_map_add_sset    (cfg_element_name(ORDER),                cfg->order_name_desc,                                  map);
	yaml_map_add_enum    (cfg_element_name(SCALING),              cfg->scaling,               on_off_name,               map);
	yaml_map_add_enum    (cfg_element_name(SCALE_ROUND_TO),       cfg->scale_round_to,        scale_round_to_name,       map);
	yaml_map_add_enum    (cfg_element_name(SCALE_ROUND_STRATEGY), cfg->scale_round_strategy,  scale_round_strategy_name, map);
	yaml_map_add_enum    (cfg_element_name(AUTO_SCALE),           cfg->auto_scale,            on_off_name,               map);
	yaml_map_add_int_nz  (cfg_element_name(AUTO_SCALE_DPI),       cfg->auto_scale_dpi,                                   map);
	yaml_map_add_float_nz(cfg_element_name(AUTO_SCALE_MIN),       cfg->auto_scale_min,                                   map);
	yaml_map_add_float_nz(cfg_element_name(AUTO_SCALE_MAX),       cfg->auto_scale_max,                                   map);
	yaml_map_add_node    (cfg_element_name(SCALE),                yaml_map_from_scales(cfg->scales),                     map);
	yaml_map_add_node    (cfg_element_name(MODE),                 yaml_map_from_cfg_modes(cfg->modes),                   map);
	yaml_map_add_node    (cfg_element_name(TRANSFORM),            yaml_map_from_transforms(cfg->transforms),             map);
	yaml_map_add_sset    (cfg_element_name(VRR_OFF),              cfg->adaptive_sync_off,                                map);
	yaml_map_add_str     (cfg_element_name(CALLBACK_CMD),         cfg->callback_cmd,                                     map);
	yaml_map_add_enum    (cfg_element_name(LAPTOP_LID_MONITOR),   cfg->laptop_lid_monitor,    on_off_name,               map);
	yaml_map_add_enum    (cfg_element_name(LOG_THRESHOLD),        cfg->log_threshold,         log_threshold_name,        map);
	yaml_map_add_node    (cfg_element_name(DISABLED),             yaml_map_from_disableds(cfg->disableds),               map);

	return map;
}

int yaml_map_from_cfg_modes(const struct SPmap* const modes) {
	if (!modes || spmap_size(modes) == 0) {
		return yaml_document_add_scalar(&mc.d, NULL, (yaml_char_t*)"", 0, YAML_PLAIN_SCALAR_STYLE);
	}

	int map_out = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map_out) {
		return 0;
	}

	for (const struct SPmapIt *it = spmap_it(modes); it; it = spmap_it_next(it)) {
		const struct Mode *mode = it->val;

		int map_in = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
		if (!map_in)
			continue;

		if (mode->max) {
			yaml_map_add_bool("MAX", mode->max, map_in);
		} else if (mode->max_preferred_refresh) {
			yaml_map_add_bool("MAX_PREFERRED_REFRESH", mode->max_preferred_refresh, map_in);
		} else {
			yaml_map_add_int("WIDTH", mode->width, map_in);
			yaml_map_add_int("HEIGHT", mode->height, map_in);
			if (mode->refresh_mhz != -1) {
				char *hz = sprintf_alloc("%g", ((float)mode->refresh_mhz) / 1000);
				yaml_map_add_str("HZ", hz, map_in);
				free(hz);
			}
		}

		yaml_map_add_node(it->key, map_in, map_out);
	}

	return map_out;
}

int yaml_map_from_disableds(const struct SPmap* const disableds) {
	if (!disableds || spmap_size(disableds) == 0) {
		return yaml_document_add_scalar(&mc.d, NULL, (yaml_char_t*)"", 0, YAML_PLAIN_SCALAR_STYLE);
	}

	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map) {
		return 0;
	}

	for (const struct SPmapIt *it = spmap_it(disableds); it; it = spmap_it_next(it)) {
		const struct CfgDisabled *disabled = it->val;
		if (pset_size(disabled->conditions) > 0 ) {
			int map_if = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
			if (!map_if)
				continue;

			const struct Plist *conditions = pset_plist(disabled->conditions);
			yaml_map_add_plist("IF", conditions, (fn_yaml_node_from_type)yaml_map_from_condition, map_if);
			plist_free(conditions);

			yaml_map_add_node(it->key, map_if, map);
		} else {
			yaml_map_add_str(it->key, "", map);
		}
	}

	return map;
}

int yaml_map_from_head_overrides(const struct Head* const head) {
	if (!head)
		return 0;

	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	if (head->overrided_enabled != NoOverride) {
		yaml_map_add_bool("DISABLED", head->overrided_enabled == OverrideTrue, map);
	}

	return map;
}

int yaml_map_from_head_state(const struct HeadState* const head_state, const struct Head* const head) {
	if (!head_state)
		return 0;

	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	bool adaptive_sync_enabled = head_state->adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	yaml_map_add_float_nz("SCALE",     wl_fixed_to_double(head_state->scale),         map);
	yaml_map_add_bool    ("ENABLED",   head_state->enabled,                           map);
	yaml_map_add_int     ("X",         head_state->x,                                 map);
	yaml_map_add_int     ("Y",         head_state->y,                                 map);
	yaml_map_add_bool    ("VRR",       adaptive_sync_enabled,                         map);
	yaml_map_add_enum    ("TRANSFORM", head_state->transform, transform_name,         map);

	yaml_map_add_node    ("MODE",      yaml_map_from_head_mode(ppmap_get(head->modes, head_state->zmode)), map);

	return map;
}

int yaml_map_from_lid(const struct Lid* const lid) {
	if (!lid)
		return 0;

	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_bool("CLOSED", lid->closed, map);
	yaml_map_add_str ("DEVICE_PATH", lid->device_path, map);

	return map;
}

int yaml_map_from_scales(const struct SImap* const scales) {
	if (!scales || simap_size(scales) == 0) {
		return yaml_document_add_scalar(&mc.d, NULL, (yaml_char_t*)"", 0, YAML_PLAIN_SCALAR_STYLE);
	}

	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map) {
		return 0;
	}

	for (const struct SImapIt *it = simap_it(scales); it; it = simap_it_next(it)) {
		yaml_map_add_float_nz(it->key, (double)it->val/1000, map);
	}

	return map;
}

int yaml_map_from_state(void) {
	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	if (g_lid)
		yaml_map_add_node("LID", yaml_map_from_lid(g_lid), map);

	if (ppmap_size(g_displ->heads) > 0) {
		const struct Plist *list = ppmap_vals_plist(g_displ->heads);
		yaml_map_add_plist("HEADS", list, (fn_yaml_node_from_type)yaml_map_from_head, map);
		plist_free(list);
	}

	return map;
}

int yaml_map_from_transforms(const struct SImap* const transforms) {
	if (!transforms || simap_size(transforms) == 0) {
		return yaml_document_add_scalar(&mc.d, NULL, (yaml_char_t*)"", 0, YAML_PLAIN_SCALAR_STYLE);
	}

	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map) {
		return 0;
	}

	for (const struct SImapIt *it = simap_it(transforms); it; it = simap_it_next(it)) {
		yaml_map_add_str(it->key, transform_name(it->val), map);
	}

	return map;
}

int yaml_seq_from_messages(const struct IpcOperation* const ipc_operation) {
	if (!ipc_operation)
		return 0;

	int seq = 0;
	int map = 0;

	for (const struct PlistIt *it = plist_it(ipc_operation->log_cap_lines); it; it = plist_it_next(it)) {
		const struct LogCapLine *cap_line = (struct LogCapLine*)it->val;

		if (!cap_line || !cap_line->line || cap_line->threshold < ipc_operation->request->log_threshold)
			continue;

		if (!seq && !(seq = yaml_document_add_sequence(&mc.d, NULL, YAML_BLOCK_SEQUENCE_STYLE)))
			return 0;

		if (!(map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE)))
			return 0;

		yaml_map_add_str(log_threshold_name(cap_line->threshold), cap_line->line, map);

		if (!yaml_document_append_sequence_item(&mc.d, seq, map))
			return 0;
	}

	return seq;
}

int yaml_map_from_condition(const struct CfgCondition* const condition) {
	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	if (sset_size(condition->plugged) > 0)
		yaml_map_add_sset("PLUGGED", condition->plugged, map);

	if (sset_size(condition->unplugged) > 0)
		yaml_map_add_sset("UNPLUGGED", condition->unplugged, map);

	yaml_map_add_enum("LID", condition->lid, condition_lid_name, map);

	return map;
}

int yaml_map_from_head(const struct Head* const head) {
	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_str  ("NAME",           head->name,                                 map);
	yaml_map_add_str  ("DESCRIPTION",    head->description,                          map);
	yaml_map_add_str  ("MAKE",           head->make,                                 map);
	yaml_map_add_str  ("MODEL",          head->model,                                map);
	yaml_map_add_str  ("SERIAL_NUMBER",  head->serial_number,                        map);
	yaml_map_add_int  ("WIDTH_MM",       head->width_mm,                             map);
	yaml_map_add_int  ("HEIGHT_MM",      head->height_mm,                            map);

	yaml_map_add_node ("CURRENT",        yaml_map_from_head_state(&head->cur, head), map);
	yaml_map_add_node ("DESIRED",        yaml_map_from_head_state(&head->des, head), map);
	yaml_map_add_node ("OVERRIDES",      yaml_map_from_head_overrides(head),         map);

	yaml_map_add_node ("MODE_PREFERRED", yaml_map_from_head_mode(ppmap_get(head->modes, head->zmode_pref)), map);

	const struct Plist *modes = ppmap_vals_plist(head->modes);
	yaml_map_add_plist("MODES",          modes,        (fn_yaml_node_from_type)yaml_map_from_head_mode, map);
	plist_free(modes);

	const struct Plist *modes_failed = ppmap_vals_plist(head->modes_failed);
	yaml_map_add_plist("MODES_FAILED",   modes_failed, (fn_yaml_node_from_type)yaml_map_from_head_mode, map);
	plist_free(modes_failed);

	return map;
}

int yaml_map_from_head_mode(const struct Mode* const mode) {
	if (!mode)
		return 0;

	int map = yaml_document_add_mapping(&mc.d, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!map)
		return 0;

	yaml_map_add_int ("WIDTH",       mode->width,       map);
	yaml_map_add_int ("HEIGHT",      mode->height,      map);
	yaml_map_add_int ("REFRESH_MHZ", mode->refresh_mhz, map);

	return map;
}

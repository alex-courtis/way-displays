#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#include <yaml.h>

#include "yaml/unmarshal-types.h"

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "enum.h"
#include "fn.h"
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
#include "wlr-output-management-unstable-v1.h"
#include "yaml/unmarshal-primitives.h"
#include "yaml/unmarshal-types-v1.h"
#include "yaml/unmarshal.h"

void *yaml_root_to_cfg(const yaml_node_t *root) {
	if (!root)
		return NULL;

	// log warnings and skip failures
	uc.t = WARNING;

	if (!yaml_check_node_type(root, YAML_MAPPING_NODE, 0))
		return NULL;

	struct Cfg *cfg = yaml_map_to_cfg(root);

	if (uc.v1_present || *uc.v1_laptop_display_prefix) {
		cfg_migrate_v1(cfg, (*uc.v1_laptop_display_prefix) ? uc.v1_laptop_display_prefix : NULL);
	}

	return cfg;
}

void *yaml_root_to_ipc_request(const yaml_node_t *root) {
	uc.t = ERROR;

	const struct SPmap *m;
	if (!root || !(m = yaml_map_to_spmap(root)))
		return NULL;

	struct IpcRequest *ipc_request = ipc_request_init(0);

	const yaml_node_t *op = spmap_remove(m, "OP");
	if (!op) {
		yaml_log_invalid_value(NULL, "OP");
		goto err;
	}

	yaml_unmarshal_log_ctx_top("OP");
	if (!(ipc_request->command = yaml_scalar_to_enum(op, ipc_command_val, ipc_command_names)))
		goto err;

	// log warnings for remainder
	uc.t = WARNING;

	yaml_unmarshal_log_ctx_top("LOG_THRESHOLD");
	const yaml_node_t *log_threshold = spmap_remove(m, "LOG_THRESHOLD");
	if (log_threshold)
		ipc_request->log_threshold = yaml_scalar_to_enum(log_threshold, log_threshold_val, log_threshold_names);

	yaml_unmarshal_log_ctx_top("CFG");
	const yaml_node_t *cfg = spmap_remove(m, "CFG");
	if (cfg)
		ipc_request->cfg = yaml_map_to_cfg(cfg);

	goto end;

err:
	ipc_request_free(ipc_request);
	ipc_request = NULL;

end:
	spmap_free(m);

	return ipc_request;
}

void *yaml_root_to_ipc_response_plist(const yaml_node_t *root) {
	if (!root)
		return NULL;

	const struct Plist *ipc_responses = ipc_response_plist_init();

	// fail on bad type
	uc.t = ERROR;
	yaml_unmarshal_log_ctx_top("document");
	if (!yaml_check_node_type(root, YAML_SEQUENCE_NODE, YAML_MAPPING_NODE)) {
		goto err;
	}

	if (root->type == YAML_SEQUENCE_NODE) {
		for (const yaml_node_item_t *item = root->data.sequence.items.start; item < root->data.sequence.items.top; item ++) {
			yaml_map_into_ipc_responses(ipc_responses, yaml_document_get_node(&uc.d, *item));
		}
	} else {
		yaml_map_into_ipc_responses(ipc_responses, root);
	}

	if (plist_size(ipc_responses) == 0) {
		goto err;
	}

	return (void*)ipc_responses;

err:
	plist_free_vals(ipc_responses);
	return NULL;
}

struct Cfg *yaml_map_to_cfg(const yaml_node_t *map) {
	if (!map)
		return NULL;

	struct Cfg *cfg = cfg_init();

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *key = yaml_document_get_node(&uc.d, pair->key);
		if (!key || key->type != YAML_SCALAR_NODE)
			continue;

		const char *element_name = (char*)key->data.scalar.value;
		if (!element_name)
			continue;

		const enum CfgElement element_val = cfg_element_val(element_name);
		if (!element_val)
			continue;

		const yaml_node_t *node = yaml_document_get_node(&uc.d, pair->value);
		if (!node)
			continue;

		yaml_unmarshal_log_ctx_top(element_name);

		switch (element_val) {
			case ARRANGE:
				cfg->arrange = yaml_scalar_to_enum_def(ARRANGE_DEFAULT, node, arrange_val_start, arrange_name, arrange_names);
				break;

			case ALIGN:
				cfg->align = yaml_scalar_to_enum_def(ALIGN_DEFAULT, node, align_val_start, align_name, align_names);
				break;

			case ORDER:
				yaml_seq_into_name_desc_sset(cfg->order_name_desc, node);
				break;

			case SCALING:
				cfg->scaling  = yaml_scalar_to_on_off_def(SCALING_DEFAULT, node);
				break;

			case AUTO_SCALE:
				cfg->auto_scale = yaml_scalar_to_on_off_def(AUTO_SCALE_DEFAULT, node);
				break;

			case SCALE:
				if (node->type == YAML_MAPPING_NODE) {
					yaml_map_into_scales(cfg->scales, node);
				} else if (node->type == YAML_SEQUENCE_NODE) { // v1, sequence with NAME_DESC
					yaml_seq_into_col(node, cfg->scales, (fn_yaml_node_into_col)yaml_map_into_scales_v1);
				}
				break;

			case SCALE_ROUND_TO:
				cfg->scale_round_to = yaml_scalar_to_scale_round_to(node);
				break;

			case SCALE_ROUND_STRATEGY:
				cfg->scale_round_strategy = yaml_scalar_to_enum_def(SCALE_ROUND_STRATEGY_DEFAULT, node, scale_round_strategy_val, scale_round_strategy_name, scale_round_strategy_names);
				break;

			case MODE:
				if (node->type == YAML_MAPPING_NODE) {
					yaml_map_into_cfg_modes(cfg->modes, node);
				} else if (node->type == YAML_SEQUENCE_NODE) { // v1, sequence with NAME_DESC
					yaml_seq_into_col(node, cfg->modes, (fn_yaml_node_into_col)yaml_map_into_cfg_modes_v1);
				}
				break;

			case TRANSFORM:
				if (node->type == YAML_MAPPING_NODE) {
					yaml_map_into_transforms(cfg->transforms, node);
				} else if (node->type == YAML_SEQUENCE_NODE) { // v1, sequence with NAME_DESC
					yaml_seq_into_col(node, cfg->transforms, (fn_yaml_node_into_col)yaml_map_into_transforms_v1);
				}
				break;

			case VRR_OFF:
				yaml_seq_into_name_desc_sset(cfg->adaptive_sync_off, node);
				break;

			case CALLBACK_CMD:
				cfg->callback_cmd = yaml_scalar_to_string_def(CALLBACK_CMD_DEFAULT, node);
				break;

			case LAPTOP_DISPLAY_PREFIX:
				yaml_scalar_into_laptop_display_prefix_v1(node);
				break;

			case LAPTOP_LID_MONITOR:
				cfg->laptop_lid_monitor = yaml_scalar_to_on_off_def(LAPTOP_LID_MONITOR_DEFAULT, node);
				break;

			case LOG_THRESHOLD:
				cfg->log_threshold = yaml_scalar_to_enum(node, log_threshold_val, log_threshold_names);
				break;

			case DISABLED:
				if (node->type == YAML_MAPPING_NODE) {
					yaml_map_into_disableds(cfg->disableds, node);
					cfg->disableds_empty = spmap_size(cfg->disableds) == 0;
				} else if (node->type == YAML_SEQUENCE_NODE) { // v1, sequence with NAME_DESC
					yaml_seq_into_col(node, cfg->disableds, (fn_yaml_node_into_col)yaml_node_into_disableds_v1);
				} else {
					cfg->disableds_empty = true;
				}
				break;

			case AUTO_SCALE_DPI:
				yaml_scalar_to_int_def(&cfg->auto_scale_dpi, AUTO_SCALE_DPI_DEFAULT, node);
				break;

			case AUTO_SCALE_MIN:
				yaml_scalar_to_float_def(&cfg->auto_scale_min, AUTO_SCALE_MIN_DEFAULT, node);
				break;

			case AUTO_SCALE_MAX:
				yaml_scalar_to_float_def(&cfg->auto_scale_max, AUTO_SCALE_MAX_DEFAULT, node);
				break;

			default:
				// ignore unexpected
				break;
		}
	}

	return cfg;
}

struct Lid *yaml_map_to_lid(const yaml_node_t *map) {
	const struct SPmap *m;
	if (!(m = yaml_map_to_spmap(map)))
		return NULL;

	struct Lid *lid = (struct Lid*)calloc(1, sizeof(struct Lid));

	lid->device_path = yaml_scalar_to_string(spmap_remove(m, "DEVICE_PATH"));
	yaml_scalar_to_boolean(&lid->closed, spmap_remove(m, "CLOSED"));

	spmap_free(m);

	return lid;
}

struct Mode *yaml_map_to_cfg_mode(const yaml_node_t *map) {
	const struct SPmap *m;
	if (!(m = yaml_map_to_spmap(map)))
		return NULL;

	struct Mode *mode = mode_init();

	yaml_unmarshal_log_ctx_key("WIDTH");
	const yaml_node_t *scalar = spmap_remove(m, "WIDTH");
	if (scalar && !yaml_scalar_to_int(&mode->width, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key("HEIGHT");
	scalar = spmap_remove(m, "HEIGHT");
	if (scalar && !yaml_scalar_to_int(&mode->height, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key("HZ");
	scalar = spmap_remove(m, "HZ");
	if (scalar) {
		float hz = 0;
		if (!yaml_scalar_to_float(&hz, scalar))
			goto err;
		mode->refresh_mhz = lround(hz * 1000);
	}

	yaml_unmarshal_log_ctx_key("MAX");
	scalar = spmap_remove(m, "MAX");
	if (scalar && !yaml_scalar_to_boolean(&mode->max, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key("MAX_PREFERRED_REFRESH");
	scalar = spmap_remove(m, "MAX_PREFERRED_REFRESH");
	if (scalar && !yaml_scalar_to_boolean(&mode->max_preferred_refresh, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key(NULL);
	yaml_log_unknown_keys(m, "WIDTH|HEIGHT|HZ|MAX|MAX_PREFERRED_REFRESH");

	goto end;

err:
	mode_free(mode);
	mode = NULL;

end:
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(NULL);

	return mode;
}

struct Mode *yaml_map_to_head_mode(const yaml_node_t *map) {
	const struct SPmap *m = yaml_map_to_spmap(map);
	if (!m)
		return NULL;

	struct Mode *mode = mode_init();

	yaml_scalar_to_int(&mode->width, spmap_remove(m, "WIDTH"));
	yaml_scalar_to_int(&mode->height, spmap_remove(m, "HEIGHT"));
	yaml_scalar_to_int(&mode->refresh_mhz, spmap_remove(m, "REFRESH_MHZ"));

	spmap_free(m);

	return mode;
}

void yaml_map_into_conditions(const struct Pset* const conditions, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!conditions || !(m = yaml_map_to_spmap(map)))
		return;

	struct CfgCondition *condition = cfg_condition_init();

	yaml_unmarshal_log_ctx_key("PLUGGED");
	const yaml_node_t *node = spmap_remove(m, "PLUGGED");
	if (node) {
		yaml_seq_into_name_desc_sset(condition->plugged, node);
	}

	yaml_unmarshal_log_ctx_key("UNPLUGGED");
	node = spmap_remove(m, "UNPLUGGED");
	if (node) {
		yaml_seq_into_name_desc_sset(condition->unplugged, node);
	}

	yaml_unmarshal_log_ctx_key("LID");
	node = spmap_remove(m, "LID");
	if (node && !(condition->lid = yaml_scalar_to_enum(node, condition_lid_val, condition_lid_names)))
		goto err;

	if (sset_size(condition->plugged) == 0 && sset_size(condition->unplugged) == 0 && !condition->lid)
		goto err;

	if (!pset_add(conditions, condition)) {
		goto err;
	}

	goto end;

err:
	cfg_condition_free(condition);

end:
	yaml_unmarshal_log_ctx_key(NULL);
	spmap_free(m);
}

void yaml_map_into_head_modes(const struct PPmap* const modes, const yaml_node_t *map) {
	if (!modes)
		return;

	const struct Mode *mode = yaml_map_to_head_mode(map);
	ppmap_put(modes, mode, mode);
}

void yaml_map_into_heads(const struct Plist* const heads, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!heads || !(m = yaml_map_to_spmap(map)))
		return;

	struct Head *head = head_init();

	// use mode equals so that we can map preferred/current/desired to the lists
	ppmap_free(head->modes);
	head->modes = mode_ppmap_equal_init();
	ppmap_free(head->modes_failed);
	head->modes_failed = mode_ppmap_equal_init();

	head->name           = yaml_scalar_to_string(spmap_remove(m, "NAME"));
	head->description    = yaml_scalar_to_string(spmap_remove(m, "DESCRIPTION"));
	head->make           = yaml_scalar_to_string(spmap_remove(m, "MAKE"));
	head->model          = yaml_scalar_to_string(spmap_remove(m, "MODEL"));
	head->serial_number  = yaml_scalar_to_string(spmap_remove(m, "SERIAL_NUMBER"));

	yaml_scalar_to_int(&head->width_mm,  spmap_remove(m, "WIDTH_MM"));
	yaml_scalar_to_int(&head->height_mm, spmap_remove(m, "HEIGHT_MM"));

	yaml_seq_into_col(spmap_remove(m, "MODES"),        head->modes,        (fn_yaml_node_into_col)yaml_map_into_head_modes);
	yaml_seq_into_col(spmap_remove(m, "MODES_FAILED"), head->modes_failed, (fn_yaml_node_into_col)yaml_map_into_head_modes);

	// find MODE_PREFERRED in MODES/MODES_FAILED and assign the key
	struct Mode *mode_pref = yaml_map_to_head_mode(spmap_remove(m, "MODE_PREFERRED"));
	if (mode_pref) {
		struct PPmapFilter f = { .val_data = (fn_pred_pp)mode_equal, .data = mode_pref, };
		head->zmode_pref = ppmap_find(head->modes, f).key;
		if (!head->zmode_pref) {
			head->zmode_pref = ppmap_find(head->modes_failed, f).key;
		}
		mode_free(mode_pref);
	}

	yaml_map_into_head_state(&head->cur, head, spmap_remove(m, "CURRENT"));
	yaml_map_into_head_state(&head->des, head, spmap_remove(m, "DESIRED"));

	const struct SPmap *mo = yaml_map_to_spmap(spmap_remove(m, "OVERRIDES"));
	if (mo) {
		bool disabled;
		if (yaml_scalar_to_boolean(&disabled, spmap_remove(mo, "DISABLED"))) {
			head->overrided_enabled = disabled ? OverrideFalse : OverrideTrue;
		}
	}

	plist_append(heads, head);

	spmap_free(m);
	spmap_free(mo);
}

void yaml_map_into_ipc_responses(const struct Plist* const ipc_responses, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!ipc_responses || !(m = yaml_map_to_spmap(map)))
		return;

	// log exceptions and fail for required fields
	uc.t = ERROR;

	struct IpcResponse *ipc_response = ipc_response_init();

	const yaml_node_t *done = spmap_remove(m, "DONE");
	if (!done) {
		yaml_log_invalid_value(NULL, "DONE");
		goto err;
	}

	const yaml_node_t *rc = spmap_remove(m, "RC");
	if (!rc) {
		yaml_log_invalid_value(NULL, "RC");
		goto err;
	}

	yaml_unmarshal_log_ctx_top("DONE");
	if (!yaml_scalar_to_boolean(&ipc_response->status.done, done))
		goto err;

	yaml_unmarshal_log_ctx_top("RC");
	if (!yaml_scalar_to_int(&ipc_response->status.rc, rc))
		goto err;

	// suppress validation failures for remainder
	uc.t = 0;

	yaml_unmarshal_log_ctx_top("CFG");
	const yaml_node_t *cfg = spmap_remove(m, "CFG");
	if (cfg)
		ipc_response->cfg = yaml_map_to_cfg(cfg);

	yaml_unmarshal_log_ctx_top("STATE");
	const yaml_node_t *state = spmap_remove(m, "STATE");
	if (state) {
		const struct SPmap *ms = yaml_map_to_spmap(state);
		if (ms) {

			ipc_response->lid =	yaml_map_to_lid(spmap_remove(ms, "LID"));

			yaml_seq_into_col(spmap_remove(ms, "HEADS"), ipc_response->heads, (fn_yaml_node_into_col)yaml_map_into_heads);

			spmap_free(ms);
		}
	}

	yaml_unmarshal_log_ctx_top("MESSAGES");
	const yaml_node_t *messages = spmap_remove(m, "MESSAGES");
	if (messages) {
		yaml_seq_into_col(messages, ipc_response->log_cap_lines, (fn_yaml_node_into_col)yaml_map_into_log_cap_lines);
	}

	plist_append(ipc_responses, ipc_response);

	goto end;

err:
	ipc_response_free(ipc_response);
	ipc_response = NULL;

end:
	spmap_free(m);
}

void yaml_map_into_log_cap_lines(const struct Plist* const log_cap_lines, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!log_cap_lines || !(m = yaml_map_to_spmap(map)))
		return;

	// unmarshal many pairs even though schema specifies exactly one
	for (const struct SPmapIt *it = spmap_it(m); it; it = spmap_it_next(it)) {

		enum LogThreshold threshold = log_threshold_val(it->key);
		char *line = yaml_scalar_to_string(it->val);

		if (threshold && line)
			plist_append(log_cap_lines, log_cap_line_init(threshold, line));

		free(line);
	}

	spmap_free(m);
}

void yaml_map_into_cfg_modes(const struct SPmap* const modes, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!modes || !(m = yaml_map_to_spmap(map)))
		return;

	for (const struct SPmapIt *it = spmap_it(m); it; it = spmap_it_next(it)) {
		yaml_unmarshal_log_ctx_name_desc(NULL);

		if (!yaml_valid_name_desc(it->key))
			continue;

		yaml_unmarshal_log_ctx_name_desc(it->key);

		spmap_put_if_absent(modes, it->key, yaml_map_to_cfg_mode(it->val));
	}

	yaml_unmarshal_log_ctx_name_desc(NULL);
	spmap_free(m);
}

struct CfgDisabled *yaml_map_to_disabled_cond(const yaml_node_t *map) {
	struct CfgDisabled *disabled = cfg_disabled_init();

	const struct SPmap *m = yaml_map_to_spmap(map);
	const yaml_node_t *node_if = spmap_remove(m, "IF");

	if (!m || !node_if) {
		yaml_log_invalid_value(NULL, "IF");
		goto err;
	}

	yaml_log_unknown_keys(m, "IF");

	yaml_unmarshal_log_ctx_key("IF");
	if (!yaml_seq_into_col(node_if, disabled->conditions, (fn_yaml_node_into_col)yaml_map_into_conditions)) {
		goto err;
	}

	if (pset_size(disabled->conditions) == 0) {
		goto err;
	}

	goto end;

err:
	cfg_disabled_free((void*)disabled);
	disabled = NULL;

end:
	spmap_free(m);

	return disabled;
}

void yaml_map_into_disableds(const struct SPmap* const disableds, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!disableds || !(m = yaml_map_to_spmap(map)))
		return;

	for (const struct SPmapIt *it = spmap_it(m); it; it = spmap_it_next(it)) {
		yaml_unmarshal_log_ctx_name_desc(NULL);
		yaml_unmarshal_log_ctx_key(NULL);

		if (!yaml_valid_name_desc(it->key))
			continue;

		yaml_unmarshal_log_ctx_name_desc(it->key);

		// map for conditons, otherwise unconditional (empty scalar)
		const yaml_node_t *node = it->val;
		if (!yaml_check_node_type(node, YAML_MAPPING_NODE, YAML_SCALAR_NODE))
			continue;

		if (node->type == YAML_SCALAR_NODE) {
			spmap_put(disableds, it->key, cfg_disabled_init());
		} else {
			spmap_put(disableds, it->key, yaml_map_to_disabled_cond(it->val));
		}

	}

	yaml_unmarshal_log_ctx_name_desc(NULL);
	yaml_unmarshal_log_ctx_key(NULL);
	spmap_free(m);
}

void yaml_map_into_scales(const struct SImap* const scales, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!scales || !(m = yaml_map_to_spmap(map)))
		return;

	float scale;
	for (const struct SPmapIt *it = spmap_it(m); it; it = spmap_it_next(it)) {
		yaml_unmarshal_log_ctx_name_desc(NULL);

		if (!yaml_valid_name_desc(it->key))
			continue;

		yaml_unmarshal_log_ctx_name_desc(it->key);

		if (!yaml_scalar_to_float(&scale, it->val))
			continue;

		if (scale <= 0) {
			yaml_log_invalid_value(((const yaml_node_t*)it->val)->data.scalar.value, "positive number");
			continue;
		}

		simap_put_if_absent(scales, it->key, round(scale * 1000));
	}

	yaml_unmarshal_log_ctx_name_desc(NULL);
	spmap_free(m);
}

void yaml_map_into_transforms(const struct SImap* const transforms, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!transforms || !(m = yaml_map_to_spmap(map)))
		return;

	enum wl_output_transform transform;
	for (const struct SPmapIt *it = spmap_it(m); it; it = spmap_it_next(it)) {
		yaml_unmarshal_log_ctx_name_desc(NULL);

		if (!yaml_valid_name_desc(it->key))
			continue;

		yaml_unmarshal_log_ctx_name_desc(it->key);

		if (!(transform = yaml_scalar_to_enum(it->val, transform_val, transform_names)))
			continue;

		simap_put_if_absent(transforms, it->key, transform);
	}

	spmap_free(m);
	yaml_unmarshal_log_ctx_name_desc(NULL);
}

void yaml_map_into_head_state(struct HeadState *head_state, const struct Head * const head, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!head_state || !(m = yaml_map_to_spmap(map)))
		return;

	yaml_scalar_to_boolean(&head_state->enabled, spmap_remove(m, "ENABLED"));

	yaml_scalar_to_int(&head_state->x, spmap_remove(m, "X"));
	yaml_scalar_to_int(&head_state->y, spmap_remove(m, "Y"));

	head_state->transform = yaml_scalar_to_enum(spmap_remove(m, "TRANSFORM"), transform_val, NULL);

	float scale;
	if (yaml_scalar_to_float(&scale, spmap_remove(m, "SCALE")))
		head_state->scale = wl_fixed_from_double(scale);

	bool vrr = false;
	if (yaml_scalar_to_boolean(&vrr, spmap_remove(m, "VRR")))
		head_state->adaptive_sync = vrr ? ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED : ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	// find MODE in MODES/MODES_FAILED and assign the key
	struct Mode *mode = yaml_map_to_head_mode(spmap_remove(m, "MODE"));
	if (mode) {
		head_state->zmode = ppmap_first_key(head->modes, mode);
		if (!head_state->zmode) {
			head_state->zmode = ppmap_first_key(head->modes_failed, mode);
		}
		mode_free(mode);
	}

	spmap_free(m);

	return;
}

char *yaml_scalar_to_name_desc(const yaml_node_t *scalar) {
	char *name_desc = yaml_scalar_to_string(scalar);
	if (!name_desc)
		return NULL;

	if (yaml_valid_name_desc(name_desc))
		return name_desc;

	free(name_desc);
	return NULL;
}

unsigned int yaml_scalar_to_scale_round_to(const yaml_node_t *scalar) {
	float val;
	unsigned int ret;

	yaml_unmarshal_log_def(scale_round_to_name(SCALE_ROUND_TO_DEFAULT));
	yaml_unmarshal_log_enum_names(scale_round_to_names);

	if (!yaml_scalar_to_float(&val, scalar)) {
		ret = SCALE_ROUND_TO_DEFAULT;
		goto end;
	}

	ret = scale_round_to_val(val);
	if (!ret) {
		yaml_log_invalid_value(scalar->data.scalar.value, "number");
		ret = SCALE_ROUND_TO_DEFAULT;
		goto end;
	}

end:
	yaml_unmarshal_log_def(NULL);
	yaml_unmarshal_log_enum_names(NULL);

	return ret;
}

void yaml_seq_into_name_desc_sset(const struct Sset* const sset, const yaml_node_t *seq) {
	// SCALAR indicates empty array, acceptable
	if (!sset || seq->type == YAML_SCALAR_NODE || !yaml_check_node_type(seq, YAML_SEQUENCE_NODE, 0))
		return;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *scalar = yaml_document_get_node(&uc.d, *item);
		if (!scalar)
			continue;

		char *val = NULL;
		if ((val = yaml_scalar_to_name_desc(scalar))) {
			sset_add(sset, val);
			free(val);
		}
	}
}


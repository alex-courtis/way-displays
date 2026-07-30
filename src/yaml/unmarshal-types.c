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
#include "yaml/unmarshal.h"

void *yaml_root_to_cfg(struct UC *c, const yaml_node_t *root) {
	if (!root)
		return NULL;

	// log warnings and skip failures
	c->t = WARNING;

	if (!yaml_check_node_type(c, root, YAML_MAPPING_NODE, 0))
		return NULL;

	return yaml_map_to_cfg(c, root);
}

void *yaml_root_to_ipc_request(struct UC *c, const yaml_node_t *root) {
	c->t = ERROR;
	yaml_unmarshal_log_ctx_top(c, "document");

	const struct SPmap *m;
	if (!root || !(m = yaml_map_to_spmap(c, root)))
		return NULL;

	struct IpcRequest *ipc_request = ipc_request_init(0);

	yaml_unmarshal_log_ctx_top(c, "OP");
	const yaml_node_t *op = spmap_get(m, "OP");
	if (!yaml_check_mandatory(c, op) || !(ipc_request->command = yaml_scalar_to_enum(c, op, ipc_command_val, ipc_command_names)))
		goto err;

	// log warnings for remainder
	c->t = WARNING;

	yaml_unmarshal_log_ctx_top(c, "LOG_THRESHOLD");
	const yaml_node_t *log_threshold = spmap_get(m, "LOG_THRESHOLD");
	if (log_threshold)
		ipc_request->log_threshold = yaml_scalar_to_enum(c, log_threshold, log_threshold_val, log_threshold_names);

	yaml_unmarshal_log_ctx_top(c, "CFG");
	const yaml_node_t *cfg = spmap_get(m, "CFG");
	if (cfg)
		ipc_request->cfg = yaml_map_to_cfg(c, cfg);

	goto end;

err:
	ipc_request_free(ipc_request);
	ipc_request = NULL;

end:
	spmap_free(m);

	return ipc_request;
}

void *yaml_root_to_ipc_response_plist(struct UC *c, const yaml_node_t *root) {
	if (!root)
		return NULL;

	const struct Plist *ipc_responses = ipc_response_plist_init();

	// fail on bad type
	c->t = ERROR;
	yaml_unmarshal_log_ctx_top(c, "document");
	if (!yaml_check_node_type(c, root, YAML_SEQUENCE_NODE, YAML_MAPPING_NODE)) {
		goto err;
	}

	if (root->type == YAML_SEQUENCE_NODE) {
		for (const yaml_node_item_t *item = root->data.sequence.items.start; item < root->data.sequence.items.top; item ++) {
			yaml_map_into_ipc_responses(c, ipc_responses, yaml_document_get_node(&c->d, *item));
		}
	} else {
		yaml_map_into_ipc_responses(c, ipc_responses, root);
	}

	if (plist_size(ipc_responses) == 0) {
		goto err;
	}

	return (void*)ipc_responses;

err:
	plist_free_vals(ipc_responses);
	return NULL;
}

struct Cfg *yaml_map_to_cfg(struct UC *c, const yaml_node_t *map) {
	if (!map)
		return NULL;

	struct Cfg *cfg = cfg_init();

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *key = yaml_document_get_node(&c->d, pair->key);
		if (!key || key->type != YAML_SCALAR_NODE)
			continue;

		const char *element_name = (char*)key->data.scalar.value;
		if (!element_name)
			continue;

		const enum CfgElement element_val = cfg_element_val(element_name);
		if (!element_val)
			continue;

		const yaml_node_t *node = yaml_document_get_node(&c->d, pair->value);
		if (!node)
			continue;

		yaml_unmarshal_log_ctx_top(c, element_name);

		switch (element_val) {
			case ARRANGE:
				cfg->arrange = yaml_scalar_to_enum_def(c, ARRANGE_DEFAULT, node, arrange_val_start, arrange_name, arrange_names);
				break;

			case ALIGN:
				cfg->align = yaml_scalar_to_enum_def(c, ALIGN_DEFAULT, node, align_val_start, align_name, align_names);
				break;

			case ORDER:
				yaml_seq_into_name_desc_sset(c, cfg->order_name_desc, node);
				break;

			case SCALING:
				cfg->scaling  = yaml_scalar_to_enum_def(c, SCALING_DEFAULT, node, on_off_val, on_off_name, on_off_names);
				break;

			case AUTO_SCALE:
				cfg->auto_scale = yaml_scalar_to_enum_def(c, AUTO_SCALE_DEFAULT, node, on_off_val, on_off_name, on_off_names);
				break;

			case SCALE:
				switch(node->type) {
					case YAML_SEQUENCE_NODE: // v1, sequence with NAME_DESC
						yaml_seq_into_col(c, node, cfg->scales, (fn_yaml_node_into_col)yaml_map_into_scales_v1);
						break;
					case YAML_MAPPING_NODE:
						yaml_map_into_scales(c, cfg->scales, node);
						break;
					default:
						yaml_check_node_type(c, node, YAML_SEQUENCE_NODE, YAML_MAPPING_NODE);
						break;
				}
				break;

			case SCALE_ROUND_TO:
				cfg->scale_round_to = yaml_scalar_to_scale_round_to(c, node);
				break;

			case SCALE_ROUND_STRATEGY:
				cfg->scale_round_strategy = yaml_scalar_to_enum_def(c, SCALE_ROUND_STRATEGY_DEFAULT, node, scale_round_strategy_val, scale_round_strategy_name, scale_round_strategy_names);
				break;

			case MODE:
				switch(node->type) {
					case YAML_SEQUENCE_NODE: // v1, sequence with NAME_DESC
						yaml_seq_into_col(c, node, cfg->modes, (fn_yaml_node_into_col)yaml_map_into_cfg_modes_v1);
						break;
					case YAML_MAPPING_NODE:
						yaml_map_into_cfg_modes(c, cfg->modes, node);
						break;
					default:
						yaml_check_node_type(c, node, YAML_SEQUENCE_NODE, YAML_MAPPING_NODE);
						break;
				}
				break;

			case TRANSFORM:
				switch(node->type) {
					case YAML_SEQUENCE_NODE: // v1, sequence with NAME_DESC
						yaml_seq_into_col(c, node, cfg->transforms, (fn_yaml_node_into_col)yaml_map_into_transforms_v1);
						break;
					case YAML_MAPPING_NODE:
						yaml_map_into_transforms(c, cfg->transforms, node);
						break;
					default:
						yaml_check_node_type(c, node, YAML_SEQUENCE_NODE, YAML_MAPPING_NODE);
						break;
				}
				break;

			case VRR_OFF:
				yaml_seq_into_name_desc_sset(c, cfg->adaptive_sync_off, node);
				break;

			case CHANGE_SUCCESS_CMD:
			case CALLBACK_CMD:
				free(cfg->callback_cmd); // may be both entries present, use the last
				cfg->callback_cmd = yaml_scalar_to_string_def(c, CALLBACK_CMD_DEFAULT, node);
				break;

			case LAPTOP_DISPLAY_PREFIX:
				cfg->laptop_display_prefix = yaml_scalar_to_string(c, node);
				break;

			case LAPTOP_LID_MONITOR:
				cfg->laptop_lid_monitor = yaml_scalar_to_enum_def(c, LAPTOP_LID_MONITOR_DEFAULT, node, on_off_val, on_off_name, on_off_names);
				break;

			case MAX_PREFERRED_REFRESH:
				yaml_seq_into_name_desc_sset(c, cfg->max_preferred_refresh, node);
				break;

			case LOG_THRESHOLD:
				cfg->log_threshold = yaml_scalar_to_enum(c, node, log_threshold_val, log_threshold_names);
				break;

			case DISABLED:
				yaml_seq_into_col(c, node, cfg->disableds, (fn_yaml_node_into_col)yaml_node_into_disableds);
				break;

			case AUTO_SCALE_DPI:
				yaml_scalar_to_int_def(c, &cfg->auto_scale_dpi, AUTO_SCALE_DPI_DEFAULT, node);
				break;

			case AUTO_SCALE_MIN:
				yaml_scalar_to_float_def(c, &cfg->auto_scale_min, AUTO_SCALE_MIN_DEFAULT, node);
				break;

			case AUTO_SCALE_MAX:
				yaml_scalar_to_float_def(c, &cfg->auto_scale_max, AUTO_SCALE_MAX_DEFAULT, node);
				break;

			default:
				// ignore unexpected
				break;
		}
	}

	return cfg;
}

void yaml_map_into_ipc_responses(struct UC *c, const struct Plist* const ipc_responses, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!ipc_responses || !(m = yaml_map_to_spmap(c, map)))
		return;

	// log exceptions and fail for required fields
	c->t = ERROR;

	struct IpcResponse *ipc_response = ipc_response_init();

	yaml_unmarshal_log_ctx_top(c, "DONE");
	const yaml_node_t *done = spmap_get(m, "DONE");
	if (!yaml_check_mandatory(c, done) || !yaml_scalar_to_boolean(c, &ipc_response->status.done, done))
		goto err;

	yaml_unmarshal_log_ctx_top(c, "RC");
	const yaml_node_t *rc = spmap_get(m, "RC");
	if (!yaml_check_mandatory(c, rc) || !yaml_scalar_to_int(c, &ipc_response->status.rc, rc))
		goto err;

	// suppress validation failures for remainder
	c->t = 0;

	yaml_unmarshal_log_ctx_top(c, "CFG");
	const yaml_node_t *cfg = spmap_get(m, "CFG");
	if (cfg)
		ipc_response->cfg = yaml_map_to_cfg(c, cfg);

	yaml_unmarshal_log_ctx_top(c, "STATE");
	const yaml_node_t *state = spmap_get(m, "STATE");
	if (state) {
		const struct SPmap *ms = yaml_map_to_spmap(c, state);
		if (ms) {

			ipc_response->lid =	yaml_map_to_lid(c, spmap_get(ms, "LID"));

			yaml_seq_into_col(c, spmap_get(ms, "HEADS"), ipc_response->heads, (fn_yaml_node_into_col)yaml_map_into_heads);

			spmap_free(ms);
		}
	}

	yaml_unmarshal_log_ctx_top(c, "MESSAGES");
	const yaml_node_t *messages = spmap_get(m, "MESSAGES");
	if (messages) {
		yaml_seq_into_col(c, messages, ipc_response->log_cap_lines, (fn_yaml_node_into_col)yaml_map_into_log_cap_lines);
	}

	plist_append(ipc_responses, ipc_response);

	goto end;

err:
	ipc_response_free(ipc_response);
	ipc_response = NULL;

end:
	spmap_free(m);
}

void yaml_map_into_conditions(struct UC *c, const struct Pset* const conditions, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!conditions || !(m = yaml_map_to_spmap(c, map)))
		return;

	struct CfgCondition *condition = cfg_condition_init();

	yaml_unmarshal_log_ctx_key(c, "PLUGGED");
	const yaml_node_t *node = spmap_get(m, "PLUGGED");
	if (node) {
		yaml_seq_into_name_desc_sset(c, condition->plugged, node);
		if (sset_size(condition->plugged) == 0) {
			goto err;
		}
	}

	yaml_unmarshal_log_ctx_key(c, "UNPLUGGED");
	node = spmap_get(m, "UNPLUGGED");
	if (node) {
		yaml_seq_into_name_desc_sset(c, condition->unplugged, node);
		if (sset_size(condition->unplugged) == 0) {
			goto err;
		}
	}

	yaml_unmarshal_log_ctx_key(c, "LID");
	node = spmap_get(m, "LID");
	if (node && !(condition->lid = yaml_scalar_to_enum(c, node, condition_lid_val, condition_lid_names)))
		goto err;

	if (sset_size(condition->plugged) == 0 && sset_size(condition->unplugged) == 0 && !condition->lid)
		goto err;

	if (!pset_add(conditions, condition)) {
		cfg_condition_free(condition);
	}

	goto end;

err:
	cfg_condition_free(condition);

end:
	yaml_unmarshal_log_ctx_key(c, NULL);
	spmap_free(m);
}

void yaml_map_into_scales_v1(struct UC *c, const struct SImap* const scales, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!scales || !(m = yaml_map_to_spmap(c, map)))
		return;

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	const yaml_node_t *scalar = spmap_get(m, "NAME_DESC");
	if (!yaml_check_mandatory(c, scalar) || !(name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto end;

	yaml_unmarshal_log_ctx_name_desc(c, name_desc);

	yaml_unmarshal_log_ctx_key(c, "SCALE");
	scalar = spmap_get(m, "SCALE");
	float scale;
	if (!yaml_check_mandatory(c, scalar) || !yaml_scalar_to_float(c, &scale, scalar))
		goto end;

	if (scale <= 0) {
		yaml_unmarshal_log_invalid_value(c, scalar->data.scalar.value, "positive number");
		goto end;
	}

	if (simap_put_if_absent(scales, name_desc, round(scale * 1000))) {
		log_warn("Removing duplicate SCALE %s", name_desc);
	}

end:
	free(name_desc);
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);

	return;
}

void yaml_map_into_scales(struct UC *c, const struct SImap* const scales, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!scales || !(m = yaml_map_to_spmap(c, map)))
		return;

	float scale;
	for (const struct SPmapIt *it = spmap_it(m); it; it = spmap_it_next(it)) {
		yaml_unmarshal_log_ctx_name_desc(c, NULL);

		if (!yaml_valid_regex(c, it->key))
			continue;

		yaml_unmarshal_log_ctx_name_desc(c, it->key);

		if (!yaml_scalar_to_float(c, &scale, it->val))
			continue;

		if (scale <= 0) {
			yaml_unmarshal_log_invalid_value(c, ((const yaml_node_t*)it->val)->data.scalar.value, "positive number");
			continue;
		}

		simap_put_if_absent(scales, it->key, round(scale * 1000));
	}

	yaml_unmarshal_log_ctx_name_desc(c, NULL);
	spmap_free(m);
}

struct Mode *yaml_map_to_cfg_mode(struct UC *c, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!(m = yaml_map_to_spmap(c, map)))
		return NULL;

	struct Mode *mode = mode_init();

	yaml_unmarshal_log_ctx_key(c, "WIDTH");
	const yaml_node_t *scalar = spmap_get(m, "WIDTH");
	if (scalar && !yaml_scalar_to_int(c, &mode->width, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key(c, "HEIGHT");
	scalar = spmap_get(m, "HEIGHT");
	if (scalar && !yaml_scalar_to_int(c, &mode->height, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key(c, "HZ");
	scalar = spmap_get(m, "HZ");
	if (scalar) {
		float hz = 0;
		if (!yaml_scalar_to_float(c, &hz, scalar))
			goto err;
		mode->refresh_mhz = lround(hz * 1000);
	}

	yaml_unmarshal_log_ctx_key(c, "MAX");
	scalar = spmap_get(m, "MAX");
	if (scalar && !yaml_scalar_to_boolean(c, &mode->max, scalar))
		goto err;

	goto end;

err:
	mode_free(mode);
	mode = NULL;

end:
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(c, NULL);

	return mode;
}

void yaml_map_into_cfg_modes_v1(struct UC *c, const struct SPmap* const modes, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!modes || !(m = yaml_map_to_spmap(c, map)))
		return;

	struct Mode *mode = NULL;

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	const yaml_node_t *scalar = spmap_get(m, "NAME_DESC");
	if (!yaml_check_mandatory(c, scalar) || !(name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto err;

	yaml_unmarshal_log_ctx_name_desc(c, name_desc);

	mode = yaml_map_to_cfg_mode(c, map);
	if (!mode)
		goto err;

	if (spmap_put_if_absent(modes, name_desc, mode)) {
		log_warn("Removing duplicate MODE %s", name_desc);
		goto err;
	}

	goto end;

err:
	mode_free(mode);

end:
	free(name_desc);
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);
}

void yaml_map_into_cfg_modes(struct UC *c, const struct SPmap* const modes, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!modes || !(m = yaml_map_to_spmap(c, map)))
		return;

	for (const struct SPmapIt *it = spmap_it(m); it; it = spmap_it_next(it)) {
		yaml_unmarshal_log_ctx_name_desc(c, NULL);

		if (!yaml_valid_regex(c, it->key))
			continue;

		yaml_unmarshal_log_ctx_name_desc(c, it->key);

		spmap_put_if_absent(modes, it->key, yaml_map_to_cfg_mode(c, it->val));
	}

	yaml_unmarshal_log_ctx_name_desc(c, NULL);
	spmap_free(m);
}

void yaml_map_into_transforms_v1(struct UC *c, const struct SImap* const transforms, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!transforms || !(m = yaml_map_to_spmap(c, map)))
		return;

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	const yaml_node_t *scalar = spmap_get(m, "NAME_DESC");
	if (!yaml_check_mandatory(c, scalar) || !(name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto end;

	yaml_unmarshal_log_ctx_name_desc(c, name_desc);

	yaml_unmarshal_log_ctx_key(c, "TRANSFORM");
	scalar = spmap_get(m, "TRANSFORM");
	enum wl_output_transform transform;
	if (!yaml_check_mandatory(c, scalar) || !(transform = yaml_scalar_to_enum(c, scalar, transform_val, transform_names)))
		goto end;

	if (simap_put_if_absent(transforms, name_desc, transform)) {
		log_warn("Removing duplicate TRANSFORM %s", name_desc);
	}

end:
	free(name_desc);
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);
}

void yaml_map_into_transforms(struct UC *c, const struct SImap* const transforms, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!transforms || !(m = yaml_map_to_spmap(c, map)))
		return;

	enum wl_output_transform transform;
	for (const struct SPmapIt *it = spmap_it(m); it; it = spmap_it_next(it)) {
		yaml_unmarshal_log_ctx_name_desc(c, NULL);

		// TODO yaml v2: maybe yaml_scalar_to_name_desc or similar, if this gets repetitive
		if (!yaml_valid_regex(c, it->key))
			continue;

		yaml_unmarshal_log_ctx_name_desc(c, it->key);

		if (!(transform = yaml_scalar_to_enum(c, it->val, transform_val, transform_names)))
			continue;

		simap_put_if_absent(transforms, it->key, transform);
	}

	spmap_free(m);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);
}

struct Lid *yaml_map_to_lid(struct UC *c, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!(m = yaml_map_to_spmap(c, map)))
		return NULL;

	struct Lid *lid = (struct Lid*)calloc(1, sizeof(struct Lid));

	lid->device_path = yaml_scalar_to_string(c, spmap_get(m, "DEVICE_PATH"));
	yaml_scalar_to_boolean(c, &lid->closed, spmap_get(m, "CLOSED"));

	spmap_free(m);

	return lid;
}

struct Mode *yaml_map_to_head_mode(struct UC *c, const yaml_node_t *map) {
	const struct SPmap *m = yaml_map_to_spmap(c, map);
	if (!m)
		return NULL;

	struct Mode *mode = mode_init();

	yaml_scalar_to_int(c, &mode->width, spmap_get(m, "WIDTH"));
	yaml_scalar_to_int(c, &mode->height, spmap_get(m, "HEIGHT"));
	yaml_scalar_to_int(c, &mode->refresh_mhz, spmap_get(m, "REFRESH_MHZ"));

	spmap_free(m);

	return mode;
}

void yaml_map_into_head_modes(struct UC *c, const struct PPmap* const modes, const yaml_node_t *map) {
	if (!modes)
		return;

	const struct Mode *mode = yaml_map_to_head_mode(c, map);
	ppmap_put(modes, mode, mode);
}

void yaml_map_into_heads(struct UC *c, const struct Plist* const heads, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!heads || !(m = yaml_map_to_spmap(c, map)))
		return;

	struct Head *head = head_init();

	// use mode equals so that we can map preferred/current/desired to the lists
	ppmap_free(head->modes);
	head->modes = mode_ppmap_equal_init();
	ppmap_free(head->modes_failed);
	head->modes_failed = mode_ppmap_equal_init();

	head->name           = yaml_scalar_to_string(c, spmap_get(m, "NAME"));
	head->description    = yaml_scalar_to_string(c, spmap_get(m, "DESCRIPTION"));
	head->make           = yaml_scalar_to_string(c, spmap_get(m, "MAKE"));
	head->model          = yaml_scalar_to_string(c, spmap_get(m, "MODEL"));
	head->serial_number  = yaml_scalar_to_string(c, spmap_get(m, "SERIAL_NUMBER"));
	yaml_scalar_to_int      (c, &head->width_mm,    spmap_get(m, "WIDTH_MM"));
	yaml_scalar_to_int      (c, &head->height_mm,   spmap_get(m, "HEIGHT_MM"));

	yaml_seq_into_col(c, spmap_get(m, "MODES"),        head->modes,        (fn_yaml_node_into_col)yaml_map_into_head_modes);
	yaml_seq_into_col(c, spmap_get(m, "MODES_FAILED"), head->modes_failed, (fn_yaml_node_into_col)yaml_map_into_head_modes);

	// find MODE_PREFERRED in MODES/MODES_FAILED and assign the key
	struct Mode *mode_pref = yaml_map_to_head_mode(c, spmap_get(m, "MODE_PREFERRED"));
	if (mode_pref) {
		struct PPmapFilter f = { .val_data = (fn_pred_pp)mode_equal, .data = mode_pref, };
		head->zmode_pref = ppmap_find(head->modes, f).key;
		if (!head->zmode_pref) {
			head->zmode_pref = ppmap_find(head->modes_failed, f).key;
		}
		mode_free(mode_pref);
	}

	yaml_map_into_head_state(c, &head->cur, head, spmap_get(m, "CURRENT"));
	yaml_map_into_head_state(c, &head->des, head, spmap_get(m, "DESIRED"));

	const struct SPmap *mo = yaml_map_to_spmap(c, spmap_get(m, "OVERRIDES"));
	if (mo) {
		bool disabled;
		if (yaml_scalar_to_boolean(c, &disabled, spmap_get(mo, "DISABLED"))) {
			head->overrided_enabled = disabled ? OverrideFalse : OverrideTrue;
		}
	}

	plist_append(heads, head);

	spmap_free(m);
	spmap_free(mo);
}

void yaml_node_into_disableds(struct UC *c, const struct Pset* const disableds, const yaml_node_t *node) {
	// TODO expect string, not scalar
	if (!disableds || !yaml_check_node_type(c, node, YAML_SCALAR_NODE, YAML_MAPPING_NODE))
		return;

	struct CfgDisabled *disabled = NULL;

	const struct SPmap *m = NULL;

	if (node->type == YAML_SCALAR_NODE) {

		disabled = cfg_disabled_init();
		if (!(disabled->name_desc = yaml_scalar_to_name_desc(c, node)))
			goto err;

		if (!pset_add(disableds, disabled)) {
			cfg_disabled_free(disabled);
		}

	} else if (node->type == YAML_MAPPING_NODE && (m = yaml_map_to_spmap(c, node))) {

		disabled = cfg_disabled_init();

		yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
		const yaml_node_t *scalar = spmap_get(m, "NAME_DESC");
		if (!yaml_check_mandatory(c, scalar) || !(disabled->name_desc = yaml_scalar_to_name_desc(c, scalar)))
			goto err;

		yaml_unmarshal_log_ctx_name_desc(c, disabled->name_desc);

		yaml_unmarshal_log_ctx_key(c, "IF");
		const yaml_node_t *map = spmap_get(m, "IF");
		if (map)
			yaml_seq_into_col(c, map, disabled->conditions, (fn_yaml_node_into_col)yaml_map_into_conditions);

		if (!pset_add(disableds, disabled)) {
			cfg_disabled_free(disabled);
		}
	}

	goto end;

err:
	cfg_disabled_free(disabled);

end:
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);
}

void yaml_map_into_head_state(struct UC *c, struct HeadState *head_state, const struct Head * const head, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!head_state || !(m = yaml_map_to_spmap(c, map)))
		return;

	yaml_scalar_to_boolean(c, &head_state->enabled, spmap_get(m, "ENABLED"));

	yaml_scalar_to_int(c, &head_state->x, spmap_get(m, "X"));
	yaml_scalar_to_int(c, &head_state->y, spmap_get(m, "Y"));

	head_state->transform = yaml_scalar_to_enum(c, spmap_get(m, "TRANSFORM"), transform_val, NULL);

	float scale;
	if (yaml_scalar_to_float(c, &scale, spmap_get(m, "SCALE")))
		head_state->scale = wl_fixed_from_double(scale);

	bool vrr = false;
	if (yaml_scalar_to_boolean(c, &vrr, spmap_get(m, "VRR")))
		head_state->adaptive_sync = vrr ? ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED : ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	// find MODE in MODES/MODES_FAILED and assign the key
	struct Mode *mode = yaml_map_to_head_mode(c, spmap_get(m, "MODE"));
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

void yaml_map_into_log_cap_lines(struct UC *c, const struct Plist* const log_cap_lines, const yaml_node_t *map) {
	const struct SPmap *m;
	if (!log_cap_lines || !(m = yaml_map_to_spmap(c, map)))
		return;

	// unmarshal many pairs even though schema specifies exactly one
	for (const struct SPmapIt *it = spmap_it(m); it; it = spmap_it_next(it)) {

		enum LogThreshold threshold = log_threshold_val(it->key);
		char *line = yaml_scalar_to_string(c, it->val);

		if (threshold && line)
			plist_append(log_cap_lines, log_cap_line_init(threshold, line));

		free(line);
	}

	spmap_free(m);
}

char *yaml_scalar_to_name_desc(struct UC *c, const yaml_node_t *scalar) {
	char *name_desc = yaml_scalar_to_string(c, scalar);
	if (!name_desc)
		return NULL;

	if (yaml_valid_regex(c, name_desc))
		return name_desc;

	free(name_desc);
	return NULL;
}

unsigned int yaml_scalar_to_scale_round_to(struct UC *c, const yaml_node_t *scalar) {
	float val;
	unsigned int ret;

	yaml_unmarshal_log_def(c, scale_round_to_name(SCALE_ROUND_TO_DEFAULT));
	yaml_unmarshal_log_enum_names(c, scale_round_to_names);

	if (!yaml_scalar_to_float(c, &val, scalar)) {
		ret = SCALE_ROUND_TO_DEFAULT;
		goto end;
	}

	ret = scale_round_to_val(val);
	if (!ret) {
		yaml_unmarshal_log_invalid_value(c, scalar->data.scalar.value, "number");
		ret = SCALE_ROUND_TO_DEFAULT;
		goto end;
	}

end:
	yaml_unmarshal_log_def(c, NULL);
	yaml_unmarshal_log_enum_names(c, NULL);

	return ret;
}

void yaml_seq_into_name_desc_sset(struct UC *c, const struct Sset* const sset, const yaml_node_t *seq) {
	// TODO expect string, not scalar
	if (!sset || !yaml_check_node_type(c, seq, YAML_SEQUENCE_NODE, 0))
		return;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *scalar = yaml_document_get_node(&c->d, *item);
		if (!scalar)
			continue;

		char *val = NULL;
		if ((val = yaml_scalar_to_name_desc(c, scalar))) {
			sset_add(sset, val);
			free(val);
		}
	}
}


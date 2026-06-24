#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>
#include <yaml.h>

#include "yaml/unmarshal-types.h"

#include "cfg.h"
#include "cfg/disabled.h"
#include "cfg/user-mode.h"
#include "cfg/user-scale.h"
#include "cfg/user-transform.h"
#include "conditions.h"
#include "convert.h"
#include "fn.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "sset.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml/unmarshal-primitives.h"
#include "yaml/unmarshal.h"

void *yaml_root_to_cfg(struct UC *c, const yaml_node_t *root) {
	if (!root)
		return NULL;

	// log warnings and skip failures
	c->t = WARNING;

	if (!yaml_check_node_type(c, root, YAML_MAPPING_NODE))
		return NULL;

	return yaml_map_to_cfg(c, root);
}

void *yaml_root_to_ipc_request(struct UC *c, const yaml_node_t *root) {
	if (!root)
		return NULL;

	// log exceptions and fail for required fields
	c->t = ERROR;

	const struct SMap *nodes = yaml_map_to_node_table(c, root);
	if (!nodes)
		return NULL;

	struct IpcRequest *ipc_request = (struct IpcRequest*)calloc(1, sizeof(struct IpcRequest));

	yaml_unmarshal_log_ctx_top(c, "OP");
	const yaml_node_t *op = smap_get(nodes, "OP");
	if (!yaml_check_mandatory(c, op) || !(ipc_request->command = yaml_scalar_to_enum(c, op, ipc_command_val, ipc_command_names)))
		goto err;

	// log warnings for remainder
	c->t = WARNING;

	yaml_unmarshal_log_ctx_top(c, "LOG_THRESHOLD");
	const yaml_node_t *log_threshold = smap_get(nodes, "LOG_THRESHOLD");
	if (log_threshold)
		ipc_request->log_threshold = yaml_scalar_to_enum(c, log_threshold, log_threshold_val, log_threshold_names);

	yaml_unmarshal_log_ctx_top(c, "CFG");
	const yaml_node_t *cfg = smap_get(nodes, "CFG");
	if (cfg)
		ipc_request->cfg = yaml_map_to_cfg(c, cfg);

	goto end;

err:
	ipc_request_free(ipc_request);
	ipc_request = NULL;

end:
	smap_free(nodes);

	return ipc_request;
}

void *yaml_root_to_ipc_response_list(struct UC *c, const yaml_node_t *root) {
	if (!root)
		return NULL;

	struct SList *ipc_responses = NULL;

	if (root->type != YAML_MAPPING_NODE && root->type != YAML_SEQUENCE_NODE) {
		log_error(NULL);
		log_error("%s: expected %s or %s, got %s", *c->prefix ? c->prefix : "", yaml_node_type_str(YAML_MAPPING_NODE), yaml_node_type_str(YAML_SEQUENCE_NODE), yaml_node_type_str(root->type));
		goto err;
	}

	if (root->type == YAML_SEQUENCE_NODE) {
		for (const yaml_node_item_t *item = root->data.sequence.items.start; item < root->data.sequence.items.top; item ++) {
			yaml_map_into_ipc_responses(c, &ipc_responses, yaml_document_get_node(&c->d, *item));
		}
	} else {
		yaml_map_into_ipc_responses(c, &ipc_responses, root);
	}

	goto end;

err:
	slist_free_vals(&ipc_responses, (fn_free)ipc_response_free);
	ipc_responses = NULL;

end:
	return ipc_responses;
}

struct Cfg *yaml_map_to_cfg(struct UC *c, const yaml_node_t *map) {
	if (!map)
		return NULL;

	struct Cfg *cfg = cfg_init();

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *key = yaml_document_get_node(&c->d, pair->key);
		if (!key || key->type != YAML_SCALAR_NODE || !key->data.scalar.value)
			continue;

		const yaml_node_t *value = yaml_document_get_node(&c->d, pair->value);
		if (!value)
			continue;

		yaml_unmarshal_log_ctx_top(c, (char*)key->data.scalar.value);

		switch (cfg_element_val((char*)key->data.scalar.value)) {
			case ARRANGE:
				cfg->arrange = yaml_scalar_to_enum_def(c, ARRANGE_DEFAULT, value, arrange_val_start, arrange_name, arrange_names);
				break;
			case ALIGN:
				cfg->align = yaml_scalar_to_enum_def(c, ALIGN_DEFAULT, value, align_val_start, align_name, align_names);
				break;
			case ORDER:
				cfg->order_name_desc = yaml_seq_to_name_desc_list(c, value);
				break;
			case SCALING:
				cfg->scaling  = yaml_scalar_to_enum_def(c, SCALING_DEFAULT, value, on_off_val, on_off_name, on_off_names);
				break;
			case AUTO_SCALE:
				cfg->auto_scale = yaml_scalar_to_enum_def(c, AUTO_SCALE_DEFAULT, value, on_off_val, on_off_name, on_off_names);
				break;
			case SCALE:
				yaml_seq_into_col(c, value, cfg->user_scales, yaml_map_into_user_scales);
				break;
			case SCALE_ROUND_TO:
				cfg->scale_round_to = yaml_scalar_to_scale_round_to(c, value);
				break;
			case SCALE_ROUND_STRATEGY:
				cfg->scale_round_strategy = yaml_scalar_to_enum_def(c, SCALE_ROUND_STRATEGY_DEFAULT, value, scale_round_strategy_val, scale_round_strategy_name, scale_round_strategy_names);
				break;
			case MODE:
				yaml_seq_into_col(c, value, cfg->user_modes, yaml_map_into_user_modes);
				break;
			case TRANSFORM:
				yaml_seq_into_col(c, value, cfg->user_transforms, yaml_map_into_user_transforms);
				break;
			case VRR_OFF:
				yaml_seq_into_name_desc_sset(c, cfg->adaptive_sync_off, value);
				break;
			case CHANGE_SUCCESS_CMD:
			case CALLBACK_CMD:
				free(cfg->callback_cmd); // may be both entries present, use the last
				cfg->callback_cmd = yaml_scalar_to_string_def(c, CALLBACK_CMD_DEFAULT, value);
				break;
			case LAPTOP_DISPLAY_PREFIX:
				cfg->laptop_display_prefix = yaml_scalar_to_string(c, value);
				break;
			case LAPTOP_LID_MONITOR:
				cfg->laptop_lid_monitor = yaml_scalar_to_enum_def(c, LAPTOP_LID_MONITOR_DEFAULT, value, on_off_val, on_off_name, on_off_names);
				break;
			case MAX_PREFERRED_REFRESH:
				cfg->max_preferred_refresh_name_desc = yaml_seq_to_name_desc_list(c, value);
				break;
			case LOG_THRESHOLD:
				cfg->log_threshold = yaml_scalar_to_enum(c, value, log_threshold_val, log_threshold_names);
				break;
			case DISABLED:
				yaml_seq_into_col(c, value, cfg->disableds, yaml_node_into_disableds);
				break;
			case AUTO_SCALE_DPI:
				yaml_scalar_to_int_def(c, &cfg->auto_scale_dpi, AUTO_SCALE_DPI_DEFAULT, value);
				break;
			case AUTO_SCALE_MIN:
				yaml_scalar_to_float_def(c, &cfg->auto_scale_min, AUTO_SCALE_MIN_DEFAULT, value);
				break;
			case AUTO_SCALE_MAX:
				yaml_scalar_to_float_def(c, &cfg->auto_scale_max, AUTO_SCALE_MAX_DEFAULT, value);
				break;
			default:
				// ignore unexpected
				break;
		}
	}

	return cfg;
}

void yaml_map_into_ipc_responses(struct UC *c, const void *col, const yaml_node_t *map) {
	if (!col)
		return;

	// log exceptions and fail for required fields
	c->t = ERROR;

	const struct SMap *nodes = yaml_map_to_node_table(c, map);
	if (!nodes)
		return;

	struct SList **ipc_responses = (struct SList**)col;
	struct IpcResponse *ipc_response = (struct IpcResponse*)calloc(1, sizeof(struct IpcResponse));

	yaml_unmarshal_log_ctx_top(c, "DONE");
	const yaml_node_t *done = smap_get(nodes, "DONE");
	if (!yaml_check_mandatory(c, done) || !yaml_scalar_to_boolean(c, &ipc_response->status.done, done))
		goto err;

	yaml_unmarshal_log_ctx_top(c, "RC");
	const yaml_node_t *rc = smap_get(nodes, "RC");
	if (!yaml_check_mandatory(c, rc) || !yaml_scalar_to_int(c, &ipc_response->status.rc, rc))
		goto err;

	// suppress validation failures for remainder
	c->t = 0;

	yaml_unmarshal_log_ctx_top(c, "CFG");
	const yaml_node_t *cfg = smap_get(nodes, "CFG");
	if (cfg)
		ipc_response->cfg = yaml_map_to_cfg(c, cfg);

	yaml_unmarshal_log_ctx_top(c, "STATE");
	const yaml_node_t *state = smap_get(nodes, "STATE");
	if (state) {
		const struct SMap *nodes_state = yaml_map_to_node_table(c, state);
		if (nodes_state) {

			ipc_response->lid =	yaml_map_to_lid(c, smap_get(nodes_state, "LID"));

			yaml_seq_into_col(c, smap_get(nodes_state, "HEADS"), &ipc_response->heads, yaml_map_into_heads);

			smap_free(nodes_state);
		}
	}

	yaml_unmarshal_log_ctx_top(c, "MESSAGES");
	const yaml_node_t *messages = smap_get(nodes, "MESSAGES");
	if (messages) {
		ipc_response->log_cap_lines = yaml_seq_to_log_cap_lines(c, messages);
	}

	slist_append(ipc_responses, ipc_response);

	goto end;

err:
	ipc_response_free(ipc_response);
	ipc_response = NULL;

end:
	smap_free(nodes);
}

void yaml_map_into_conditions(struct UC *c, const void *col, const yaml_node_t *map) {
	if (!col)
		return;

	const struct SMap *nodes = yaml_map_to_node_table(c, map);
	if (!nodes)
		return;

	struct SList **conditions = (struct SList**)col;
	struct Condition *condition = (struct Condition*)calloc(1, sizeof(struct Condition));

	yaml_unmarshal_log_ctx_key(c, "PLUGGED");
	const yaml_node_t *node = smap_get(nodes, "PLUGGED");
	if (node && !(condition->plugged = yaml_seq_to_name_desc_list(c, node)))
		goto err;

	yaml_unmarshal_log_ctx_key(c, "UNPLUGGED");
	node = smap_get(nodes, "UNPLUGGED");
	if (node && !(condition->unplugged = yaml_seq_to_name_desc_list(c, node)))
		goto err;

	yaml_unmarshal_log_ctx_key(c, "LID");
	node = smap_get(nodes, "LID");
	if (node && !(condition->lid = yaml_scalar_to_enum(c, node, condition_lid_val, condition_lid_names)))
		goto err;

	if (!condition->plugged && !condition->unplugged && !condition->lid)
		goto err;

	slist_append(conditions, condition);

	goto end;

err:
	condition_free(condition);

end:
	yaml_unmarshal_log_ctx_key(c, NULL);
	smap_free(nodes);
}

void yaml_map_into_user_scales(struct UC *c, const void *col, const yaml_node_t *map) {
	if (!col)
		return;

	const struct SMap *nodes = yaml_map_to_node_table(c, map);
	if (!nodes)
		return;

	const struct SMap *user_scales = col;
	struct UserScale *user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	const yaml_node_t *scalar = smap_get(nodes, "NAME_DESC");
	if (!yaml_check_mandatory(c, scalar) || !(name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto err;

	yaml_unmarshal_log_ctx_name_desc(c, name_desc);

	yaml_unmarshal_log_ctx_key(c, "SCALE");
	scalar = smap_get(nodes, "SCALE");
	if (!yaml_check_mandatory(c, scalar) || !yaml_scalar_to_float(c, &user_scale->scale, scalar))
		goto err;

	if (smap_put_if_absent(user_scales, name_desc, user_scale)) {
		yaml_unmarshal_log_remove_duplicate_value(c, name_desc);
		goto err;
	}

	goto end;

err:
	user_scale_free(user_scale);

end:
	free(name_desc);
	smap_free(nodes);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);

	return;
}

void yaml_map_into_user_modes(struct UC *c, const void *col, const yaml_node_t *map) {
	if (!col)
		return;

	const struct SMap *nodes = yaml_map_to_node_table(c, map);
	if (!nodes)
		return;

	const struct SMap *user_modes = col;
	struct UserMode *user_mode = user_mode_init_default();

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	const yaml_node_t *scalar = smap_get(nodes, "NAME_DESC");
	if (!yaml_check_mandatory(c, scalar) || !(name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto err;

	yaml_unmarshal_log_ctx_name_desc(c, name_desc);

	yaml_unmarshal_log_ctx_key(c, "WIDTH");
	scalar = smap_get(nodes, "WIDTH");
	if (scalar && !yaml_scalar_to_int(c, &user_mode->width, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key(c, "HEIGHT");
	scalar = smap_get(nodes, "HEIGHT");
	if (scalar && !yaml_scalar_to_int(c, &user_mode->height, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key(c, "HZ");
	scalar = smap_get(nodes, "HZ");
	if (scalar) {
		float hz = 0;
		if (!yaml_scalar_to_float(c, &hz, scalar))
			goto err;
		user_mode->refresh_mhz = lround(hz * 1000);
	}

	yaml_unmarshal_log_ctx_key(c, "MAX");
	scalar = smap_get(nodes, "MAX");
	if (scalar && !yaml_scalar_to_boolean(c, &user_mode->max, scalar))
		goto err;

	if (smap_put_if_absent(user_modes, name_desc, user_mode)) {
		yaml_unmarshal_log_remove_duplicate_value(c, name_desc);
		goto err;
	}

	goto end;

err:
	user_mode_free(user_mode);

end:
	free(name_desc);
	smap_free(nodes);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);
}

void yaml_map_into_user_transforms(struct UC *c, const void *col, const yaml_node_t *map) {
	if (!col)
		return;

	const struct SMap *nodes = yaml_map_to_node_table(c, map);
	if (!nodes)
		return;

	const struct SMap *user_transforms = col;
	struct UserTransform *user_transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	const yaml_node_t *scalar = smap_get(nodes, "NAME_DESC");
	if (!yaml_check_mandatory(c, scalar) || !(name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto err;

	yaml_unmarshal_log_ctx_name_desc(c, name_desc);

	yaml_unmarshal_log_ctx_key(c, "TRANSFORM");
	scalar = smap_get(nodes, "TRANSFORM");
	if (!yaml_check_mandatory(c, scalar) || !(user_transform->transform = yaml_scalar_to_enum(c, scalar, transform_val, transform_names)))
		goto err;

	if (smap_put_if_absent(user_transforms, name_desc, user_transform)) {
		yaml_unmarshal_log_remove_duplicate_value(c, name_desc);
		goto err;
	}

	goto end;

err:
	cfg_user_transform_free(user_transform);

end:
	free(name_desc);
	smap_free(nodes);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);
}

struct Lid *yaml_map_to_lid(struct UC *c, const yaml_node_t *map) {
	const struct SMap *nodes = yaml_map_to_node_table(c, map);
	if (!nodes)
		return NULL;

	struct Lid *lid = (struct Lid*)calloc(1, sizeof(struct Lid));

	lid->device_path = yaml_scalar_to_string(c, smap_get(nodes, "DEVICE_PATH"));
	yaml_scalar_to_boolean(c, &lid->closed, smap_get(nodes, "CLOSED"));

	smap_free(nodes);

	return lid;
}

struct Mode *yaml_map_to_mode(struct UC *c, const yaml_node_t *map) {
	const struct SMap *nodes = yaml_map_to_node_table(c, map);
	if (!nodes)
		return NULL;

	struct Mode *mode = (struct Mode*)calloc(1, sizeof(struct Mode));

	yaml_scalar_to_int(c, &mode->width, smap_get(nodes, "WIDTH"));
	yaml_scalar_to_int(c, &mode->height, smap_get(nodes, "HEIGHT"));
	yaml_scalar_to_int(c, &mode->refresh_mhz, smap_get(nodes, "REFRESH_MHZ"));
	yaml_scalar_to_boolean(c, &mode->preferred, smap_get(nodes, "PREFERRED"));

	smap_free(nodes);

	return mode;
}

void yaml_map_into_modes(struct UC *c, const void *col, const yaml_node_t *map) {
	const struct SMap *nodes = yaml_map_to_node_table(c, map);
	if (!nodes)
		return;

	struct Mode *mode = yaml_map_to_mode(c, map);

	if (mode) {
		slist_append((struct SList**)col, mode);
	}

	smap_free(nodes);
}

void yaml_map_into_heads(struct UC *c, const void *col, const yaml_node_t *map) {
	const struct SMap *nodes = yaml_map_to_node_table(c, map);
	if (!nodes)
		return;

	struct SList **heads = (struct SList**)col;
	struct Head *head = (struct Head*)calloc(1, sizeof(struct Head));

	head->name = yaml_scalar_to_string(c, smap_get(nodes, "NAME"));
	head->description = yaml_scalar_to_string(c, smap_get(nodes, "DESCRIPTION"));
	head->make = yaml_scalar_to_string(c, smap_get(nodes, "MAKE"));
	head->model = yaml_scalar_to_string(c, smap_get(nodes, "MODEL"));
	head->serial_number = yaml_scalar_to_string(c, smap_get(nodes, "SERIAL_NUMBER"));
	yaml_scalar_to_int(c, &head->width_mm, smap_get(nodes, "WIDTH_MM"));
	yaml_scalar_to_int(c, &head->height_mm, smap_get(nodes, "HEIGHT_MM"));

	yaml_map_to_head_state(c, &head->current, smap_get(nodes,"CURRENT"));
	yaml_map_to_head_state(c, &head->desired, smap_get(nodes,"DESIRED"));

	yaml_seq_into_col(c, smap_get(nodes, "MODES"), &head->modes, yaml_map_into_modes);

	const struct SMap *nodes_overrides = yaml_map_to_node_table(c, smap_get(nodes, "OVERRIDES"));
	if (nodes_overrides) {
		bool disabled;
		if (yaml_scalar_to_boolean(c, &disabled, smap_get(nodes_overrides, "DISABLED"))) {
			head->overrided_enabled = disabled ? OverrideFalse : OverrideTrue;
		}
	}

	slist_append(heads, head);

	smap_free(nodes);
	smap_free(nodes_overrides);
}

void yaml_node_into_disableds(struct UC *c, const void *col, const yaml_node_t *node) {
	if (!col)
		return;

	const struct PSet *disableds = col;
	struct Disabled *disabled = NULL;

	const struct SMap *node_map = NULL;

	switch (node->type) {
		case YAML_SCALAR_NODE:
			{
				disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));
				if (!(disabled->name_desc = yaml_scalar_to_name_desc(c, node)))
					goto err;

				pset_add(disableds, disabled);

				break;
			}

		case YAML_MAPPING_NODE:
			{
				if (!(node_map = yaml_map_to_node_table(c, node)))
					return;

				disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));

				yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
				const yaml_node_t *scalar = smap_get(node_map, "NAME_DESC");
				if (!yaml_check_mandatory(c, scalar) || !(disabled->name_desc = yaml_scalar_to_name_desc(c, scalar)))
					goto err;

				yaml_unmarshal_log_ctx_name_desc(c, disabled->name_desc);

				yaml_unmarshal_log_ctx_key(c, "IF");
				const yaml_node_t *map = smap_get(node_map, "IF");
				if (map)
					yaml_seq_into_col(c, map, &disabled->conditions, yaml_map_into_conditions);

				pset_add(disableds, disabled);

				break;
			}

		default:
			log_warn("Ignoring invalid DISABLED expected scalar or map, got %s", yaml_node_type_str(node->type));
			goto err;
			break;
	}

	goto end;

err:
	disabled_free(disabled);

end:
	smap_free(node_map);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);
}

bool yaml_map_to_head_state(struct UC *c, struct HeadState *head_state, const yaml_node_t *map) {
	const struct SMap *nodes = yaml_map_to_node_table(c, map);
	if (!nodes)
		return false;

	yaml_scalar_to_boolean(c, &head_state->enabled, smap_get(nodes, "ENABLED"));

	yaml_scalar_to_int(c, &head_state->x, smap_get(nodes, "X"));
	yaml_scalar_to_int(c, &head_state->y, smap_get(nodes, "Y"));

	head_state->transform = yaml_scalar_to_enum(c, smap_get(nodes, "TRANSFORM"), transform_val, NULL);

	float scale;
	if (yaml_scalar_to_float(c, &scale, smap_get(nodes, "SCALE")))
		head_state->scale = wl_fixed_from_double(scale);

	bool vrr = false;
	if (yaml_scalar_to_boolean(c, &vrr, smap_get(nodes, "VRR")))
		head_state->adaptive_sync = vrr ? ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED : ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	head_state->mode = yaml_map_to_mode(c, smap_get(nodes, "MODE"));

	smap_free(nodes);

	return true;
}

struct SList *yaml_seq_to_log_cap_lines(struct UC *c, const yaml_node_t *seq) {
	if (!yaml_check_node_type(c, seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(&c->d, *item);
		if (!node)
			continue;

		const struct SMap *nodes_line = yaml_map_to_node_table(c, node);
		if (!nodes_line)
			return NULL;

		// unmarshal many pairs even though schema specifies exactly one
		for (const struct SMapIt *i = smap_it(nodes_line); i; i = smap_it_next(i)) {

			enum LogThreshold threshold = log_threshold_val(i->key);
			char *line = yaml_scalar_to_string(c, i->val);

			if (threshold && line) {
				struct LogCapLine *log_cap_line = (struct LogCapLine*)calloc(1, sizeof(struct LogCapLine));
				log_cap_line->threshold = threshold;
				log_cap_line->line = strdup(line);
				slist_append(&list, log_cap_line);

			}

			free(line);
		}

		smap_free(nodes_line);
	}

	return list;
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
	c->valid_names_fn = scale_round_to_names;

	if (!yaml_scalar_to_float(c, &val, scalar)) {
		ret = SCALE_ROUND_TO_DEFAULT;
		goto end;
	}

	ret = scale_round_to_val(val);
	if (!ret) {
		yaml_unmarshal_log_invalid_value(c, scalar->data.scalar.value);
		ret = SCALE_ROUND_TO_DEFAULT;
		goto end;
	}

end:
	yaml_unmarshal_log_def(c, NULL);
	c->valid_names_fn = NULL;

	return ret;
}

struct SList *yaml_seq_to_name_desc_list(struct UC *c, const yaml_node_t *seq) {
	if (!yaml_check_node_type(c, seq, YAML_SEQUENCE_NODE))
		return NULL;

	const struct SSet *set = sset_init();

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *scalar = yaml_document_get_node(&c->d, *item);
		if (!scalar)
			continue;

		char *val = NULL;
		if ((val = yaml_scalar_to_name_desc(c, scalar))) {
			sset_add(set, val);
			free(val);
		}
	}

	struct SList *list = sset_slist_deep(set);

	sset_free(set);

	return list;
}

void yaml_seq_into_name_desc_sset(struct UC *c, const struct SSet *sset, const yaml_node_t *seq) {
	if (!sset || !yaml_check_node_type(c, seq, YAML_SEQUENCE_NODE))
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


#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>
#include <yaml.h>

#include "yaml/unmarshal-types.h"

#include "cfg.h"
#include "conditions.h"
#include "convert.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "slist.h"
#include "stable.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml/unmarshal-primitives.h"

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

	const struct STable *table = yaml_map_to_node_table(c, root);
	if (!table)
		return NULL;

	struct IpcRequest *ipc_request = (struct IpcRequest*)calloc(1, sizeof(struct IpcRequest));

	yaml_unmarshal_log_ctx_top(c, "OP");
	const yaml_node_t *op = stable_get(table, "OP");
	if (!yaml_check_mandatory(c, op) || !(ipc_request->command = yaml_scalar_to_enum(c, op, ipc_command_val, ipc_command_names)))
		goto err;

	// log warnings for remainder
	c->t = WARNING;
	// TODO sort out prefixes once and for all
	yaml_unmarshal_log_prefix(c, NULL);

	yaml_unmarshal_log_ctx_top(c, "LOG_THRESHOLD");
	const yaml_node_t *log_threshold = stable_get(table, "LOG_THRESHOLD");
	if (log_threshold)
		ipc_request->log_threshold = yaml_scalar_to_enum(c, log_threshold, log_threshold_val, log_threshold_names);

	yaml_unmarshal_log_ctx_top(c, "CFG");
	const yaml_node_t *cfg = stable_get(table, "CFG");
	if (cfg)
		ipc_request->cfg = yaml_map_to_cfg(c, cfg);

	goto end;

err:
	ipc_request_free(ipc_request);
	ipc_request = NULL;

end:
	stable_free(table);

	return ipc_request;
}

void *yaml_root_to_ipc_response_list(struct UC *c, const yaml_node_t *root) {
	if (!root)
		return NULL;

	struct SList *ipc_responses = NULL;

	if (root->type != YAML_MAPPING_NODE && root->type != YAML_SEQUENCE_NODE) {
		log_error("");
		log_error("parsing ipc response: expected %s or %s, got %s", yaml_node_type_str(YAML_MAPPING_NODE), yaml_node_type_str(YAML_SEQUENCE_NODE), yaml_node_type_str(root->type));
		goto err;
	}

	if (root->type == YAML_SEQUENCE_NODE) {
		for (const yaml_node_item_t *item = root->data.sequence.items.start; item < root->data.sequence.items.top; item ++) {
			const yaml_node_t *node = yaml_document_get_node(&c->d, *item);
			if (!node)
				continue;

			struct IpcResponse *ipc_response = yaml_map_to_ipc_response(c, node);
			if (ipc_response)
				slist_append(&ipc_responses, ipc_response);
		}
	} else {
		struct IpcResponse *ipc_response = yaml_map_to_ipc_response(c, root);
		if (ipc_response)
			slist_append(&ipc_responses, ipc_response);
	}

	goto end;

err:
	slist_free_vals(&ipc_responses, ipc_response_free);
	ipc_responses = NULL;

end:
	return ipc_responses;
}

void *yaml_map_to_cfg(struct UC *c, const yaml_node_t *map) {
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
				cfg->user_scales = yaml_seq_to_type_list(c, value, yaml_map_to_user_scale);
				break;
			case SCALE_ROUND_TO:
				cfg->scale_round_to = yaml_scalar_to_scale_round_to(c, value);
				break;
			case SCALE_ROUND_STRATEGY:
				cfg->scale_round_strategy = yaml_scalar_to_enum_def(c, SCALE_ROUND_STRATEGY_DEFAULT, value, scale_round_strategy_val, scale_round_strategy_name, scale_round_strategy_names);
				break;
			case MODE:
				cfg->user_modes = yaml_seq_to_type_list(c, value, yaml_map_to_user_mode);
				break;
			case TRANSFORM:
				cfg->user_transforms = yaml_seq_to_type_list(c, value, yaml_map_to_user_transform);
				break;
			case VRR_OFF:
				cfg->adaptive_sync_off_name_desc = yaml_seq_to_name_desc_list(c, value);
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
				cfg->disabled = yaml_seq_to_type_list(c, value, yaml_node_to_disabled);
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

void *yaml_map_to_ipc_response(struct UC *c, const yaml_node_t *map) {

	// log exceptions and fail for required fields
	c->t = ERROR;

	const struct STable *table = yaml_map_to_node_table(c, map);
	if (!table)
		return NULL;

	struct IpcResponse *ipc_response = (struct IpcResponse*)calloc(1, sizeof(struct IpcResponse));

	yaml_unmarshal_log_ctx_top(c, "DONE");
	const yaml_node_t *done = stable_get(table, "DONE");
	if (!yaml_check_mandatory(c, done) || !yaml_scalar_to_boolean(c, &ipc_response->status.done, done))
		goto err;

	yaml_unmarshal_log_ctx_top(c, "RC");
	const yaml_node_t *rc = stable_get(table, "RC");
	if (!yaml_check_mandatory(c, rc) || !yaml_scalar_to_int(c, &ipc_response->status.rc, rc))
		goto err;

	// suppress validation failures for remainder
	c->t = 0;

	yaml_unmarshal_log_ctx_top(c, "CFG");
	const yaml_node_t *cfg = stable_get(table, "CFG");
	if (cfg)
		ipc_response->cfg = yaml_map_to_cfg(c, cfg);

	yaml_unmarshal_log_ctx_top(c, "STATE");
	const yaml_node_t *state = stable_get(table, "STATE");
	if (state) {
		const struct STable *table_state = yaml_map_to_node_table(c, state);
		if (table_state) {

			ipc_response->lid =	yaml_map_to_lid(c, stable_get(table_state, "LID"));

			ipc_response->heads = yaml_seq_to_type_list(c, stable_get(table_state, "HEADS"), yaml_map_to_head);

			stable_free(table_state);
		}
	}

	yaml_unmarshal_log_ctx_top(c, "MESSAGES");
	const yaml_node_t *messages = stable_get(table, "MESSAGES");
	if (messages) {
		ipc_response->log_cap_lines = yaml_seq_to_log_cap_lines(c, messages);
	}

	goto end;

err:
	ipc_response_free(ipc_response);
	ipc_response = NULL;

end:
	stable_free(table);

	return ipc_response;
}

void *yaml_map_to_condition(struct UC *c, const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(c, map);
	if (!table)
		return NULL;

	const yaml_node_t *node;

	struct Condition *condition = (struct Condition*)calloc(1, sizeof(struct Condition));

	yaml_unmarshal_log_ctx_key(c, "PLUGGED");
	node = stable_get(table, "PLUGGED");
	if (node && !(condition->plugged = yaml_seq_to_name_desc_list(c, node)))
		goto err;

	yaml_unmarshal_log_ctx_key(c, "UNPLUGGED");
	node = stable_get(table, "UNPLUGGED");
	if (node && !(condition->unplugged = yaml_seq_to_name_desc_list(c, node)))
		goto err;

	yaml_unmarshal_log_ctx_key(c, "LID");
	node = stable_get(table, "LID");
	if (node && !(condition->lid = yaml_scalar_to_enum(c, node, condition_lid_val, condition_lid_names)))
		goto err;

	if (!condition->plugged && !condition->unplugged && !condition->lid)
		goto err;

	goto end;

err:
	condition_free(condition);
	condition = NULL;

end:
	yaml_unmarshal_log_ctx_key(c, NULL);
	stable_free(table);

	return condition;
}

void *yaml_map_to_user_scale(struct UC *c, const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(c, map);
	if (!table)
		return NULL;

	const yaml_node_t *scalar;

	struct UserScale *user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	scalar = stable_get(table, "NAME_DESC");
	if (!yaml_check_mandatory(c, scalar) || !(user_scale->name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto err;

	yaml_unmarshal_log_ctx_name_desc(c, user_scale->name_desc);

	yaml_unmarshal_log_ctx_key(c, "SCALE");
	scalar = stable_get(table, "SCALE");
	if (!yaml_check_mandatory(c, scalar) || !yaml_scalar_to_float(c, &user_scale->scale, scalar))
		goto err;

	goto end;

err:
	cfg_user_scale_free(user_scale);
	user_scale = NULL;

end:
	stable_free(table);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);

	return user_scale;
}

void *yaml_map_to_user_mode(struct UC *c, const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(c, map);
	if (!table)
		return NULL;

	const yaml_node_t *scalar;

	struct UserMode *user_mode = cfg_user_mode_default();

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	scalar = stable_get(table, "NAME_DESC");
	if (!yaml_check_mandatory(c, scalar) || !(user_mode->name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto err;

	yaml_unmarshal_log_ctx_name_desc(c, user_mode->name_desc);

	yaml_unmarshal_log_ctx_key(c, "WIDTH");
	scalar = stable_get(table, "WIDTH");
	if (scalar && !yaml_scalar_to_int(c, &user_mode->width, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key(c, "HEIGHT");
	scalar = stable_get(table, "HEIGHT");
	if (scalar && !yaml_scalar_to_int(c, &user_mode->height, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key(c, "HZ");
	scalar = stable_get(table, "HZ");
	if (scalar) {
		float hz = 0;
		if (!yaml_scalar_to_float(c, &hz, scalar))
			goto err;
		user_mode->refresh_mhz = lround(hz * 1000);
	}

	yaml_unmarshal_log_ctx_key(c, "MAX");
	scalar = stable_get(table, "MAX");
	if (scalar && !yaml_scalar_to_boolean(c, &user_mode->max, scalar))
		goto err;

	goto end;

err:
	cfg_user_mode_free(user_mode);
	user_mode = NULL;

end:
	stable_free(table);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);

	return user_mode;
}

void *yaml_map_to_user_transform(struct UC *c, const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(c, map);
	if (!table)
		return NULL;

	struct UserTransform *user_transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

	const yaml_node_t *scalar;

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	scalar = stable_get(table, "NAME_DESC");
	if (!yaml_check_mandatory(c, scalar) || !(user_transform->name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto err;

	yaml_unmarshal_log_ctx_name_desc(c, user_transform->name_desc);

	yaml_unmarshal_log_ctx_key(c, "TRANSFORM");
	scalar = stable_get(table, "TRANSFORM");
	if (!yaml_check_mandatory(c, scalar) || !(user_transform->transform = yaml_scalar_to_enum(c, scalar, transform_val, transform_names)))
		goto err;

	goto end;

err:
	cfg_user_transform_free(user_transform);
	user_transform = NULL;

end:
	stable_free(table);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);

	return user_transform;
}

void *yaml_map_to_lid(struct UC *c, const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(c, map);
	if (!table)
		return NULL;

	struct Lid *lid = (struct Lid*)calloc(1, sizeof(struct Lid));

	lid->device_path = yaml_scalar_to_string(c, stable_get(table, "DEVICE_PATH"));
	yaml_scalar_to_boolean(c, &lid->closed, stable_get(table, "CLOSED"));

	stable_free(table);

	return lid;
}

void *yaml_map_to_mode(struct UC *c, const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(c, map);
	if (!table)
		return NULL;

	struct Mode *mode = (struct Mode*)calloc(1, sizeof(struct Mode));

	yaml_scalar_to_int(c, &mode->width, stable_get(table, "WIDTH"));
	yaml_scalar_to_int(c, &mode->height, stable_get(table, "HEIGHT"));
	yaml_scalar_to_int(c, &mode->refresh_mhz, stable_get(table, "REFRESH_MHZ"));
	yaml_scalar_to_boolean(c, &mode->preferred, stable_get(table, "PREFERRED"));

	stable_free(table);

	return mode;
}

void *yaml_map_to_head(struct UC *c, const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(c, map);
	if (!table)
		return NULL;

	struct Head *head = (struct Head*)calloc(1, sizeof(struct Head));

	head->name = yaml_scalar_to_string(c, stable_get(table, "NAME"));
	head->description = yaml_scalar_to_string(c, stable_get(table, "DESCRIPTION"));
	head->make = yaml_scalar_to_string(c, stable_get(table, "MAKE"));
	head->model = yaml_scalar_to_string(c, stable_get(table, "MODEL"));
	head->serial_number = yaml_scalar_to_string(c, stable_get(table, "SERIAL_NUMBER"));
	yaml_scalar_to_int(c, &head->width_mm, stable_get(table, "WIDTH_MM"));
	yaml_scalar_to_int(c, &head->height_mm, stable_get(table, "HEIGHT_MM"));

	yaml_map_to_head_state(c, &head->current, stable_get(table,"CURRENT"));
	yaml_map_to_head_state(c, &head->desired, stable_get(table,"DESIRED"));

	head->modes = yaml_seq_to_type_list(c, stable_get(table, "MODES"), yaml_map_to_mode);

	const struct STable *table_overrides = yaml_map_to_node_table(c, stable_get(table, "OVERRIDES"));
	if (table_overrides) {
		bool disabled;
		if (yaml_scalar_to_boolean(c, &disabled, stable_get(table_overrides, "DISABLED"))) {
			head->overrided_enabled = disabled ? OverrideFalse : OverrideTrue;
		}
	}

	stable_free(table);
	stable_free(table_overrides);

	return head;
}

void *yaml_node_to_disabled(struct UC *c, const yaml_node_t *node) {
	struct Disabled *disabled = NULL;
	const struct STable *table_map = NULL;

	switch (node->type) {
		case YAML_SCALAR_NODE:
			{
				disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));
				if (!(disabled->name_desc = yaml_scalar_to_name_desc(c, node)))
					goto err;
				break;
			}

		case YAML_MAPPING_NODE:
			{
				if (!(table_map = yaml_map_to_node_table(c, node)))
					return NULL;

				const yaml_node_t *scalar;

				disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));

				yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
				scalar = stable_get(table_map, "NAME_DESC");
				if (!yaml_check_mandatory(c, scalar) || !(disabled->name_desc = yaml_scalar_to_name_desc(c, scalar)))
					goto err;

				yaml_unmarshal_log_ctx_name_desc(c, disabled->name_desc);

				yaml_unmarshal_log_ctx_key(c, "IF");
				scalar = stable_get(table_map, "IF");
				if (scalar)
					disabled->conditions = yaml_seq_to_type_list(c, scalar, yaml_map_to_condition);

				break;
			}

		default:
			log_warn("Ignoring invalid DISABLED expected scalar or map, got %s", yaml_node_type_str(node->type));
			goto err;
			break;
	}

	goto end;

err:
	cfg_disabled_free(disabled);
	disabled = NULL;

end:
	stable_free(table_map);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);

	return disabled;
}

bool yaml_map_to_head_state(struct UC *c, struct HeadState *head_state, const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(c, map);
	if (!table)
		return false;

	yaml_scalar_to_boolean(c, &head_state->enabled, stable_get(table, "ENABLED"));

	yaml_scalar_to_int(c, &head_state->x, stable_get(table, "X"));
	yaml_scalar_to_int(c, &head_state->y, stable_get(table, "Y"));

	head_state->transform = yaml_scalar_to_enum(c, stable_get(table, "TRANSFORM"), transform_val, NULL);

	float scale;
	if (yaml_scalar_to_float(c, &scale, stable_get(table, "SCALE")))
		head_state->scale = wl_fixed_from_double(scale);

	bool vrr = false;
	if (yaml_scalar_to_boolean(c, &vrr, stable_get(table, "VRR")))
		head_state->adaptive_sync = vrr ? ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED : ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	head_state->mode = yaml_map_to_mode(c, stable_get(table, "MODE"));

	stable_free(table);

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

		const struct STable *table_line = yaml_map_to_node_table(c, node);
		if (!table_line)
			return NULL;

		// unmarshal many pairs even though schema specifies exactly one
		for (const struct STableIter *i = stable_iter(table_line); i; i = stable_iter_next(i)) {

			enum LogThreshold threshold = log_threshold_val(stable_iter_key(i));
			char *line = yaml_scalar_to_string(c, stable_iter_val(i));

			if (threshold && line) {
				struct LogCapLine *log_cap_line = (struct LogCapLine*)calloc(1, sizeof(struct LogCapLine));
				log_cap_line->threshold = threshold;
				log_cap_line->line = strdup(line);
				slist_append(&list, log_cap_line);

			}

			free(line);
		}

		stable_free(table_line);
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

	const struct STable *table = stable_init(10, 10, false);

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *scalar = yaml_document_get_node(&c->d, *item);
		if (!scalar)
			continue;

		char *val = NULL;
		if ((val = yaml_scalar_to_name_desc(c, scalar))) {
			stable_put(table, val, NULL);
			free(val);
		}
	}

	struct SList *list = stable_keys_slist(table);

	stable_free(table);

	return list;
}

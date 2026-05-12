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
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "slist.h"
#include "stable.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml/unmarshal-context.h"
#include "yaml/unmarshal-primitives.h"

void *map_to_condition(const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(map);
	if (!table)
		return NULL;

	const yaml_node_t *seq;

	struct Condition *condition = (struct Condition*)calloc(1, sizeof(struct Condition));

	yaml_log_ctx_key("PLUGGED");
	seq = stable_get(table, "PLUGGED");
	if (seq && !(condition->plugged = yaml_seq_to_name_desc_list(seq)))
		goto err;

	yaml_log_ctx_key("UNPLUGGED");
	seq = stable_get(table, "UNPLUGGED");
	if (seq && !(condition->unplugged = yaml_seq_to_name_desc_list(seq)))
		goto err;

	goto end;

err:
	condition_free(condition);
	condition = NULL;

end:
	yaml_log_ctx_key(NULL);
	stable_free(table);

	return condition;
}

void *node_to_disabled(const yaml_node_t *node) {
	struct Disabled *disabled = NULL;
	const struct STable *table_map = NULL;

	switch (node->type) {
		case YAML_SCALAR_NODE:
			{
				disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));
				if (!(disabled->name_desc = yaml_scalar_to_name_desc(node)))
					goto err;
				break;
			}

		case YAML_MAPPING_NODE:
			{
				if (!(table_map = yaml_map_to_node_table(node)))
					return NULL;

				const yaml_node_t *scalar;

				disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));

				yaml_log_ctx_key("NAME_DESC");
				scalar = stable_get(table_map, "NAME_DESC");
				if (!yaml_check_mandatory(scalar) || !(disabled->name_desc = yaml_scalar_to_name_desc(scalar)))
					goto err;

				yaml_log_ctx_name_desc(disabled->name_desc);

				yaml_log_ctx_key("IF");
				scalar = stable_get(table_map, "IF");
				if (scalar)
					disabled->conditions = yaml_seq_to_type_list(scalar, map_to_condition);

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
	yaml_log_ctx_key(NULL);
	yaml_log_ctx_name_desc(NULL);

	return disabled;
}

void *map_to_user_scale(const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(map);
	if (!table)
		return NULL;

	const yaml_node_t *scalar;

	struct UserScale *user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	yaml_log_ctx_key("NAME_DESC");
	scalar = stable_get(table, "NAME_DESC");
	if (!yaml_check_mandatory(scalar) || !(user_scale->name_desc = yaml_scalar_to_name_desc(scalar)))
		goto err;

	yaml_log_ctx_name_desc(user_scale->name_desc);

	yaml_log_ctx_key("SCALE");
	scalar = stable_get(table, "SCALE");
	if (!yaml_check_mandatory(scalar) || !yaml_scalar_to_float(&user_scale->scale, scalar))
		goto err;

	goto end;

err:
	cfg_user_scale_free(user_scale);
	user_scale = NULL;

end:
	stable_free(table);
	yaml_log_ctx_key(NULL);
	yaml_log_ctx_name_desc(NULL);

	return user_scale;
}

void *map_to_user_mode(const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(map);
	if (!table)
		return NULL;

	const yaml_node_t *scalar;

	struct UserMode *user_mode = cfg_user_mode_default();

	yaml_log_ctx_key("NAME_DESC");
	scalar = stable_get(table, "NAME_DESC");
	if (!yaml_check_mandatory(scalar) || !(user_mode->name_desc = yaml_scalar_to_name_desc(scalar)))
		goto err;

	yaml_log_ctx_name_desc(user_mode->name_desc);

	yaml_log_ctx_key("WIDTH");
	scalar = stable_get(table, "WIDTH");
	if (scalar && !yaml_scalar_to_int(&user_mode->width, scalar))
		goto err;

	yaml_log_ctx_key("HEIGHT");
	scalar = stable_get(table, "HEIGHT");
	if (scalar && !yaml_scalar_to_int(&user_mode->height, scalar))
		goto err;

	yaml_log_ctx_key("HZ");
	scalar = stable_get(table, "HZ");
	if (scalar) {
		float hz = 0;
		if (!yaml_scalar_to_float(&hz, scalar))
			goto err;
		user_mode->refresh_mhz = lround(hz * 1000);
	}

	yaml_log_ctx_key("MAX");
	scalar = stable_get(table, "MAX");
	if (scalar && !yaml_scalar_to_boolean(&user_mode->max, scalar))
		goto err;

	goto end;

err:
	cfg_user_mode_free(user_mode);
	user_mode = NULL;

end:
	stable_free(table);
	yaml_log_ctx_key(NULL);
	yaml_log_ctx_name_desc(NULL);

	return user_mode;
}

void *map_to_user_transform(const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(map);
	if (!table)
		return NULL;

	struct UserTransform *user_transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

	const yaml_node_t *scalar;

	yaml_log_ctx_key("NAME_DESC");
	scalar = stable_get(table, "NAME_DESC");
	if (!yaml_check_mandatory(scalar) || !(user_transform->name_desc = yaml_scalar_to_name_desc(scalar)))
		goto err;

	yaml_log_ctx_name_desc(user_transform->name_desc);

	yaml_log_ctx_key("TRANSFORM");
	scalar = stable_get(table, "TRANSFORM");
	if (!yaml_check_mandatory(scalar) || !(user_transform->transform = yaml_scalar_to_enum(scalar, transform_val)))
		goto err;

	goto end;

err:
	cfg_user_transform_free(user_transform);
	user_transform = NULL;

end:
	stable_free(table);
	yaml_log_ctx_key(NULL);
	yaml_log_ctx_name_desc(NULL);

	return user_transform;
}

void *map_to_lid(const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(map);
	if (!table)
		return NULL;

	struct Lid *lid = (struct Lid*)calloc(1, sizeof(struct Lid));

	lid->device_path = yaml_scalar_to_string(stable_get(table, "DEVICE_PATH"));
	yaml_scalar_to_boolean(&lid->closed, stable_get(table, "CLOSED"));

	stable_free(table);

	return lid;
}

void *map_to_mode(const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(map);
	if (!table)
		return NULL;

	struct Mode *mode = (struct Mode*)calloc(1, sizeof(struct Mode));

	yaml_scalar_to_int(&mode->width, stable_get(table, "WIDTH"));
	yaml_scalar_to_int(&mode->height, stable_get(table, "HEIGHT"));
	yaml_scalar_to_int(&mode->refresh_mhz, stable_get(table, "REFRESH_MHZ"));
	yaml_scalar_to_boolean(&mode->preferred, stable_get(table, "PREFERRED"));

	stable_free(table);

	return mode;
}

void map_to_head_state(struct HeadState *head_state, const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(map);
	if (!table)
		return;

	yaml_scalar_to_boolean(&head_state->enabled, stable_get(table, "ENABLED"));

	yaml_scalar_to_int(&head_state->x, stable_get(table, "X"));
	yaml_scalar_to_int(&head_state->y, stable_get(table, "Y"));

	head_state->transform = yaml_scalar_to_enum(stable_get(table, "TRANSFORM"), transform_val);

	float scale;
	if (yaml_scalar_to_float(&scale, stable_get(table, "SCALE")))
		head_state->scale = wl_fixed_from_double(scale);

	bool vrr = false;
	if (yaml_scalar_to_boolean(&vrr, stable_get(table, "VRR")))
		head_state->adaptive_sync = vrr ? ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED : ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	head_state->mode = map_to_mode(stable_get(table, "MODE"));

	stable_free(table);
}

void *map_to_head(const yaml_node_t *map) {
	const struct STable *table = yaml_map_to_node_table(map);
	if (!table)
		return NULL;

	struct Head *head = (struct Head*)calloc(1, sizeof(struct Head));

	head->name = yaml_scalar_to_string(stable_get(table, "NAME"));
	head->description = yaml_scalar_to_string(stable_get(table, "DESCRIPTION"));
	head->make = yaml_scalar_to_string(stable_get(table, "MAKE"));
	head->model = yaml_scalar_to_string(stable_get(table, "MODEL"));
	head->serial_number = yaml_scalar_to_string(stable_get(table, "SERIAL_NUMBER"));
	yaml_scalar_to_int(&head->width_mm, stable_get(table, "WIDTH_MM"));
	yaml_scalar_to_int(&head->height_mm, stable_get(table, "HEIGHT_MM"));

	map_to_head_state(&head->current, stable_get(table,"CURRENT"));
	map_to_head_state(&head->desired, stable_get(table,"DESIRED"));

	head->modes = yaml_seq_to_type_list(stable_get(table, "MODES"), map_to_mode);

	const struct STable *table_overrides = yaml_map_to_node_table(stable_get(table, "OVERRIDES"));
	if (table_overrides) {
		bool disabled;
		if (yaml_scalar_to_boolean(&disabled, stable_get(table_overrides, "DISABLED"))) {
			head->overrided_enabled = disabled ? OverrideFalse : OverrideTrue;
		}
	}

	stable_free(table);
	stable_free(table_overrides);

	return head;
}

bool map_to_cfg(struct Cfg *cfg, const yaml_node_t *map) {
	if (!cfg || !map)
		return false;

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *key = yaml_document_get_node(ctx.document, pair->key);
		if (!key || key->type != YAML_SCALAR_NODE || !key->data.scalar.value)
			continue;

		const yaml_node_t *value = yaml_document_get_node(ctx.document, pair->value);
		if (!value)
			continue;

		yaml_log_ctx_top((char*)key->data.scalar.value);

		switch (cfg_element_val((char*)key->data.scalar.value)) {
			case ARRANGE:
				cfg->arrange = yaml_scalar_to_enum_def(ARRANGE_DEFAULT, value, arrange_val_start, arrange_name);
				break;
			case ALIGN:
				cfg->align = yaml_scalar_to_enum_def(ALIGN_DEFAULT, value, align_val_start, align_name);
				break;
			case ORDER:
				cfg->order_name_desc = yaml_seq_to_name_desc_list(value);
				break;
			case SCALING:
				cfg->scaling  = yaml_scalar_to_enum_def(SCALING_DEFAULT, value, on_off_val, on_off_name);
				break;
			case AUTO_SCALE:
				cfg->auto_scale = yaml_scalar_to_enum_def(AUTO_SCALE_DEFAULT, value, on_off_val, on_off_name);
				break;
			case SCALE:
				cfg->user_scales = yaml_seq_to_type_list(value, map_to_user_scale);
				break;
			case MODE:
				cfg->user_modes = yaml_seq_to_type_list(value, map_to_user_mode);
				break;
			case TRANSFORM:
				cfg->user_transforms = yaml_seq_to_type_list(value, map_to_user_transform);
				break;
			case VRR_OFF:
				cfg->adaptive_sync_off_name_desc = yaml_seq_to_name_desc_list(value);
				break;
			case CHANGE_SUCCESS_CMD:
			case CALLBACK_CMD:
				scalar_to_callback_cmd(&cfg->callback_cmd, value);
				break;
			case LAPTOP_DISPLAY_PREFIX:
				cfg->laptop_display_prefix = yaml_scalar_to_string(value);
				break;
			case MAX_PREFERRED_REFRESH:
				cfg->max_preferred_refresh_name_desc = yaml_seq_to_name_desc_list(value);
				break;
			case LOG_THRESHOLD:
				cfg->log_threshold = yaml_scalar_to_enum(value, log_threshold_val);
				break;
			case DISABLED:
				cfg->disabled = yaml_seq_to_type_list(value, node_to_disabled);
				break;
			case AUTO_SCALE_MIN:
				yaml_scalar_to_float_def(&cfg->auto_scale_min, AUTO_SCALE_MIN_DEFAULT, value);
				break;
			case AUTO_SCALE_MAX:
				yaml_scalar_to_float_def(&cfg->auto_scale_max, AUTO_SCALE_MAX_DEFAULT, value);
				break;
			default:
				// ignore unexpected
				break;
		}
	}

	return true;
}

void scalar_to_callback_cmd(char **dst, const yaml_node_t *scalar) {
	if (*dst) {
		free(*dst);
		*dst = NULL;
	}

	yaml_log_ctx_def(CALLBACK_CMD_DEFAULT);

	*dst = yaml_scalar_to_string(scalar);

	if (!*dst) {
		*dst = strdup(CALLBACK_CMD_DEFAULT);
	} else if (*dst && strlen(*dst) == 0) {
		free(*dst);
		*dst = NULL;
	}

	yaml_log_ctx_def(NULL);
}

struct SList *seq_to_log_cap_lines(const yaml_node_t *seq) {
	if (!yaml_check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		const struct STable *table_line = yaml_map_to_node_table(node);
		if (!table_line)
			return NULL;

		// unmarshal many pairs even though schema specifies exactly one
		for (const struct STableIter *i = stable_iter(table_line); i; i = stable_iter_next(i)) {

			enum LogThreshold threshold = log_threshold_val(stable_iter_key(i));
			char *line = yaml_scalar_to_string(stable_iter_val(i));

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

char *yaml_scalar_to_name_desc(const yaml_node_t *scalar) {
	char *name_desc = yaml_scalar_to_string(scalar);
	if (!name_desc)
		return NULL;

	if (yaml_valid_regex(name_desc))
		return name_desc;

	free(name_desc);
	return NULL;
}

struct SList *yaml_seq_to_name_desc_list(const yaml_node_t *seq) {
	if (!yaml_check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	const struct STable *table = stable_init(10, 10, false);

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *scalar = yaml_document_get_node(ctx.document, *item);
		if (!scalar)
			continue;

		char *val = NULL;
		if ((val = yaml_scalar_to_name_desc(scalar))) {
			stable_put(table, val, NULL);
			free(val);
		}
	}

	struct SList *list = stable_keys_slist(table);

	stable_free(table);

	return list;
}

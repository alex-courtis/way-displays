#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-util.h>
#include <yaml.h>

#include "yaml-unmarshal.h"

#include "cfg.h"
#include "convert.h"
#include "conditions.h"
#include "ipc.h"
#include "head.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "slist.h"
#include "stable.h"
#include "wlr-output-management-unstable-v1.h"

// unmarshal a map of nodes
static const struct STable *map_to_node_table(const yaml_node_t *map) {
	if (!check_node_type(map, YAML_MAPPING_NODE))
		return NULL;

	const struct STable *table = stable_init(10, 10, false);

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *pair_key = yaml_document_get_node(ctx.document, pair->key);

		char *key = NULL;
		if (!(key = scalar_to_string(pair_key))) {
			stable_free(table);
			return NULL;
		}

		const yaml_node_t *pair_value = yaml_document_get_node(ctx.document, pair->value);

		if (key && pair_value)
			stable_put(table, key, pair_value);

		if (key)
			free(key);
	}

	return table;
}

// unmarshal a sequence of strings, removing duplicates
static struct SList *seq_to_string_list(const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	const struct STable *table = stable_init(10, 10, false);

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *scalar = yaml_document_get_node(ctx.document, *item);
		if (!scalar)
			continue;

		char *val = NULL;
		if ((val = scalar_to_string(scalar))) {
			stable_put(table, val, NULL);
			free(val);
		}
	}

	struct SList *list = stable_keys_slist(table);

	stable_free(table);

	return list;
}

// unmarshal a sequence of regex validated name_desc, removing duplicates
static struct SList *seq_to_name_desc_list(const yaml_node_t *seq) {
	struct SList *list = seq_to_string_list(seq);

	if (!list)
		return NULL;

	slist_remove_all_free(&list, invalid_regex, NULL, NULL);

	return list;
}

// unmarshal a CALLBACK_CMD dst, frees first, sets NULL on empty string, otherwise default
static void scalar_to_callback_cmd(char **dst, const yaml_node_t *scalar) {
	if (*dst) {
		free(*dst);
		*dst = NULL;
	}

	ctx_def(CALLBACK_CMD_DEFAULT);

	*dst = scalar_to_string(scalar);

	if (!*dst) {
		*dst = strdup(CALLBACK_CMD_DEFAULT);
	} else if (*dst && strlen(*dst) == 0) {
		free(*dst);
		*dst = NULL;
	}

	ctx_def(NULL);
}

// unmarshal an IF into a Condition
static struct Condition *map_to_condition(const yaml_node_t *map) {
	const struct STable *table = map_to_node_table(map);
	if (!table)
		return NULL;

	const yaml_node_t *seq;

	struct Condition *condition = (struct Condition*)calloc(1, sizeof(struct Condition));

	ctx_key("PLUGGED");
	seq = stable_get(table, "PLUGGED");
	if (seq && !(condition->plugged = seq_to_name_desc_list(seq)))
		goto err;

	ctx_key("UNPLUGGED");
	seq = stable_get(table, "UNPLUGGED");
	if (seq && !(condition->unplugged = seq_to_name_desc_list(seq)))
		goto err;

	goto end;

err:
	condition_free(condition);
	condition = NULL;

end:
	ctx_key(NULL);
	stable_free(table);

	return condition;
}

// unmarshal IF into a Conditions_list
static struct SList *seq_to_conditions_list(const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	struct Condition *condition = NULL;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		if ((condition = map_to_condition(node)))
			slist_append(&list, condition);
	}

	return list;
}

// unmarshal a DISABLED
static struct Disabled *map_to_disabled(const yaml_node_t *map) {
	const struct STable *table = map_to_node_table(map);
	if (!table)
		return NULL;

	const yaml_node_t *node;

	struct Disabled *disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));

	ctx_key("NAME_DESC");
	node = stable_get(table, "NAME_DESC");
	if (!check_mandatory(node) || !(disabled->name_desc = scalar_to_string(node)) || invalid_regex(disabled->name_desc, NULL))
		goto err;

	ctx_name_desc(disabled->name_desc);

	ctx_key("IF");
	node = stable_get(table, "IF");
	if (node)
		disabled->conditions = seq_to_conditions_list(node);

	goto end;

err:
	cfg_disabled_free(disabled);
	disabled = NULL;

end:
	stable_free(table);
	ctx_key(NULL);
	ctx_name_desc(NULL);

	return disabled;
}

// unmarshal DISABLED into a Disabled list
static struct SList *seq_to_disabled_list(const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	struct Disabled *disabled = NULL;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		switch (node->type) {
			case YAML_SCALAR_NODE:
				{
					struct Disabled *disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));
					if ((disabled->name_desc = scalar_to_string(node)) && !invalid_regex(disabled->name_desc, NULL))
						slist_append(&list, disabled);
					else
						cfg_disabled_free(disabled);
					break;
				}

			case YAML_MAPPING_NODE:
				{
					if ((disabled = map_to_disabled(node)))
						slist_append(&list, disabled);
					break;
				}

			default:
				log_warn("Ignoring invalid DISABLED expected scalar or map, got %s", node_type_str(node->type));
				break;
		}
	}

	return list;
}

// unmarshal a SCALE into a UserScale
static struct UserScale *map_to_user_scale(const yaml_node_t *map) {
	const struct STable *table = map_to_node_table(map);
	if (!table)
		return NULL;

	const yaml_node_t *scalar;

	struct UserScale *user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	ctx_key("NAME_DESC");
	scalar = stable_get(table, "NAME_DESC");
	if (!check_mandatory(scalar) || !(user_scale->name_desc = scalar_to_string(scalar)) || invalid_regex(user_scale->name_desc, NULL))
		goto err;

	ctx_name_desc(user_scale->name_desc);

	ctx_key("SCALE");
	scalar = stable_get(table, "SCALE");
	if (!check_mandatory(scalar) || !scalar_to_float(&user_scale->scale, scalar))
		goto err;

	goto end;

err:
	cfg_user_scale_free(user_scale);
	user_scale = NULL;

end:
	stable_free(table);
	ctx_key(NULL);
	ctx_name_desc(NULL);

	return user_scale;
}

// unmarshal SCALE into a UserScale list
static struct SList *seq_to_user_scale_list(const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	struct UserScale *user_scale = NULL;

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		if ((user_scale = map_to_user_scale(node)))
			slist_append(&list, user_scale);
	}

	return list;
}

// unmarshal a MODE into a UserMode
static struct UserMode *map_to_user_mode(const yaml_node_t *map) {
	const struct STable *table = map_to_node_table(map);
	if (!table)
		return NULL;

	const yaml_node_t *scalar;

	struct UserMode *user_mode = cfg_user_mode_default();

	ctx_key("NAME_DESC");
	scalar = stable_get(table, "NAME_DESC");
	if (!check_mandatory(scalar) || !(user_mode->name_desc = scalar_to_string(scalar)) || invalid_regex(user_mode->name_desc, NULL))
		goto err;

	ctx_name_desc(user_mode->name_desc);

	ctx_key("WIDTH");
	scalar = stable_get(table, "WIDTH");
	if (scalar && !scalar_to_int(&user_mode->width, scalar))
		goto err;

	ctx_key("HEIGHT");
	scalar = stable_get(table, "HEIGHT");
	if (scalar && !scalar_to_int(&user_mode->height, scalar))
		goto err;

	ctx_key("HZ");
	scalar = stable_get(table, "HZ");
	if (scalar) {
		float hz = 0;
		if (!scalar_to_float(&hz, scalar))
			goto err;
		user_mode->refresh_mhz = lround(hz * 1000);
	}

	ctx_key("MAX");
	scalar = stable_get(table, "MAX");
	if (scalar && !scalar_to_boolean(&user_mode->max, scalar))
		goto err;

	goto end;

err:
	cfg_user_mode_free(user_mode);
	user_mode = NULL;

end:
	stable_free(table);
	ctx_key(NULL);
	ctx_name_desc(NULL);

	return user_mode;
}

// unmarshal MODE into a UserMode list
static struct SList *seq_to_user_mode_list(const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	struct UserMode *user_mode = NULL;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		if ((user_mode = map_to_user_mode(node)))
			slist_append(&list, user_mode);
	}

	return list;
}

// unmarshal a TRANSFORM into a UserTransform
static struct UserTransform *map_to_user_transform(const yaml_node_t *map) {
	const struct STable *table = map_to_node_table(map);
	if (!table)
		return NULL;

	struct UserTransform *user_transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

	const yaml_node_t *scalar;

	ctx_key("NAME_DESC");
	scalar = stable_get(table, "NAME_DESC");
	if (!check_mandatory(scalar) ||!(user_transform->name_desc = scalar_to_string(scalar)) || invalid_regex(user_transform->name_desc, NULL))
		goto err;

	ctx_name_desc(user_transform->name_desc);

	ctx_key("TRANSFORM");
	scalar = stable_get(table, "TRANSFORM");
	if (!check_mandatory(scalar) || !(user_transform->transform = scalar_to_enum(scalar, transform_val)))
		goto err;

	goto end;

err:
	cfg_user_transform_free(user_transform);
	user_transform = NULL;

end:
	stable_free(table);
	ctx_key(NULL);
	ctx_name_desc(NULL);

	return user_transform;
}

// unmarshal TRANSFORM into a UserTransform list
static struct SList *seq_to_transform_list(const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	struct UserTransform *user_transform = NULL;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		if ((user_transform = map_to_user_transform(node)))
			slist_append(&list, user_transform);
	}

	return list;
}

static bool map_to_cfg(struct Cfg *cfg, const yaml_node_t *map) {
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

		ctx_top((char*)key->data.scalar.value);

		switch (cfg_element_val((char*)key->data.scalar.value)) {
			case ARRANGE:
				cfg->arrange = scalar_to_enum_def(ARRANGE_DEFAULT, value, arrange_val_start, arrange_name);
				break;
			case ALIGN:
				cfg->align = scalar_to_enum_def(ALIGN_DEFAULT, value, align_val_start, align_name);
				break;
			case ORDER:
				cfg->order_name_desc = seq_to_name_desc_list(value);
				break;
			case SCALING:
				cfg->scaling  = scalar_to_enum_def(SCALING_DEFAULT, value, on_off_val, on_off_name);
				break;
			case AUTO_SCALE:
				cfg->auto_scale = scalar_to_enum_def(AUTO_SCALE_DEFAULT, value, on_off_val, on_off_name);
				break;
			case SCALE:
				cfg->user_scales = seq_to_user_scale_list(value);
				break;
			case MODE:
				cfg->user_modes = seq_to_user_mode_list(value);
				break;
			case TRANSFORM:
				cfg->user_transforms = seq_to_transform_list(value);
				break;
			case VRR_OFF:
				cfg->adaptive_sync_off_name_desc = seq_to_name_desc_list(value);
				break;
			case CHANGE_SUCCESS_CMD:
			case CALLBACK_CMD:
				scalar_to_callback_cmd(&cfg->callback_cmd, value);
				break;
			case LAPTOP_DISPLAY_PREFIX:
				cfg->laptop_display_prefix = scalar_to_string(value);
				break;
			case MAX_PREFERRED_REFRESH:
				cfg->max_preferred_refresh_name_desc = seq_to_name_desc_list(value);
				break;
			case LOG_THRESHOLD:
				cfg->log_threshold = scalar_to_enum(value, log_threshold_val);
				break;
			case DISABLED:
				cfg->disabled = seq_to_disabled_list(value);
				break;
			case AUTO_SCALE_MIN:
				scalar_to_float_def(&cfg->auto_scale_min, AUTO_SCALE_MIN_DEFAULT, value);
				break;
			case AUTO_SCALE_MAX:
				scalar_to_float_def(&cfg->auto_scale_max, AUTO_SCALE_MAX_DEFAULT, value);
				break;
			default:
				// ignore unexpected
				break;
		}
	}

	return true;
}

bool yaml_file_into_cfg(struct Cfg *cfg) {
	if (!cfg->file_path) {
		return false;
	}

	FILE *input = fopen(cfg->file_path, "rb");
	if (!input) {
		log_error("\nparsing file %s: inexistent", cfg->file_path);
		return false;
	}

	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser)) {
		log_error("\nparsing file %s: yaml_parser_initialize failed", cfg->file_path);
		fclose(input);
		return false;
	}

	yaml_parser_set_input_file(&parser, input);

	yaml_document_t document;
	ctx.document = &document;

	if (!yaml_parser_load(&parser, &document)) {
		log_error("\nparsing file %s: yaml_parser_load failed", cfg->file_path);
		yaml_parser_delete(&parser);
		fclose(input);
		return false;
	}

	bool ok = true;

	const yaml_node_t *root;

	if (!(root = yaml_document_get_root_node(ctx.document))) {
		log_error("\nparsing file %s no root node", cfg->file_path);
		ok = false;
		goto end;
	}

	if (root->type != YAML_MAPPING_NODE) {
		log_error("\nparsing file %s empty cfg, expected map", cfg->file_path);
		ok = false;
		goto end;
	}

	ctx_reset();
	ok = map_to_cfg(cfg, root);

end:
	ctx_reset();
	ctx.document = NULL;

	yaml_document_delete(&document);

	yaml_parser_delete(&parser);
	fclose(input);

	return ok;
}

static void *root_to_ipc_request(const yaml_node_t *root) {

	// log exceptions and fail for required fields
	ctx.t = ERROR;

	const struct STable *table = map_to_node_table(root);
	if (!table)
		return NULL;

	struct IpcRequest *ipc_request = (struct IpcRequest*)calloc(1, sizeof(struct IpcRequest));
	ipc_request->cfg = cfg_init();

	ctx_top("OP");
	const yaml_node_t *op = stable_get(table, "OP");
	if (!check_mandatory(op) || !(ipc_request->command = scalar_to_enum(op, ipc_command_val)))
		goto err;

	// log warnings for remainder
	ctx.t = WARNING;
	ctx_action(NULL);

	ctx_top("LOG_THRESHOLD");
	ipc_request->log_threshold = scalar_to_enum(stable_get(table, "LOG_THRESHOLD"), log_threshold_val);

	ctx_top("CFG");
	map_to_cfg(ipc_request->cfg, stable_get(table, "CFG"));

	goto end;

err:
	ipc_request_free(ipc_request);
	ipc_request = NULL;

end:
	stable_free(table);

	return ipc_request;
}

static struct Lid *map_to_lid(const yaml_node_t *map) {
	const struct STable *table = map_to_node_table(map);
	if (!table)
		return NULL;

	struct Lid *lid = (struct Lid*)calloc(1, sizeof(struct Lid));

	lid->device_path = scalar_to_string(stable_get(table, "DEVICE_PATH"));
	scalar_to_boolean(&lid->closed, stable_get(table, "CLOSED"));

	stable_free(table);

	return lid;
}

static struct Mode *map_to_mode(const yaml_node_t *map) {
	const struct STable *table = map_to_node_table(map);
	if (!table)
		return NULL;

	struct Mode *mode = (struct Mode*)calloc(1, sizeof(struct Mode));

	scalar_to_int(&mode->width, stable_get(table, "WIDTH"));
	scalar_to_int(&mode->height, stable_get(table, "HEIGHT"));
	scalar_to_int(&mode->refresh_mhz, stable_get(table, "REFRESH_MHZ"));
	scalar_to_boolean(&mode->preferred, stable_get(table, "PREFERRED"));

	stable_free(table);

	return mode;
}

// unmarshal MODES into a Mode list
static struct SList *seq_to_mode_list(const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	struct Mode *mode = NULL;

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		if ((mode = map_to_mode(node)))
			slist_append(&list, mode);
	}

	return list;
}

static void map_to_head_state(struct HeadState *head_state, const yaml_node_t *map) {
	const struct STable *table = map_to_node_table(map);
	if (!table)
		return;

	scalar_to_boolean(&head_state->enabled, stable_get(table, "ENABLED"));

	scalar_to_int(&head_state->x, stable_get(table, "X"));
	scalar_to_int(&head_state->y, stable_get(table, "Y"));

	head_state->transform = scalar_to_enum(stable_get(table, "TRANSFORM"), transform_val);

	float scale;
	if (scalar_to_float(&scale, stable_get(table, "SCALE")))
		head_state->scale = wl_fixed_from_double(scale);

	bool vrr = false;
	if (scalar_to_boolean(&vrr, stable_get(table, "VRR")))
		head_state->adaptive_sync = vrr ? ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED : ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	head_state->mode = map_to_mode(stable_get(table, "MODE"));

	stable_free(table);
}

static struct Head *map_to_head(const yaml_node_t *map) {
	const struct STable *table = map_to_node_table(map);
	if (!table)
		return NULL;

	struct Head *head = (struct Head*)calloc(1, sizeof(struct Head));

	head->name = scalar_to_string(stable_get(table, "NAME"));
	head->description = scalar_to_string(stable_get(table, "DESCRIPTION"));
	head->make = scalar_to_string(stable_get(table, "MAKE"));
	head->model = scalar_to_string(stable_get(table, "MODEL"));
	head->serial_number = scalar_to_string(stable_get(table, "SERIAL_NUMBER"));
	scalar_to_int(&head->width_mm, stable_get(table, "WIDTH_MM"));
	scalar_to_int(&head->height_mm, stable_get(table, "HEIGHT_MM"));

	map_to_head_state(&head->current, stable_get(table,"CURRENT"));
	map_to_head_state(&head->desired, stable_get(table,"DESIRED"));

	head->modes = seq_to_mode_list(stable_get(table, "MODES"));

	const struct STable *table_overrides = map_to_node_table(stable_get(table, "OVERRIDES"));
	if (table_overrides) {
		bool disabled;
		if (scalar_to_boolean(&disabled, stable_get(table_overrides, "DISABLED"))) {
			head->overrided_enabled = disabled ? OverrideFalse : OverrideTrue;
		}
	}

	stable_free(table);
	stable_free(table_overrides);

	return head;
}

// unmarshal HEADS into a Head list
static struct SList *seq_to_head_list(const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	struct Head *head = NULL;

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		if ((head = map_to_head(node)))
			slist_append(&list, head);
	}

	return list;
}

// populates lid and heads
static bool map_into_state(struct IpcResponse *ipc_response, const yaml_node_t *map) {
	if (!ipc_response)
		return false;

	const struct STable *table = map_to_node_table(map);
	if (!table)
		return false;

	ipc_response->lid =	map_to_lid(stable_get(table, "LID"));

	ipc_response->heads = seq_to_head_list(stable_get(table, "HEADS"));

	stable_free(table);

	return true;
}

static struct SList *seq_to_log_cap_lines(const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return NULL;

	struct SList *list = NULL;

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		const struct STable *table_line = map_to_node_table(node);
		if (!table_line)
			return NULL;

		// unmarshal many pairs even though schema specifies exactly one
		for (const struct STableIter *i = stable_iter(table_line); i; i = stable_iter_next(i)) {

			enum LogThreshold threshold = log_threshold_val(stable_iter_key(i));
			char *line = scalar_to_string(stable_iter_val(i));

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

static struct IpcResponse *map_to_ipc_response(const yaml_node_t *map) {

	// log exceptions and fail for required fields
	ctx.t = ERROR;

	const struct STable *table = map_to_node_table(map);
	if (!table)
		return NULL;

	struct IpcResponse *ipc_response = (struct IpcResponse*)calloc(1, sizeof(struct IpcResponse));

	ctx_top("DONE");
	const yaml_node_t *done = stable_get(table, "DONE");
	if (!check_mandatory(done) || !scalar_to_boolean(&ipc_response->status.done, done))
		goto err;

	ctx_top("RC");
	const yaml_node_t *rc = stable_get(table, "RC");
	if (!check_mandatory(rc) || !scalar_to_int(&ipc_response->status.rc, rc))
		goto err;

	// suppress validation failures for remainder
	ctx.t = 0;

	ctx_top("CFG");
	const yaml_node_t *cfg = stable_get(table, "CFG");
	if (cfg) {
		ipc_response->cfg = cfg_init();
		map_to_cfg(ipc_response->cfg, cfg);
	}

	ctx_top("STATE");
	const yaml_node_t *state = stable_get(table, "STATE");
	if (state) {
		map_into_state(ipc_response, state);
	}

	ctx_top("MESSAGES");
	const yaml_node_t *messages = stable_get(table, "MESSAGES");
	if (messages) {
		ipc_response->log_cap_lines = seq_to_log_cap_lines(messages);
	}

	goto end;

err:
	ipc_response_free(ipc_response);
	ipc_response = NULL;

end:
	stable_free(table);

	return ipc_response;
}

static void *root_to_ipc_response_list(const yaml_node_t *root) {

	if (!root)
		return NULL;

	struct SList *ipc_responses = NULL;

	if (root->type != YAML_MAPPING_NODE && root->type != YAML_SEQUENCE_NODE) {
		log_error("\n%s: expected %s or %s, got %s", ctx.action, node_type_str(YAML_MAPPING_NODE), node_type_str(YAML_SEQUENCE_NODE), node_type_str(root->type));
		goto err;
	}

	if (root->type == YAML_SEQUENCE_NODE) {
		for (const yaml_node_item_t *item = root->data.sequence.items.start; item < root->data.sequence.items.top; item ++) {
			const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
			if (!node)
				continue;

			struct IpcResponse *ipc_response = map_to_ipc_response(node);
			if (ipc_response)
				slist_append(&ipc_responses, ipc_response);
		}
	} else {
		struct IpcResponse *ipc_response = map_to_ipc_response(root);
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

// marshal a yaml string to data via fn, logs use action
typedef void *(*root_to_struct_fn)(const yaml_node_t *root);
static void *yaml_to_struct(const char *yaml, root_to_struct_fn fn, char *action) {
	if (!yaml || !action)
		return NULL;

	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser)) {
		log_error("\n%s: yaml_parser_initialize failed", action);
		return NULL;
	}

	yaml_parser_set_input_string(&parser, (yaml_char_t*)yaml, strlen(yaml));

	yaml_document_t document;

	if (!yaml_parser_load(&parser, &document)) {
		log_error("\n%s: yaml_parser_load failed", action);
		yaml_parser_delete(&parser);
		log_error("========================================\n%s\n----------------------------------------", yaml);
		return NULL;
	}

	const yaml_node_t *root;

	void *out = NULL;

	if (!(root = yaml_document_get_root_node(&document))) {
		log_error("\n%s: empty request", action);
		goto err;
	}

	ctx_reset();
	ctx.document = &document;
	ctx_action(action);

	if ((out = fn(root)))
		goto end;

err:
	log_error("========================================\n%s\n----------------------------------------", yaml);

end:
	ctx_reset();
	ctx.document = NULL;

	yaml_document_delete(&document);

	yaml_parser_delete(&parser);

	return out;
}

struct IpcRequest *yaml_to_ipc_request(char *yaml) {
	return yaml_to_struct(yaml, root_to_ipc_request, "unmarshalling ipc request");
}

struct SList *yaml_to_ipc_responses(const char *yaml) {
	return yaml_to_struct(yaml, root_to_ipc_response_list, "unmarshalling ipc response");
}

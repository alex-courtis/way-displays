#include <math.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <yaml.h>

#include "cfg.h"
#include "convert.h"
#include "conditions.h"
#include "lid.h"
#include "log.h"
#include "slist.h"
#include "stable.h"

// static for convenience, must be deleted after use to free resources
static yaml_document_t document;
static yaml_parser_t parser;

struct UnmarshalCtx {
	bool silent;
	enum LogThreshold t;

	char action[128];
	char top_level_key[128];
	char name_desc[128];
	char key[128];
	char def[1024];
} ctx = { 0 };

static void ctx_reset(void) {
	memset(&ctx, 0, sizeof(struct UnmarshalCtx));
	ctx.silent = false;
	ctx.t = WARNING;
}

static void ctx_action(const char *action) {
	snprintf(ctx.action, 128, "%s", action ? action : "");
}

static void ctx_top_level_key(const char *key) {
	snprintf(ctx.top_level_key, 128, "%s", key);
}

static void ctx_name_desc(const char *name_desc) {
	snprintf(ctx.name_desc, 128, "%s", name_desc ? name_desc : "");
}

static void ctx_key(const char *key) {
	snprintf(ctx.key, 128, "%s", key ? key : "");
}

static void ctx_def(const char *def) {
	snprintf(ctx.def, 1024, "%s", def ? def : "");
}

// return a static string for the node type
static char *node_type_str(const yaml_node_type_t type) {
	switch (type) {
		case YAML_NO_NODE:
			return "empty";
		case YAML_MAPPING_NODE:
			return "map";
		case YAML_SEQUENCE_NODE:
			return "sequence";
		case YAML_SCALAR_NODE:
			return "scalar";
		default:
			return "???";
	}
}

static void log_invalid(const yaml_char_t *value, const yaml_node_type_t type_expected, const yaml_node_type_t type_actual) {
	if (ctx.silent)
		return;

	char *b = NULL;

	if (ctx.action[0])
		b = str_app(b, "\n%s:", ctx.action);
	else
		b = str_app(b, "Ignoring");

	if (ctx.top_level_key[0])
		b = str_app(b, " invalid %s", ctx.top_level_key);
	if (ctx.name_desc[0])
		b = str_app(b, " %s", ctx.name_desc);
	if (ctx.key[0])
		b = str_app(b, " %s", ctx.key);
	if (type_expected)
		b = str_app(b, " expected %s, got %s", node_type_str(type_expected), node_type_str(type_actual));
	if (value)
		b = str_app(b, " %s", value);
	if (ctx.def[0])
		b = str_app(b, ", using default %s", ctx.def);

	if (b) {
		log_(ctx.t, "%s", b);
		free(b);
	}
}

static void log_misssing(void) {
	if (ctx.silent)
		return;

	char *b = NULL;

	if (ctx.action[0])
		b = str_app(b, "\n%s: missing %s", ctx.action, ctx.top_level_key);
	else
		b = str_app(b, "%s: Ignoring missing", ctx.top_level_key);

	if (ctx.key[0])
		b = str_app(b, " %s", ctx.key);

	if (ctx.name_desc[0])
		b = str_app(b, " for '%s'", ctx.name_desc);

	if (b) {
		log_(ctx.t, "%s", b);
		free(b);
	}
}

static void log_invalid_value(const yaml_char_t *value) {
	log_invalid(value, YAML_NO_NODE, YAML_NO_NODE);
}

static void log_invalid_type(const yaml_node_type_t expected, const yaml_node_t *node) {
	log_invalid(NULL, expected, node ? node->type : YAML_NO_NODE);
}

// fn_equals to valdate a regex pattern by attempting to compile it
static bool invalid_regex(const void *pattern, const void *unused) {
	bool rc = false;
	char *p = (char*)pattern;

	if (p && p[0] == '!') {
		regex_t regex;
		int result = regcomp(&regex, p + 1, REG_EXTENDED);
		if (result) {
			char err[1024];
			regerror(result, &regex, err, 1024);
			log_warn("Ignoring invalid %s regex '%s':  %s", ctx.top_level_key, p + 1, err);
			rc = true;
		}
		regfree(&regex);
	}
	return rc;
}

// validate expected is of type actual, returning false and logging a warning if not
static bool check_node_type(const yaml_node_t *node, const yaml_node_type_t expected) {
	if (node && node->type == expected)
		return true;

	log_invalid_type(expected, node);

	return false;
}

// check that node is not null, logging a contextual warning
static bool check_mandatory(const yaml_node_t *node) {
	if (node)
		return true;

	log_misssing();

	return false;
}

// unmarshal a scalar to a strdup string
static char *scalar_to_string(const yaml_node_t *scalar) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return NULL;

	return(strdup((char*)scalar->data.scalar.value));
}

// unmarshal a scalar int to dst
static bool scalar_to_int(int32_t *dst, const yaml_node_t *scalar) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%d", dst) == 1)
		return true;

	log_invalid_value(scalar->data.scalar.value);
	return false;
}

// unmarshal a scalar float to dst
static bool scalar_to_float(float *dst, const yaml_node_t *scalar) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%f", dst) == 1)
		return true;

	log_invalid_value(scalar->data.scalar.value);
	return false;
}

// unmarshal a scalar float to dst, sets def on failure
static bool scalar_to_float_def(float *dst, float def, const yaml_node_t *scalar) {
	bool ok = true;

	char def_str[10];
	snprintf(def_str, 10, "%.1f", def);

	ctx_def(def_str);

	if (!(ok = scalar_to_float(dst, scalar)))
		*dst = def;

	ctx_def(NULL);

	return ok;
}

// unmarshal an scalar enum
typedef unsigned int (*scalar_to_enum_fn_val)(const char *name);
typedef const char* (*scalar_to_enum_fn_name)(unsigned int val);
static int scalar_to_enum(const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return 0;

	int val = fn_val((char*)scalar->data.scalar.value);
	if (val)
		return val;

	log_invalid_value(scalar->data.scalar.value);
	return 0;
}

// unmarshal an scalar enum, returns def on failure
static int scalar_to_enum_def(const int def, const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val, scalar_to_enum_fn_name fn_name) {
	ctx_def(fn_name(def));

	int ret = scalar_to_enum(scalar, fn_val);
	if (!ret)
		ret = def;

	ctx_def(NULL);

	return ret;
}

// unmarshal a scalar bool to dst
static bool scalar_to_boolean(bool *dst, const yaml_node_t *scalar) {
	int val = scalar_to_enum(scalar, on_off_val);

	if (val) {
		*dst = val == ON;
		return true;
	}

	return false;
}

// unmarshal a map of nodes
static const struct STable *map_to_node_table(const yaml_node_t *map) {
	if (!check_node_type(map, YAML_MAPPING_NODE))
		return NULL;

	const struct STable *table = stable_init(10, 10, false);

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *pair_key = yaml_document_get_node(&document, pair->key);

		char *key = NULL;
		if (!(key = scalar_to_string(pair_key))) {
			stable_free(table);
			return NULL;
		}

		const yaml_node_t *pair_value = yaml_document_get_node(&document, pair->value);

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
		const yaml_node_t *scalar = yaml_document_get_node(&document, *item);
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

	if (!check_node_type(scalar, YAML_SCALAR_NODE)) {
		*dst = strdup(CALLBACK_CMD_DEFAULT);
	} else if (strlen((char*)scalar->data.scalar.value) == 0) {
		*dst = NULL;
	} else {
		*dst = strdup((char*)scalar->data.scalar.value);
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
	seq = stable_get(table, ctx.key);
	if (seq && !(condition->plugged = seq_to_name_desc_list(seq)))
		goto err;

	ctx_key("UNPLUGGED");
	seq = stable_get(table, ctx.key);
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

		const yaml_node_t *node = yaml_document_get_node(&document, *item);
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
	node = stable_get(table, ctx.key);
	if (!check_mandatory(node))
		goto err;
	if (!(disabled->name_desc = scalar_to_string(node)))
		goto err;
	if (invalid_regex(disabled->name_desc, NULL))
		goto err;

	ctx_name_desc(disabled->name_desc);

	ctx_key("IF");
	node = stable_get(table, ctx.key);
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

		const yaml_node_t *node = yaml_document_get_node(&document, *item);
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
	scalar = stable_get(table, ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!(user_scale->name_desc = scalar_to_string(scalar)))
		goto err;
	if (invalid_regex(user_scale->name_desc, NULL))
		goto err;

	ctx_name_desc(user_scale->name_desc);

	ctx_key("SCALE");
	scalar = stable_get(table, ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!scalar_to_float(&user_scale->scale, scalar))
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

		const yaml_node_t *node = yaml_document_get_node(&document, *item);
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
	scalar = stable_get(table, ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!(user_mode->name_desc = scalar_to_string(scalar)))
		goto err;
	if (invalid_regex(user_mode->name_desc, NULL))
		goto err;

	ctx_name_desc(user_mode->name_desc);

	ctx_key("WIDTH");
	scalar = stable_get(table, ctx.key);
	if (scalar && !scalar_to_int(&user_mode->width, scalar))
		goto err;

	ctx_key("HEIGHT");
	scalar = stable_get(table, ctx.key);
	if (scalar && !scalar_to_int(&user_mode->height, scalar))
		goto err;

	ctx_key("HZ");
	scalar = stable_get(table, ctx.key);
	if (scalar) {
		float hz = 0;
		if (!scalar_to_float(&hz, scalar))
			goto err;
		user_mode->refresh_mhz = lround(hz * 1000);
	}

	ctx_key("MAX");
	scalar = stable_get(table, ctx.key);
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

		const yaml_node_t *node = yaml_document_get_node(&document, *item);
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
	scalar = stable_get(table, ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!(user_transform->name_desc = scalar_to_string(scalar)))
		goto err;
	if (invalid_regex(user_transform->name_desc, NULL))
		goto err;

	ctx_name_desc(user_transform->name_desc);

	ctx_key("TRANSFORM");
	scalar = stable_get(table, ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!(user_transform->transform = scalar_to_enum(scalar, transform_val)))
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

		const yaml_node_t *node = yaml_document_get_node(&document, *item);
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

		const yaml_node_t *key = yaml_document_get_node(&document, pair->key);
		if (!key || key->type != YAML_SCALAR_NODE || !key->data.scalar.value)
			continue;

		const yaml_node_t *value = yaml_document_get_node(&document, pair->value);
		if (!value)
			continue;

		ctx_top_level_key((char*)key->data.scalar.value);

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

	if (!yaml_parser_initialize(&parser)) {
		log_error("\nparsing file %s: yaml_parser_initialize failed", cfg->file_path);
		fclose(input);
		return false;
	}

	yaml_parser_set_input_file(&parser, input);

	if (!yaml_parser_load(&parser, &document)) {
		log_error("\nparsing file %s: yaml_parser_load failed", cfg->file_path);
		yaml_parser_delete(&parser);
		fclose(input);
		return false;
	}

	bool ok = true;

	const yaml_node_t *root;

	if (!(root = yaml_document_get_root_node(&document))) {
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

	yaml_document_delete(&document);
	yaml_parser_delete(&parser);
	fclose(input);

	return ok;
}

static void *root_to_ipc_request(const yaml_node_t *root) {
	ctx.t = ERROR;

	const struct STable *table = map_to_node_table(root);
	if (!table)
		return NULL;

	struct IpcRequest *ipc_request = (struct IpcRequest*)calloc(1, sizeof(struct IpcRequest));
	ipc_request->cfg = cfg_init();

	ctx_top_level_key("OP");
	const yaml_node_t *op = stable_get(table, ctx.top_level_key);
	if (!check_mandatory(op))
		goto err;
	if (!(ipc_request->command = scalar_to_enum(op, ipc_command_val)))
		goto err;

	ctx.silent = true;

	ctx_top_level_key("STATE");
	ipc_request->log_threshold = scalar_to_enum(stable_get(table, "LOG_THRESHOLD"), log_threshold_val);
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

// populates lid and heads
static bool map_into_state(struct IpcResponse *ipc_response, const yaml_node_t *map) {
	if (!ipc_response)
		return false;

	const struct STable *table = map_to_node_table(map);

	ipc_response->lid =	map_to_lid(stable_get(table, "LID"));

	stable_free(table);

	return true;
}

static struct IpcResponse *map_to_ipc_response(const yaml_node_t *map) {
	const struct STable *table = map_to_node_table(map);
	if (!table)
		return NULL;

	struct IpcResponse *ipc_response = (struct IpcResponse*)calloc(1, sizeof(struct IpcResponse));

	const yaml_node_t *done = stable_get(table, "DONE");
	ctx_top_level_key("DONE");
	if (!check_mandatory(done))
		goto err;
	if (!scalar_to_boolean(&ipc_response->status.done, done))
		goto err;

	const yaml_node_t *rc = stable_get(table, "RC");
	ctx_top_level_key("RC");
	if (!check_mandatory(rc))
		goto err;
	if (!scalar_to_int(&ipc_response->status.rc, rc))
		goto err;

	ctx.silent = true;

	const yaml_node_t *cfg = stable_get(table, "CFG");
	ctx_top_level_key("CFG");
	if (cfg) {
		ipc_response->cfg = cfg_init();
		map_to_cfg(ipc_response->cfg, cfg);
	}

	const yaml_node_t *state = stable_get(table, "STATE");
	if (state) {
		ctx_top_level_key("STATE");
		map_into_state(ipc_response, state);
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
	ctx.t = ERROR;

	if (!root)
		return NULL;

	struct SList *ipc_responses = NULL;

	if (root->type != YAML_MAPPING_NODE && root->type != YAML_SEQUENCE_NODE) {
		log_error("\nunmarshalling ipc response: expected %s or %s, got %s", node_type_str(YAML_MAPPING_NODE), node_type_str(YAML_SEQUENCE_NODE), node_type_str(root->type));
		goto err;
	}

	if (root->type == YAML_SEQUENCE_NODE) {
		for (const yaml_node_item_t *item = root->data.sequence.items.start; item < root->data.sequence.items.top; item ++) {
			const yaml_node_t *node = yaml_document_get_node(&document, *item);
			if (!node)
				continue;

			struct IpcResponse *ipc_response = map_to_ipc_response(node);
			if (ipc_response)
				slist_append(&ipc_responses, ipc_response);
		}
	} else {
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

	if (!yaml_parser_initialize(&parser)) {
		log_error("\n%s: yaml_parser_initialize failed", action);
		return NULL;
	}

	yaml_parser_set_input_string(&parser, (yaml_char_t*)yaml, strlen(yaml));

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
	ctx_action(action);

	if ((out = fn(root)))
		goto end;

err:
	log_error("========================================\n%s\n----------------------------------------", yaml);

end:
	ctx_reset();

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

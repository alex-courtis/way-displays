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
#include "log.h"
#include "slist.h"
#include "stable.h"

// static for convenience, must be deleted after use to free resources
static yaml_document_t document;
static yaml_parser_t parser;

struct UnmarshalCtx {
	yaml_node_type_t type_expected;
	yaml_node_type_t type_actual;
	char top_level_key[128];
	char name_desc[128];
	char key[128];
	char def[1024];
	bool silent;
} unmarshal_ctx = { 0 };

static void unmarshal_ctx_clear(void) {
	unmarshal_ctx.type_expected = YAML_NO_NODE;
	unmarshal_ctx.type_actual = YAML_NO_NODE;
	unmarshal_ctx.silent = false;
	memset(&unmarshal_ctx, 0, sizeof(struct UnmarshalCtx));
}

static void unmarshal_ctx_clear_name_desc_key(void) {
	unmarshal_ctx.name_desc[0] = '\0';
	unmarshal_ctx.key[0] = '\0';
}

static void unmarshal_ctx_top_level_key(const char *key) {
	snprintf(unmarshal_ctx.top_level_key, 128, "%s", key);
}

static void unmarshal_ctx_name_desc(const char *name_desc) {
	snprintf(unmarshal_ctx.name_desc, 128, "%s", name_desc);
}

static void unmarshal_ctx_key(const char *key) {
	snprintf(unmarshal_ctx.key, 128, "%s", key);
}

static void unmarshal_ctx_def(const char *def) {
	snprintf(unmarshal_ctx.def, 1024, "%s", def);
}

// return a static string for the node type
static char* node_type_str(const yaml_node_type_t type) {
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

// TODO specific log_invalid_value for each unmarshaller
static void log_invalid_value(const yaml_char_t *value) {
	if (unmarshal_ctx.silent)
		return;

	static char buf[1024];
	char *bufp = buf;

	bufp += snprintf(bufp, 1024 - (bufp - buf), "Ignoring invalid");

	if (unmarshal_ctx.top_level_key[0])
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", unmarshal_ctx.top_level_key);
	if (unmarshal_ctx.name_desc[0])
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", unmarshal_ctx.name_desc);
	if (unmarshal_ctx.key[0])
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", unmarshal_ctx.key);
	if (unmarshal_ctx.type_expected)
		bufp += snprintf(bufp, 1024 - (bufp - buf), " expected %s", node_type_str(unmarshal_ctx.type_expected));
	if (unmarshal_ctx.type_expected)
		bufp += snprintf(bufp, 1024 - (bufp - buf), ", got %s", node_type_str(unmarshal_ctx.type_actual));
	if (value)
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", value);
	if (unmarshal_ctx.def[0])
		bufp += snprintf(bufp, 1024 - (bufp - buf), ", using default %s", unmarshal_ctx.def);

	log_warn("%s", buf);
}

static void log_misssing(void) {
	if (unmarshal_ctx.silent)
		return;

	static char buf[1024];
	char *bufp = buf;

	bufp += snprintf(bufp, 1024 - (bufp - buf), "%s: Ignoring missing", unmarshal_ctx.top_level_key);

	if (unmarshal_ctx.key[0])
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", unmarshal_ctx.key);
	if (unmarshal_ctx.name_desc[0])
		bufp += snprintf(bufp, 1024 - (bufp - buf), " for '%s'", unmarshal_ctx.name_desc);
	if (unmarshal_ctx.def[0])
		bufp += snprintf(bufp, 1024 - (bufp - buf), ", using default %s", unmarshal_ctx.def);

	log_warn("%s", buf);
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
			log_warn("Ignoring invalid %s regex '%s':  %s", unmarshal_ctx.top_level_key, p + 1, err);
			rc = true;
		}
		regfree(&regex);
	}
	return rc;
}

// validate expected is of type actual, returning false and logging a warning if not
static bool check_node_type(const yaml_node_t *node, const yaml_node_type_t expected) {
	if (node->type == expected)
		return true;

	unmarshal_ctx.type_expected = expected;
	unmarshal_ctx.type_actual = node->type;

	log_invalid_value(NULL);

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
	char def_str[10];
	snprintf(def_str, 10, "%.1f", def);

	unmarshal_ctx_def(def_str);

	if (scalar_to_float(dst, scalar))
		return true;

	*dst = def;
	return false;
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
	unmarshal_ctx_def(fn_name(def));

	int val = scalar_to_enum(scalar, fn_val);
	if (val)
		return val;
	else
		return def;
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
	if (*dst)
		free(*dst);

	unmarshal_ctx_def(CALLBACK_CMD_DEFAULT);

	if (!check_node_type(scalar, YAML_SCALAR_NODE)) {
		*dst = strdup(unmarshal_ctx.def);
		return;
	}

	if (strlen((char*)scalar->data.scalar.value) == 0) {
		*dst = NULL;
	} else {
		*dst = strdup((char*)scalar->data.scalar.value);
	}
}

// unmarshal an IF into a Condition
static struct Condition *map_to_condition(const yaml_node_t *map) {
	const struct STable *table = NULL;
	if (!(table = map_to_node_table(map)))
		return NULL;

	const yaml_node_t *seq;

	struct Condition *condition = (struct Condition*)calloc(1, sizeof(struct Condition));

	unmarshal_ctx_key("PLUGGED");
	seq = stable_get(table, unmarshal_ctx.key);
	if (seq && !(condition->plugged = seq_to_name_desc_list(seq)))
		goto err;

	unmarshal_ctx_key("UNPLUGGED");
	seq = stable_get(table, unmarshal_ctx.key);
	if (seq && !(condition->unplugged = seq_to_name_desc_list(seq)))
		goto err;

	goto end;

err:
	condition_free(condition);
	condition = NULL;

end:
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
	const struct STable *table = NULL;
	if (!(table = map_to_node_table(map)))
		return NULL;

	const yaml_node_t *node;

	struct Disabled *disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));

	unmarshal_ctx_key("NAME_DESC");
	node = stable_get(table, unmarshal_ctx.key);
	if (!check_mandatory(node))
		goto err;
	if (!(disabled->name_desc = scalar_to_string(node)))
		goto err;
	if (invalid_regex(disabled->name_desc, NULL))
		goto err;

	unmarshal_ctx_name_desc(disabled->name_desc);

	unmarshal_ctx_key("IF");
	node = stable_get(table, unmarshal_ctx.key);
	if (node)
		disabled->conditions = seq_to_conditions_list(node);

	goto end;

err:
	cfg_disabled_free(disabled);
	disabled = NULL;

end:
	stable_free(table);
	unmarshal_ctx_clear_name_desc_key();

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
	const struct STable *table = NULL;
	if (!(table = map_to_node_table(map)))
		return NULL;

	const yaml_node_t *scalar;

	struct UserScale *user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	unmarshal_ctx_key("NAME_DESC");
	scalar = stable_get(table, unmarshal_ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!(user_scale->name_desc = scalar_to_string(scalar)))
		goto err;
	if (invalid_regex(user_scale->name_desc, NULL))
		goto err;

	unmarshal_ctx_name_desc(user_scale->name_desc);

	unmarshal_ctx_key("SCALE");
	scalar = stable_get(table, unmarshal_ctx.key);
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
	unmarshal_ctx_clear_name_desc_key();

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
	const struct STable *table = NULL;
	if (!(table = map_to_node_table(map)))
		return NULL;

	const yaml_node_t *scalar;

	struct UserMode *user_mode = cfg_user_mode_default();

	unmarshal_ctx_key("NAME_DESC");
	scalar = stable_get(table, unmarshal_ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!(user_mode->name_desc = scalar_to_string(scalar)))
		goto err;
	if (invalid_regex(user_mode->name_desc, NULL))
		goto err;

	unmarshal_ctx_name_desc(user_mode->name_desc);

	unmarshal_ctx_key("WIDTH");
	scalar = stable_get(table, unmarshal_ctx.key);
	if (scalar && !scalar_to_int(&user_mode->width, scalar))
		goto err;

	unmarshal_ctx_key("HEIGHT");
	scalar = stable_get(table, unmarshal_ctx.key);
	if (scalar && !scalar_to_int(&user_mode->height, scalar))
		goto err;

	unmarshal_ctx_key("HZ");
	scalar = stable_get(table, unmarshal_ctx.key);
	if (scalar) {
		float hz = 0;
		if (!scalar_to_float(&hz, scalar))
			goto err;
		user_mode->refresh_mhz = lround(hz * 1000);
	}

	unmarshal_ctx_key("MAX");
	scalar = stable_get(table, unmarshal_ctx.key);
	if (scalar && !scalar_to_boolean(&user_mode->max, scalar))
		goto err;

	goto end;

err:
	cfg_user_mode_free(user_mode);
	user_mode = NULL;

end:
	stable_free(table);
	unmarshal_ctx_clear_name_desc_key();

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
	const struct STable *table = NULL;
	if (!(table = map_to_node_table(map)))
		return NULL;

	struct UserTransform *user_transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

	const yaml_node_t *scalar;

	unmarshal_ctx_key("NAME_DESC");
	scalar = stable_get(table, unmarshal_ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!(user_transform->name_desc = scalar_to_string(scalar)))
		goto err;
	if (invalid_regex(user_transform->name_desc, NULL))
		goto err;

	unmarshal_ctx_name_desc(user_transform->name_desc);

	unmarshal_ctx_key("TRANSFORM");
	scalar = stable_get(table, unmarshal_ctx.key);
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
	unmarshal_ctx_clear_name_desc_key();

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

		unmarshal_ctx_clear();
		unmarshal_ctx_top_level_key((char*)key->data.scalar.value);

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

	unmarshal_ctx_clear();

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

	ok = map_to_cfg(cfg, root);

end:
	unmarshal_ctx_clear();

	yaml_document_delete(&document);
	yaml_parser_delete(&parser);
	fclose(input);

	return ok;
}

static void *root_to_ipc_request(const yaml_node_t *root) {
	unmarshal_ctx_clear();
	unmarshal_ctx.silent = true;

	struct IpcRequest *ipc_request = (struct IpcRequest*)calloc(1, sizeof(struct IpcRequest));
	ipc_request->cfg = cfg_init();

	const struct STable *table = NULL;

	if (root->type != YAML_MAPPING_NODE) {
		log_error("\nunmarshalling ipc request: expected %s, got %s", node_type_str(YAML_MAPPING_NODE), node_type_str(root->type));
		goto err;
	}

	table = map_to_node_table(root);

	// validate OP
	const yaml_node_t *op = stable_get(table, "OP");
	if (!check_mandatory(op)) {
		log_error("\nunmarshalling ipc request: missing OP");
		goto err;
	}
	if (!check_node_type(op, YAML_SCALAR_NODE)) {
		log_error("\nunmarshalling ipc request: invalid OP expected %s, got %s", node_type_str(YAML_SCALAR_NODE), node_type_str(op->type));
		goto err;
	}
	if (!(ipc_request->command = scalar_to_enum(op, ipc_command_val))) {
		log_error("\nunmarshalling ipc request: invalid OP '%s'", (char*)op->data.scalar.value);
		goto err;
	}

	// silently parse remainder
	ipc_request->log_threshold = scalar_to_enum(stable_get(table, "LOG_THRESHOLD"), log_threshold_val);
	map_to_cfg(ipc_request->cfg, stable_get(table, "CFG"));

	goto end;

err:

	ipc_request_free(ipc_request);
	ipc_request = NULL;

end:
	stable_free(table);

	unmarshal_ctx_clear();

	return ipc_request;
}

static void *root_to_ipc_responses(const yaml_node_t *map) {
	unmarshal_ctx_clear();
	unmarshal_ctx.silent = true;

	struct SList *responses = NULL;

	return responses;
}

// marshal a yaml string to data via fn, logs use name
typedef void *(*root_to_struct_fn)(const yaml_node_t *root);
static void *yaml_to_struct(const char *yaml, root_to_struct_fn fn, const char *name) {
	if (!yaml) {
		return NULL;
	}

	void *out = NULL;

	if (!yaml_parser_initialize(&parser)) {
		log_error("unmarshalling %s: yaml_parser_initialize failed", name);
		return NULL;
	}

	yaml_parser_set_input_string(&parser, (yaml_char_t*)yaml, strlen(yaml));

	if (!yaml_parser_load(&parser, &document)) {
		log_error("unmarshalling %s: yaml_parser_load failed", name);
		yaml_parser_delete(&parser);
		return NULL;
	}

	const yaml_node_t *root;

	if (!(root = yaml_document_get_root_node(&document))) {
		log_error("\nunmarshalling %s: empty request", name);
		goto err;
	}

	if ((out = fn(root)))
		goto end;

err:
	log_error("========================================\n%s\n----------------------------------------", yaml);

end:
	yaml_document_delete(&document);
	yaml_parser_delete(&parser);

	return out;
}

struct IpcRequest *yaml_to_ipc_request(char *yaml) {
	return yaml_to_struct(yaml, root_to_ipc_request, "ipc request");
}

struct SList *yaml_to_ipc_responses(const char *yaml) {
	return yaml_to_struct(yaml, root_to_ipc_responses, "ipc response");
}

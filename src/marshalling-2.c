#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/param.h>
#include <yaml.h>

#include "cfg.h"
#include "convert.h"
#include "conditions.h"
#include "marshalling.h"
#include "stable.h"

// TODO
// deal with unsigned char
// clean up test alternates, gitignore etc.
// consider SSet
// pretty print errors somewhat hierarchically

struct UnmarshalContext {
	yaml_document_t *document;
	enum CfgElement element;
	yaml_node_type_t type_expected;
	yaml_node_type_t type_actual;
	char *name_desc;
	char *key;
	char *def;
} ctx = { 0 };

static void ctx_clear(void) {
	ctx.document = NULL;
	ctx.type_expected = YAML_NO_NODE;
	ctx.type_actual = YAML_NO_NODE;
	if (ctx.name_desc)
		free(ctx.name_desc);
	if (ctx.key)
		free(ctx.key);
	if (ctx.def)
		free(ctx.def);
	memset(&ctx, 0, sizeof(struct UnmarshalContext));
}

static void ctx_clear_name_desc_key(void) {
	if (ctx.name_desc) {
		free(ctx.name_desc);
		ctx.name_desc = NULL;
	}
	if (ctx.key) {
		free(ctx.key);
		ctx.key = NULL;
	}
}

void ctx_name_desc(const char *name_desc) {
	if (ctx.name_desc)
		free(ctx.name_desc);
	if (name_desc)
		ctx.name_desc = strdup(name_desc);
	else
		ctx.name_desc = NULL;
}

void ctx_key(const char *key) {
	if (ctx.key)
		free(ctx.key);
	if (key)
		ctx.key = strdup(key);
	else
		ctx.key = NULL;
}

void ctx_def(const char *def) {
	if (ctx.def)
		free(ctx.def);
	if (def)
		ctx.def = strdup(def);
	else
		ctx.def = NULL;
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

void log_invalid_value(const yaml_char_t *value) {
	static char buf[1024];
	char *bufp = buf;

	bufp += snprintf(bufp, 1024 - (bufp - buf), "Ignoring invalid");

	if (ctx.element)
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", cfg_element_name(ctx.element));
	if (ctx.name_desc)
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", ctx.name_desc);
	if (ctx.key)
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", ctx.key);
	if (ctx.type_expected)
		bufp += snprintf(bufp, 1024 - (bufp - buf), " expected %s", node_type_str(ctx.type_expected));
	if (ctx.type_expected)
		bufp += snprintf(bufp, 1024 - (bufp - buf), ", got %s", node_type_str(ctx.type_actual));
	if (value)
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", value);
	if (ctx.def)
		bufp += snprintf(bufp, 1024 - (bufp - buf), ", using default %s", ctx.def);

	log_warn("%s", buf);
}

void log_invalid(void) {
	log_invalid_value(NULL);
}

void log_misssing(void) {
	static char buf[1024];
	char *bufp = buf;

	bufp += snprintf(bufp, 1024 - (bufp - buf), "Ignoring missing");

	if (ctx.element)
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", cfg_element_name(ctx.element));
	if (ctx.name_desc)
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", ctx.name_desc);
	if (ctx.key)
		bufp += snprintf(bufp, 1024 - (bufp - buf), " %s", ctx.key);
	if (ctx.def)
		bufp += snprintf(bufp, 1024 - (bufp - buf), ", using default %s", ctx.def);

	log_warn("%s", buf);
}

// fn_equals to valdate a regex pattern by attempting to compile it
bool invalid_regex(const void *pattern, const void *unused) {
	bool rc = false;
	char *p = (char*)pattern;

	if (p && p[0] == '!') {
		regex_t regex;
		int result = regcomp(&regex, p + 1, REG_EXTENDED);
		if (result) {
			char err[1024];
			regerror(result, &regex, err, 1024);
			log_warn("Ignoring invalid %s regex '%s':  %s", cfg_element_name(ctx.element), p + 1, err);
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

	ctx.type_expected = expected;
	ctx.type_actual = node->type;

	log_invalid();

	return false;
}

// check that node is not null, logging a contextual warning
static bool check_mandatory(const yaml_node_t *node) {
	if (node)
		return true;

	log_misssing();

	return false;
}

// unmarshal a scalar string to dst
static bool scalar_to_string(char **dst, const yaml_node_t *scalar) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	*dst = strdup((char*)scalar->data.scalar.value);

	return true;
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

	ctx_def(def_str);

	if (scalar_to_float(dst, scalar))
		return true;

	*dst = def;
	return false;
}

// unmarshal an scalar enum to dst
typedef unsigned int (*scalar_to_enum_fn_val)(const char *name);
typedef const char* (*scalar_to_enum_fn_name)(unsigned int val);
static bool scalar_to_enum(int *dst, const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	int val = fn_val((char*)scalar->data.scalar.value);
	if (val) {
		*dst = val;
		return true;
	}

	log_invalid_value(scalar->data.scalar.value);
	return false;
}

// unmarshal an scalar enum to dst, sets def on failure
static bool scalar_to_enum_def(int *dst, const int def, const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val, scalar_to_enum_fn_name fn_name) {
	ctx_def(fn_name(def));

	if (scalar_to_enum(dst, scalar, fn_val)) {
		return true;
	} else {
		*dst = def;
		return false;
	}
}

// unmarshal a scalar bool to dst
static bool scalar_to_boolean(bool *dst, const yaml_node_t *scalar) {
	int val;

	if (scalar_to_enum(&val, scalar, on_off_val)) {
		*dst = val == ON;
		return true;
	}

	return false;
}

// unmarshal a map of nodes into dst
static bool map_to_node_table(const struct STable **dst, const yaml_node_t *map) {
	if (!check_node_type(map, YAML_MAPPING_NODE))
		return false;

	*dst = stable_init(10, 10, false);

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *pair_key = yaml_document_get_node(ctx.document, pair->key);

		char *key = NULL;
		if (!scalar_to_string(&key, pair_key)) {
			stable_free(*dst);
			*dst = NULL;
			return false;
		}

		const yaml_node_t *pair_value = yaml_document_get_node(ctx.document, pair->value);

		if (key && pair_value)
			stable_put(*dst, key, pair_value);

		if (key)
			free(key);
	}

	return true;
}

// unmarshal a sequence of strings into dst, removing duplicates
static bool seq_to_string_list(struct SList **dst, const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return false;

	const struct STable *table = stable_init(10, 10, false);

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *scalar = yaml_document_get_node(ctx.document, *item);
		if (!scalar)
			continue;

		char *val = NULL;
		if (scalar_to_string(&val, scalar)) {
			stable_put(table, val, NULL);
			free(val);
		}
	}

	*dst = stable_keys_slist(table);

	stable_free(table);

	return true;
}

// unmarshal a sequence of regex validated name_desc into dst, removing duplicates
static bool seq_to_name_desc(struct SList **dst, const yaml_node_t *seq) {
	if (!seq_to_string_list(dst, seq))
		return false;

	return (slist_remove_all_free(dst, invalid_regex, NULL, NULL) == 0);
}

// unmarshal a CALLBACK_CMD dst, frees first, sets NULL on empty string, otherwise default
static bool scalar_to_callback_cmd(char **dst, const yaml_node_t *scalar) {
	if (*dst)
		free(*dst);

	ctx_def(CALLBACK_CMD_DEFAULT);

	if (!check_node_type(scalar, YAML_SCALAR_NODE)) {
		*dst = strdup(ctx.def);
		return false;
	}

	if (strlen((char*)scalar->data.scalar.value) == 0) {
		// TODO explicit empty string to NULL test
		*dst = NULL;
	} else {
		*dst = strdup((char*)scalar->data.scalar.value);
	}

	return true;
}

// unmarshal a LOG_THRESHOLD to dst, sets empty on failure
static bool scalar_to_log_threshold(int *dst, const int def, const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val, scalar_to_enum_fn_name fn_name) {

	if (scalar_to_enum_def(dst, LOG_THRESHOLD_DEFAULT, scalar, log_threshold_val, log_threshold_name))
		return true;

	*dst = 0;

	return false;
}

// unmarshal an IF into a Condition
static bool map_to_condition(struct Condition **condition, const yaml_node_t *map) {
	const struct STable *table = NULL;
	if (!map_to_node_table(&table, map))
		return false;

	const yaml_node_t *seq;

	*condition = (struct Condition*)calloc(1, sizeof(struct Condition));

	ctx_key("PLUGGED");
	seq = stable_get(table, ctx.key);
	if (seq && !seq_to_name_desc(&(*condition)->plugged, seq))
		goto err;

	ctx_key("UNPLUGGED");
	seq = stable_get(table, ctx.key);
	if (seq && !seq_to_name_desc(&(*condition)->unplugged, seq))
		goto err;

	stable_free(table);

	return true;

err:
	stable_free(table);

	condition_free(*condition);
	*condition = NULL;

	return false;
}

// unmarshal IF into a Conditions_list
static bool seq_to_conditions_list(struct SList **conditions_list, const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return false;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		struct Condition *condition = NULL;

		if (map_to_condition(&condition, node))
			slist_append(conditions_list, condition);
	}

	return true;
}

// unmarshal a DISABLED into a Disabled
static bool map_to_disabled(struct Disabled **disabled, const yaml_node_t *map) {
	const struct STable *table = NULL;
	if (!map_to_node_table(&table, map))
		return false;

	const yaml_node_t *node;

	*disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));

	ctx_key("NAME_DESC");
	node = stable_get(table, ctx.key);
	if (!check_mandatory(node))
		goto err;
	if (!scalar_to_string(&(*disabled)->name_desc, node))
		goto err;
	if (invalid_regex((*disabled)->name_desc, NULL))
		goto err;

	ctx_name_desc((*disabled)->name_desc);

	ctx_key("IF");
	node = stable_get(table, ctx.key);
	if (node)
		seq_to_conditions_list(&(*disabled)->conditions, node);

	stable_free(table);
	ctx_clear_name_desc_key();

	return true;

err:
	stable_free(table);
	ctx_clear_name_desc_key();

	cfg_disabled_free(*disabled);
	*disabled = NULL;

	return false;
}

// unmarshal DISABLED into a Disabled list
static bool seq_to_disabled_list(struct SList **disableds, const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return false;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		switch (node->type) {
			case YAML_SCALAR_NODE:
				{
					struct Disabled *disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));
					if (scalar_to_string(&disabled->name_desc, node) && !invalid_regex(disabled->name_desc, NULL))
						slist_append(disableds, disabled);
					else
						cfg_disabled_free(disabled);
					break;
				}

			case YAML_MAPPING_NODE:
				{
					struct Disabled *disabled = NULL;
					if (map_to_disabled(&disabled, node))
						slist_append(disableds, disabled);
					break;
				}

			default:
				log_warn("TODO test and add context Ignoring invalid DISABLED: expected scalar or map, got %s", node_type_str(node->type));
				break;
		}
	}

	return true;
}

// unmarshal a SCALE into a UserScale
static bool map_to_user_scale(struct UserScale **user_scale, const yaml_node_t *map) {
	const struct STable *table = NULL;
	if (!map_to_node_table(&table, map))
		return false;

	const yaml_node_t *scalar;

	*user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	ctx_key("NAME_DESC");
	scalar = stable_get(table, ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!scalar_to_string(&(*user_scale)->name_desc, scalar))
		goto err;
	if (invalid_regex((*user_scale)->name_desc, NULL))
		goto err;

	ctx_name_desc((*user_scale)->name_desc);

	ctx_key("SCALE");
	scalar = stable_get(table, ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!scalar_to_float(&(*user_scale)->scale, scalar))
		goto err;

	stable_free(table);
	ctx_clear_name_desc_key();

	return true;

err:
	stable_free(table);
	ctx_clear_name_desc_key();

	cfg_user_scale_free(*user_scale);
	*user_scale = NULL;

	return false;
}

// unmarshal SCALE into a UserScale list
static bool seq_to_user_scale_list(struct SList **user_scales, const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return false;

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		struct UserScale *user_scale = NULL;

		if (map_to_user_scale(&user_scale, node))
			slist_append(user_scales, user_scale);
	}

	return true;
}

// unmarshal a MODE into a UserMode
static bool map_to_user_mode(struct UserMode **user_mode, const yaml_node_t *map) {
	const struct STable *table = NULL;
	if (!map_to_node_table(&table, map))
		return false;

	const yaml_node_t *scalar;

	*user_mode = cfg_user_mode_default();

	ctx_key("NAME_DESC");
	scalar = stable_get(table, ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!scalar_to_string(&(*user_mode)->name_desc, scalar))
		goto err;
	if (invalid_regex((*user_mode)->name_desc, NULL))
		goto err;

	ctx_name_desc((*user_mode)->name_desc);

	ctx_key("WIDTH");
	scalar = stable_get(table, ctx.key);
	if (scalar && !scalar_to_int(&(*user_mode)->width, scalar))
		goto err;

	ctx_key("HEIGHT");
	scalar = stable_get(table, ctx.key);
	if (scalar && !scalar_to_int(&(*user_mode)->height, scalar))
		goto err;

	ctx_key("HZ");
	scalar = stable_get(table, ctx.key);
	if (scalar) {
		float hz = 0;
		if (!scalar_to_float(&hz, scalar))
			goto err;
		(*user_mode)->refresh_mhz = lround(hz * 1000);
	}

	ctx_key("MAX");
	scalar = stable_get(table, ctx.key);
	if (scalar && !scalar_to_boolean(&(*user_mode)->max, scalar))
		goto err;

	stable_free(table);
	ctx_clear_name_desc_key();

	return true;

err:
	stable_free(table);
	ctx_clear_name_desc_key();

	cfg_user_mode_free(*user_mode);
	*user_mode = NULL;

	return false;
}

// unmarshal MODE into a UserMode list
static bool seq_to_user_mode_list(struct SList **user_modes, const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return false;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		struct UserMode *user_mode = NULL;

		if (map_to_user_mode(&user_mode, node))
			slist_append(user_modes, user_mode);
	}

	return true;
}

// unmarshal a TRANSFORM into a UserTransform
static bool map_to_user_transform(struct UserTransform **user_transform, const yaml_node_t *map) {
	const struct STable *table = NULL;
	if (!map_to_node_table(&table, map))
		return false;

	*user_transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

	const yaml_node_t *scalar;

	ctx_key("NAME_DESC");
	scalar = stable_get(table, ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!scalar_to_string(&(*user_transform)->name_desc, scalar))
		goto err;
	if (invalid_regex((*user_transform)->name_desc, NULL))
		goto err;

	ctx_name_desc((*user_transform)->name_desc);

	ctx_key("TRANSFORM");
	scalar = stable_get(table, ctx.key);
	if (!check_mandatory(scalar))
		goto err;
	if (!scalar_to_enum((int*)&(*user_transform)->transform, scalar, transform_val))
		goto err;

	stable_free(table);
	ctx_clear_name_desc_key();

	return true;

err:
	stable_free(table);
	ctx_clear_name_desc_key();

	cfg_user_transform_free(*user_transform);
	*user_transform = NULL;

	return false;
}

// unmarshal TRANSFORM into a UserTransform list
static bool seq_to_transform_list(struct SList **user_transforms, const yaml_node_t *seq) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return false;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(ctx.document, *item);
		if (!node)
			continue;

		struct UserTransform *user_transform = NULL;

		if (map_to_user_transform(&user_transform, node))
			slist_append(user_transforms, user_transform);
	}

	return true;
}

static bool doc_to_cfg(struct Cfg *cfg, yaml_document_t *document) {

	const yaml_node_t *start_doc = document->nodes.start;
	if (!start_doc || start_doc->type != YAML_MAPPING_NODE) {
		log_error("\nparsing file %s empty cfg, expected map", cfg->file_path);
		return false;
	}

	for (const yaml_node_pair_t *pair = start_doc->data.mapping.pairs.start; pair < start_doc->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *key = yaml_document_get_node(document, pair->key);
		if (!key || key->type != YAML_SCALAR_NODE || !key->data.scalar.value)
			continue;

		const yaml_node_t *value = yaml_document_get_node(document, pair->value);
		if (!value)
			continue;

		ctx_clear();
		ctx.document = document;
		ctx.element = cfg_element_val((char*)key->data.scalar.value);

		switch (ctx.element) {
			case ARRANGE:
				scalar_to_enum_def((int*)&cfg->arrange, ARRANGE_DEFAULT, value, arrange_val_start, arrange_name);
				break;
			case ALIGN:
				scalar_to_enum_def((int*)&cfg->align, ALIGN_DEFAULT, value, align_val_start, align_name);
				break;
			case ORDER:
				seq_to_name_desc(&cfg->order_name_desc, value);
				break;
			case SCALING:
				scalar_to_enum_def((int*)&cfg->scaling, SCALING_DEFAULT, value, on_off_val, on_off_name);
				break;
			case AUTO_SCALE:
				scalar_to_enum_def((int*)&cfg->auto_scale, AUTO_SCALE_DEFAULT, value, on_off_val, on_off_name);
				break;
			case SCALE:
				seq_to_user_scale_list(&cfg->user_scales, value);
				break;
			case MODE:
				seq_to_user_mode_list(&cfg->user_modes, value);
				break;
			case TRANSFORM:
				seq_to_transform_list(&cfg->user_transforms, value);
				break;
			case VRR_OFF:
				seq_to_name_desc(&cfg->adaptive_sync_off_name_desc, value);
				break;
			case CHANGE_SUCCESS_CMD:
			case CALLBACK_CMD:
				scalar_to_callback_cmd(&cfg->callback_cmd, value);
				break;
			case LAPTOP_DISPLAY_PREFIX:
				scalar_to_string(&cfg->laptop_display_prefix, value);
				break;
			case MAX_PREFERRED_REFRESH:
				seq_to_name_desc(&cfg->max_preferred_refresh_name_desc, value);
				break;
			case LOG_THRESHOLD:
				scalar_to_log_threshold((int*)&cfg->log_threshold, LOG_THRESHOLD_DEFAULT, value, log_threshold_val, log_threshold_name);
				break;
			case DISABLED:
				seq_to_disabled_list(&cfg->disabled, value);
				break;
			case AUTO_SCALE_MIN:
				scalar_to_float_def(&cfg->auto_scale_min, AUTO_SCALE_MIN_DEFAULT, value);
				break;
			case AUTO_SCALE_MAX:
				scalar_to_float_def(&cfg->auto_scale_max, AUTO_SCALE_MAX_DEFAULT, value);
				break;
			default:
				log_warn("\nTODO maybe: parsing file %s unexpected entry %s", cfg->file_path, (char*)key->data.scalar.value);
				break;
		}
	}

	ctx_clear();

	return true;
}

bool unmarshal_cfg_from_file_2(struct Cfg *cfg) {
	if (!cfg->file_path) {
		return false;
	}

	bool ok = true;

	FILE *input = fopen(cfg->file_path, "rb");
	if (!input) {
		log_error("\nparsing file %s missing", cfg->file_path);
		return false;
	}

	yaml_parser_t parser;
	yaml_document_t document;

	if (!(ok = yaml_parser_initialize(&parser))) {
		log_error("\nparsing file %s could not initialise parser", cfg->file_path);
		goto end;
	}

	yaml_parser_set_input_file(&parser, input);

	if (!(ok = yaml_parser_load(&parser, &document))) {
		log_error("\nparsing file %s TODO some sort of stack", cfg->file_path);
		goto end;
	}

	if (!(ok = yaml_document_get_root_node(&document))) {
		log_error("\nparsing file %s empty cfg, expected map", cfg->file_path);
		goto end;
	}

	ok = doc_to_cfg(cfg, &document);

end:
	yaml_document_delete(&document);

	yaml_parser_delete(&parser);

	if (input)
		fclose(input);

	return ok;
}


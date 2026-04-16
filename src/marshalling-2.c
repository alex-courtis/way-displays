#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/param.h>
#include <yaml.h>

#include "cfg.h"
#include "convert.h"
#include "conditions.h"
#include "stable.h"

// TODO
// deal with unsigned char
// all generic _to_ should return false and free/null dst on any error
// clean up test alternates, gitignore etc.
// consider SSet

struct UnmarshalContext {
	enum CfgElement element;
	char *name_desc;
	char *key;
} ctx = { 0 };

static void ctx_clear(void) {
	if (ctx.name_desc)
		free(ctx.name_desc);
	if (ctx.key)
		free(ctx.key);
	memset(&ctx, 0, sizeof(struct UnmarshalContext));
}

void ctx_element(const enum CfgElement element) {
	ctx.element = element;
}

void ctx_name_desc(const char *name_desc) {
	if (ctx.name_desc)
		free(ctx.name_desc);
	ctx.name_desc = strdup(name_desc);
}

void ctx_key(const char *key) {
	if (ctx.key)
		free(ctx.key);
	ctx.key = strdup(key);
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

// validate expected is of type actual, returning false and logging a warning if not
static bool check_node_type(const yaml_node_t *node, const yaml_node_type_t expected) {
	if (node->type == expected)
		return true;

	// TODO add a context string renderer

	log_warn(
			"Ignoring invalid %s %s %s expected %s, got %s",
			cfg_element_name(ctx.element),
			ctx.name_desc ? ctx.name_desc : "",
			ctx.key ? ctx.key : "",
			node_type_str(expected),
			node_type_str(node->type)
			);

	return false;
}

// unmarshal a scalar string to empty dst, freeing first
static bool scalar_to_string(char **dst, const yaml_node_t *scalar) {
	if (*dst)
		free(*dst);
	*dst = NULL;

	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	*dst = strdup((char*)scalar->data.scalar.value);

	return true;
}

// unmarshal a scalar string to empty dst, freeing first, copies def on failure
static bool scalar_to_string_def(char **dst, const char *def, const yaml_node_t *scalar) {
	if (!scalar_to_string(dst, scalar)) {
		log_warn("TODO scalar_to_string_def default message");
		*dst = strdup(def);
		return false;
	}

	return true;
}

// unmarshal a scalar number to dst
static bool scalar_to_int(int32_t *dst, const yaml_node_t *scalar) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%d", dst) != 1) {
		log_warn("Ignoring invalid %s %s %s %s", cfg_element_name(ctx.element), ctx.name_desc, ctx.key, scalar->data.scalar.value);
		return false;
	}

	return true;
}

// unmarshal a scalar number to dst
static bool scalar_to_float(float *dst, const yaml_node_t *scalar) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%f", dst) != 1) {
		log_warn("Ignoring invalid %s %s %s %s", cfg_element_name(ctx.element), ctx.name_desc, ctx.key, scalar->data.scalar.value);
		return false;
	}

	return true;
}

// unmarshal a scalar number to dst, sets def on failure
static bool scalar_to_float_def(float *dst, float def, const yaml_node_t *scalar) {
	if (!scalar_to_float(dst, scalar)) {
		log_warn("TODO scalar_to_float_def default message");
		*dst = def;
		return false;
	}

	return true;
}

// unmarshal an scalar enum to dst
typedef unsigned int (*scalar_to_enum_fn_val)(const char *name);
typedef const char* (*scalar_to_enum_fn_name)(unsigned int val);
static bool scalar_to_enum(int *dst, const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	int val = fn_val((char*)scalar->data.scalar.value);
	if (!val) {
		log_warn("Ignoring invalid %s %s %s %s", cfg_element_name(ctx.element), ctx.name_desc, ctx.key, scalar->data.scalar.value);
		return false;
	}

	*dst = val;

	return true;
}

// unmarshal an scalar enum to dst, sets def on failure
static bool scalar_to_enum_def(int *dst, const int def, const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val, scalar_to_enum_fn_name fn_name) {
	if (!scalar_to_enum(dst, scalar, fn_val)) {
		log_warn("TODO scalar_to_enum_def default message %s", fn_name(def));
		*dst = def;
		return false;
	}

	return true;
}

// unmarshal a scalar bool to dst
static bool scalar_to_boolean(bool *dst, const yaml_node_t *scalar) {
	int val;
	if (!scalar_to_enum(&val, scalar, on_off_val))
		return false;

	*dst = val == ON;

	return true;
}

// unmarshal a map of nodes into empty dst, caller frees
static bool map_to_node_table(const struct STable **dst, const yaml_node_t *map, yaml_document_t *document) {
	if (!check_node_type(map, YAML_MAPPING_NODE))
		return false;

	*dst = stable_init(10, 10, false);

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *pair_key = yaml_document_get_node(document, pair->key);

		char *key = NULL;
		if (!scalar_to_string(&key, pair_key)) {
			stable_free(*dst);
			return false;
		}

		const yaml_node_t *pair_value = yaml_document_get_node(document, pair->value);

		if (key && pair_value)
			stable_put(*dst, key, pair_value);

		if (key)
			free(key);
	}

	return true;
}

// TODO check regex
// unmarshal a sequence of strings into empty dst, removing duplicates, caller frees
static bool seq_to_string_list(struct SList **dst, const yaml_node_t *seq, yaml_document_t *document) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return false;

	const struct STable *table = stable_init(10, 10, false);

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *scalar = yaml_document_get_node(document, *item);
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

// TODO generify
// unmarshal ORDER into order_name_desc, removing invalid patterns
static bool seq_to_order(struct SList **order_name_desc, const yaml_node_t *seq, yaml_document_t *document) {
	if (!seq_to_string_list(order_name_desc, seq, document))
		return false;

	return (slist_remove_all_free(order_name_desc, cfg_invalid_order_regex, NULL, NULL) == 0);
}

// unmarshal DISABLED into a Condition
static bool map_to_condition(struct Condition **condition, const yaml_node_t *map, yaml_document_t *document) {
	const struct STable *table = NULL;
	if (!map_to_node_table(&table, map, document))
		return false;

	bool ok = true;

	const yaml_node_t *seq;

	*condition = (struct Condition*)calloc(1, sizeof(struct Condition));

	ctx_key("PLUGGED");
	if ((seq = stable_get(table, ctx.key)))
		if (!(ok = seq_to_string_list(&(*condition)->plugged, seq, document)))
			goto end;

	ctx_key("UNPLUGGED");
	if ((seq = stable_get(table, ctx.key)))
		if (!(ok = seq_to_string_list(&(*condition)->unplugged, seq, document)))
			goto end;

end:

	stable_free(table);

	if (!ok) {
		condition_free(*condition);
		*condition = NULL;
	}

	return ok;
}

// unmarshal a map into conditions_list, freeing first
static bool seq_to_conditions_list(struct SList **conditions_list, const yaml_node_t *seq, yaml_document_t *document) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return false;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *node = yaml_document_get_node(document, *item);
		if (!node)
			continue;

		struct Condition *condition = NULL;
		if (map_to_condition(&condition, node, document))
			slist_append(conditions_list, condition);
	}

	return true;
}

// unmarshal DISABLED into a Disabled
static bool map_to_disabled(struct Disabled **disabled, const yaml_node_t *map, yaml_document_t *document) {
	const struct STable *table = NULL;
	if (!map_to_node_table(&table, map, document))
		return false;

	bool ok = true;

	const yaml_node_t *node;

	*disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));

	ctx_key("NAME_DESC");
	if ((node = stable_get(table, ctx.key)))
		if (!(ok = scalar_to_string(&(*disabled)->name_desc, node)))
			goto end;
	// TODO regex

	ctx_name_desc((*disabled)->name_desc);

	ctx_key("IF");
	if ((node = stable_get(table, ctx.key)))
		seq_to_conditions_list(&(*disabled)->conditions, node, document);

end:

	stable_free(table);

	// free on validation fail
	if (!ok) {
		cfg_disabled_free(*disabled);
		*disabled = NULL;
	}

	return ok;
}

// unmarshal DISABLED into a Disabled list
static bool seq_to_disabled_list(struct SList **disableds, const yaml_node_t *seq, yaml_document_t *document) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return false;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *node = yaml_document_get_node(document, *item);

		if (!node)
			continue;

		switch (node->type) {
			case YAML_SCALAR_NODE:
				{
					struct Disabled *disabled = (struct Disabled*)calloc(1, sizeof(struct Disabled));
					if (scalar_to_string(&disabled->name_desc, node))
						slist_append(disableds, disabled);
					else
						cfg_disabled_free(disabled);
					break;
				}

			case YAML_MAPPING_NODE:
				{
					struct Disabled *disabled = NULL;
					if (map_to_disabled(&disabled, node, document))
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
static bool map_to_user_scale(struct UserScale **user_scale, const yaml_node_t *map, yaml_document_t *document) {
	const struct STable *table = NULL;
	if (!map_to_node_table(&table, map, document))
		return false;

	bool ok = true;

	*user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	const yaml_node_t *scalar;

	ctx_key("NAME_DESC");
	if ((scalar = stable_get(table, ctx.key)))
		if (!(ok = scalar_to_string(&(*user_scale)->name_desc, scalar)))
			goto end;

	ctx_name_desc((*user_scale)->name_desc);

	ctx_key("SCALE");
	if ((ok = (scalar = stable_get(table, ctx.key))))
		if (!(ok = scalar_to_float(&(*user_scale)->scale, scalar)))
			goto end;

end:

	stable_free(table);

	// free on validation fail
	if (!ok) {
		cfg_user_scale_free(*user_scale);
		*user_scale = NULL;
	}

	return ok;
}

// unmarshal SCALE into a UserScale list
static bool seq_to_user_scale_list(struct SList **user_scales, const yaml_node_t *seq, yaml_document_t *document) {
	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *node = yaml_document_get_node(document, *item);
		if (!node)
			continue;

		struct UserScale *user_scale = NULL;

		if (map_to_user_scale(&user_scale, node, document))
			slist_append(user_scales, user_scale);
	}

	return true;
}

// unmarshal a MODE into a UserMode
static bool map_to_user_mode(struct UserMode **user_mode, const yaml_node_t *map, yaml_document_t *document) {
	const struct STable *table = NULL;
	if (!map_to_node_table(&table, map, document))
		return false;

	bool ok = true;

	*user_mode = cfg_user_mode_default();

	const yaml_node_t *scalar;

	ctx_key("NAME_DESC");
	if ((scalar = stable_get(table, ctx.key)))
		if (!(ok = scalar_to_string(&(*user_mode)->name_desc, scalar)))
			goto end;
	// TODO missing and regex

	ctx_name_desc((*user_mode)->name_desc);

	ctx_key("WIDTH");
	if ((scalar = stable_get(table, ctx.key)))
		if (!(ok = scalar_to_int(&(*user_mode)->width, scalar)))
			goto end;

	ctx_key("HEIGHT");
	if ((scalar = stable_get(table, ctx.key)))
		if (!(ok = scalar_to_int(&(*user_mode)->height, scalar)))
			goto end;

	ctx_key("HZ");
	if ((scalar = stable_get(table, ctx.key))) {
		float hz;
		if (!(ok = scalar_to_float(&hz, scalar)))
			goto end;

		(*user_mode)->refresh_mhz = lround(hz * 1000);
	}

	ctx_key("MAX");
	if ((scalar = stable_get(table, ctx.key)))
		if (!(ok = scalar_to_boolean(&(*user_mode)->max, scalar)))
			goto end;

end:

	stable_free(table);

	// free on validation fail
	if (!ok) {
		cfg_user_mode_free(*user_mode);
		user_mode = NULL;
	}

	return ok;
}

// unmarshal MODE into a UserMode list
static bool seq_to_user_mode_list(struct SList **user_modes, const yaml_node_t *seq, yaml_document_t *document) {
	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return false;

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		const yaml_node_t *node = yaml_document_get_node(document, *item);
		if (!node)
			continue;

		struct UserMode *user_mode = NULL;

		if (map_to_user_mode(&user_mode, node, document))
			slist_append(user_modes, user_mode);
	}

	return true;
}

// unmarshal TRANSFORM into user_transforms
static void seq_to_transforms_list(struct SList **user_transforms, const yaml_node_t *seq, yaml_document_t *document) {
	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {

		// TODO extract function for proper error handling

		const yaml_node_t *node = yaml_document_get_node(document, *item);
		if (!node)
			continue;

		const struct STable *table = NULL;
		if (!map_to_node_table(&table, node, document))
			continue;

		struct UserTransform *transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

		const yaml_node_t *scalar;

		ctx_key("NAME_DESC");
		if ((scalar = stable_get(table, ctx.key)))
			if (!scalar_to_string(&transform->name_desc, scalar))
				goto end;

		ctx_name_desc(transform->name_desc);

		ctx_key("TRANSFORM");
		if ((scalar = stable_get(table, ctx.key)))
			if (!scalar_to_enum((int*)&transform->transform, scalar, transform_val))
				goto end;

		// OK
		slist_append(user_transforms, transform);
		transform = NULL;

end:

		stable_free(table);

		// free on validation fail
		if (transform) {
			cfg_user_transform_free(transform);
			transform = NULL;
		}
	}
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

		ctx.element = cfg_element_val((char*)key->data.scalar.value);

		switch (ctx.element) {
			case ARRANGE:
				scalar_to_enum_def((int*)&cfg->arrange, ARRANGE_DEFAULT, value, arrange_val_start, arrange_name);
				break;
			case ALIGN:
				scalar_to_enum_def((int*)&cfg->align, ALIGN_DEFAULT, value, align_val_start, align_name);
				break;
			case ORDER:
				seq_to_order(&cfg->order_name_desc, value, document);
				break;
			case SCALING:
				scalar_to_enum_def((int*)&cfg->scaling, SCALING_DEFAULT, value, on_off_val, on_off_name);
				break;
			case AUTO_SCALE:
				scalar_to_enum_def((int*)&cfg->auto_scale, AUTO_SCALE_DEFAULT, value, on_off_val, on_off_name);
				break;
			case SCALE:
				seq_to_user_scale_list(&cfg->user_scales, value, document);
				break;
			case MODE:
				seq_to_user_mode_list(&cfg->user_modes, value, document);
				break;
			case TRANSFORM:
				seq_to_transforms_list(&cfg->user_transforms, value, document);
				break;
			case VRR_OFF:
				seq_to_string_list(&cfg->adaptive_sync_off_name_desc, value, document);
				break;
			case CHANGE_SUCCESS_CMD:
			case CALLBACK_CMD:
				scalar_to_string_def(&cfg->callback_cmd, CALLBACK_CMD_DEFAULT, value);
				break;
			case LAPTOP_DISPLAY_PREFIX:
				scalar_to_string(&cfg->laptop_display_prefix, value);
				break;
			case MAX_PREFERRED_REFRESH:
				// TODO
				break;
			case LOG_THRESHOLD:
				scalar_to_enum_def((int*)&cfg->log_threshold, LOG_THRESHOLD_DEFAULT, value, log_threshold_val, log_threshold_name);
				break;
			case DISABLED:
				seq_to_disabled_list(&cfg->disabled, value, document);
				break;
			case ARRANGE_ALIGN:
				// TODO
				break;
			case AUTO_SCALE_MIN:
				scalar_to_float_def(&cfg->auto_scale_min, AUTO_SCALE_MIN_DEFAULT, value);
				break;
			case AUTO_SCALE_MAX:
				scalar_to_float_def(&cfg->auto_scale_max, AUTO_SCALE_MAX_DEFAULT, value);
				break;
			default:
				log_warn("\nparsing file %s unexpected entry %s", cfg->file_path, (char*)key->data.scalar.value);
				break;
		}

		ctx_clear();
	}

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


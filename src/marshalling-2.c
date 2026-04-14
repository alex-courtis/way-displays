#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/param.h>
#include <yaml.h>

#include "cfg.h"
#include "convert.h"
#include "stable.h"

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

// unmarshal a scalar string to dst, freeing first
static bool scalar_to_string(char **dst, const yaml_node_t *scalar) {
	if (*dst)
		free(*dst);
	*dst = NULL;

	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	*dst = strdup((char*)scalar->data.scalar.value);

	return true;
}

// unmarshal a scalar string to dst, freeing first, copies def on failure
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

// unmarshal a map of strings to nodes, caller frees
static const struct STable *map_to_node_table(const yaml_node_t *map, yaml_document_t *document) {

	if (!check_node_type(map, YAML_MAPPING_NODE))
		return NULL;

	const struct STable *dst = stable_init(10, 10, false);

	for (const yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
		if (!pair->key || !pair->value)
			continue;

		const yaml_node_t *pair_key = yaml_document_get_node(document, pair->key);
		char *key = NULL;
		scalar_to_string(&key, pair_key);

		const yaml_node_t *pair_value = yaml_document_get_node(document, pair->value);

		if (key && pair_value) {
			stable_put(dst, key, pair_value);
		}

		if (key)
			free(key);
	}

	return dst;
}

// TODO check regex
// unmarshal a sequence of strings, freeing first, removing duplicates, caller frees
static void seq_to_string_list(struct SList **dst, const yaml_node_t *seq, yaml_document_t *document) {
	if (*dst)
		slist_free_vals(dst, NULL);

	if (!check_node_type(seq, YAML_SEQUENCE_NODE))
		return;

	const struct STable *map = stable_init(10, 10, false);

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *scalar = yaml_document_get_node(document, *item);
		if (!scalar)
			continue;

		char *val = NULL;
		scalar_to_string(&val, scalar);
		if (!val)
			continue;

		stable_put(map, val, NULL);

		free(val);
	}

	*dst = stable_keys_slist(map);

	stable_free(map);
}

// TODO generify
// unmarshal ORDER into order_name_desc, freeing first, removing invalid patterns
static void seq_to_order(struct SList **order_name_desc, const yaml_node_t *seq, yaml_document_t *document) {
	seq_to_string_list(order_name_desc, seq, document);

	slist_remove_all_free(order_name_desc, cfg_invalid_order_regex, NULL, NULL);
}

// unmarshal DISABLED into disabled, freeing first
static void seq_to_disabled(struct SList **disabled, const yaml_node_t *seq, yaml_document_t *document) {
	if (*disabled)
		slist_free_vals(disabled, NULL);

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *node = yaml_document_get_node(document, *item);
		if (!node)
			continue;

		struct Disabled *d = (struct Disabled*)calloc(1, sizeof(struct Disabled));

		switch (node->type) {
			case YAML_SCALAR_NODE:
				if (scalar_to_string(&d->name_desc, node)) {
					slist_append(disabled, d);
					continue;
				}
				break;

			case YAML_MAPPING_NODE:
				log_error("TODO disabled condition");
				break;

			default:
				log_warn("Ignoring invalid DISABLED: expected scalar or map, got %s", node_type_str(node->type));
				break;
		}

		// free on validation fail
		if (d)
			cfg_disabled_free(d);
	}
}

// unmarshal SCALE into user_scales, freeing first
static void seq_to_scales(struct SList **user_scales, const yaml_node_t *seq, yaml_document_t *document) {
	if (*user_scales)
		slist_free_vals(user_scales, NULL);

	for (yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *node = yaml_document_get_node(document, *item);
		if (!node)
			continue;

		const struct STable *map = map_to_node_table(node, document);
		if (!map)
			continue;

		struct UserScale *scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));

		const yaml_node_t *scalar;

		ctx_key("NAME_DESC");
		if ((scalar = stable_get(map, ctx.key)))
			if (!scalar_to_string(&scale->name_desc, scalar))
				goto seq_to_scales_done;

		ctx_name_desc(scale->name_desc);

		ctx_key("SCALE");
		if ((scalar = stable_get(map, ctx.key)))
			if (!scalar_to_float(&scale->scale, scalar))
				goto seq_to_scales_done;

		// OK
		slist_append(user_scales, scale);
		scale = NULL;

seq_to_scales_done:

		stable_free(map);

		// free on validation fail
		if (scale)
			cfg_user_scale_free(scale);
	}
}

// unmarshal MODE into user_modes, freeing first
static void seq_to_modes(struct SList **user_modes, const yaml_node_t *seq, yaml_document_t *document) {
	if (*user_modes)
		slist_free_vals(user_modes, NULL);

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *node = yaml_document_get_node(document, *item);
		if (!node)
			continue;

		const struct STable *map = map_to_node_table(node, document);
		if (!map)
			continue;

		struct UserMode *mode = cfg_user_mode_default();

		const yaml_node_t *scalar;

		ctx_key("NAME_DESC");
		if ((scalar = stable_get(map, ctx.key)))
			if (!scalar_to_string(&mode->name_desc, scalar))
				goto seq_to_modes_done;
		// TODO missing and regex

		ctx_name_desc(mode->name_desc);

		ctx_key("WIDTH");
		if ((scalar = stable_get(map, ctx.key)))
			if (!scalar_to_int(&mode->width, scalar))
				goto seq_to_modes_done;

		ctx_key("HEIGHT");
		if ((scalar = stable_get(map, ctx.key)))
			if (!scalar_to_int(&mode->height, scalar))
				goto seq_to_modes_done;

		ctx_key("HZ");
		if ((scalar = stable_get(map, ctx.key))) {
			float hz;
			if (!scalar_to_float(&hz, scalar))
				goto seq_to_modes_done;

			mode->refresh_mhz = lround(hz * 1000);
		}

		ctx_key("MAX");
		if ((scalar = stable_get(map, ctx.key)))
			if (!scalar_to_boolean(&mode->max, scalar))
				goto seq_to_modes_done;

		// OK
		slist_append(user_modes, mode);
		mode = NULL;

seq_to_modes_done:

		stable_free(map);

		// free on validation fail
		if (mode)
			cfg_user_mode_free(mode);
	}
}

// unmarshal TRANSFORM into user_transforms, freeing first
static void seq_to_transforms(struct SList **user_transforms, const yaml_node_t *seq, yaml_document_t *document) {
	if (*user_transforms)
		slist_free_vals(user_transforms, NULL);

	for (const yaml_node_item_t *item = seq->data.sequence.items.start; item < seq->data.sequence.items.top; item ++) {
		const yaml_node_t *node = yaml_document_get_node(document, *item);
		if (!node)
			continue;

		const struct STable *map = map_to_node_table(node, document);
		if (!map)
			continue;

		struct UserTransform *transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

		const yaml_node_t *scalar;

		ctx_key("NAME_DESC");
		if ((scalar = stable_get(map, ctx.key)))
			if (!scalar_to_string(&transform->name_desc, scalar))
				goto seq_to_transforms_done;

		ctx_name_desc(transform->name_desc);

		ctx_key("TRANSFORM");
		if ((scalar = stable_get(map, ctx.key)))
			if (!scalar_to_enum((int*)&transform->transform, scalar, transform_val))
				goto seq_to_transforms_done;

		// OK
		slist_append(user_transforms, transform);
		transform = NULL;

seq_to_transforms_done:

		stable_free(map);

		// free on validation fail
		if (transform)
			cfg_user_transform_free(transform);
	}
}

static bool unmarshal_cfg(struct Cfg *cfg, yaml_document_t *document) {

	const yaml_node_t *start_doc = document->nodes.start;
	if (start_doc->type != YAML_MAPPING_NODE) {
		// TODO error
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
				seq_to_scales(&cfg->user_scales, value, document);
				break;
			case MODE:
				seq_to_modes(&cfg->user_modes, value, document);
				break;
			case TRANSFORM:
				seq_to_transforms(&cfg->user_transforms, value, document);
				break;
			case VRR_OFF:
				seq_to_string_list(&cfg->adaptive_sync_off_name_desc, value, document);
				break;
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
				seq_to_disabled(&cfg->disabled, value, document);
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
				// TODO maybe unknown key warning
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

	// TODO deal with the unsigned char

	yaml_parser_t parser;
	yaml_document_t document;

	// TODO validate
	FILE *input = fopen(cfg->file_path, "rb");

	if (!yaml_parser_initialize(&parser)) {

		// TODO all error handling and destruction
		fprintf(stderr, "Could not initialize the parser object\n");
		return 1;
	}

	yaml_parser_set_input_file(&parser, input);

	yaml_parser_load(&parser, &document);

	yaml_document_get_root_node(&document);

	unmarshal_cfg(cfg, &document);

	yaml_document_delete(&document);

	yaml_parser_delete(&parser);

	fclose(input);

	return true;
}


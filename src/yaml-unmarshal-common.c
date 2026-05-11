#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml-unmarshal.h"

#include "cfg.h"
#include "convert.h"
#include "log.h"

struct UnmarshalCtx ctx = { 0 };

struct UnmarshalLogCtx {
	char *name_desc;
	char *def;
	char *key;
	char *top;
} log_ctx;

void ctx_action(const char *action) {
	if (ctx.action)
		free(ctx.action);
	ctx.action = action ? strdup(action) : NULL;
}

void ctx_top(const char *top) {
	if (log_ctx.top)
		free(log_ctx.top);
	log_ctx.top = top ? strdup(top) : NULL;
}

void ctx_name_desc(const char *name_desc) {
	if (log_ctx.name_desc)
		free(log_ctx.name_desc);
	log_ctx.name_desc = name_desc ? strdup(name_desc) : NULL;
}

void ctx_key(const char *key) {
	if (log_ctx.key)
		free(log_ctx.key);
	log_ctx.key = key ? strdup(key) : NULL;
}

void ctx_def(const char *def) {
	if (log_ctx.def)
		free(log_ctx.def);
	log_ctx.def = def ? strdup(def) : NULL;
}

void ctx_reset(void) {
	ctx.t = WARNING;
	ctx_action(NULL);
	ctx_top(NULL);
	ctx_name_desc(NULL);
	ctx_key(NULL);
	ctx_def(NULL);
}

// return a static string for the node type
char *node_type_str(const yaml_node_type_t type) {
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

	char *msg = NULL;

	if (ctx.action)
		msg = sprintf_append(msg, "\n%s:", ctx.action);
	else
		msg = sprintf_append(msg, "Ignoring");

	if (log_ctx.top)
		msg = sprintf_append(msg, " invalid %s", log_ctx.top);
	if (log_ctx.name_desc)
		msg = sprintf_append(msg, " %s", log_ctx.name_desc);
	if (log_ctx.key)
		msg = sprintf_append(msg, " %s", log_ctx.key);
	if (type_expected)
		msg = sprintf_append(msg, " expected %s, got %s", node_type_str(type_expected), node_type_str(type_actual));
	if (value)
		msg = sprintf_append(msg, " %s", value);
	if (log_ctx.def)
		msg = sprintf_append(msg, ", using default %s", log_ctx.def);

	if (msg) {
		log_(ctx.t, "%s", msg);
		free(msg);
	}
}

static void log_misssing(void) {

	char *msg = NULL;

	if (ctx.action)
		msg = sprintf_append(msg, "\n%s: missing %s", ctx.action, log_ctx.top);
	else
		msg = sprintf_append(msg, "%s: Ignoring missing", log_ctx.top);

	if (log_ctx.key)
		msg = sprintf_append(msg, " %s", log_ctx.key);

	if (log_ctx.name_desc)
		msg = sprintf_append(msg, " for '%s'", log_ctx.name_desc);

	if (msg) {
		log_(ctx.t, "%s", msg);
		free(msg);
	}
}

static void log_invalid_value(const yaml_char_t *value) {
	log_invalid(value, YAML_NO_NODE, YAML_NO_NODE);
}

static void log_invalid_type(const yaml_node_type_t expected, const yaml_node_t *node) {
	log_invalid(NULL, expected, node ? node->type : YAML_NO_NODE);
}

bool check_node_type(const yaml_node_t *node, const yaml_node_type_t expected) {
	if (node && node->type == expected)
		return true;

	log_invalid_type(expected, node);

	return false;
}

bool check_mandatory(const yaml_node_t *node) {
	if (node)
		return true;

	log_misssing();

	return false;
}

char *scalar_to_string(const yaml_node_t *scalar) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return NULL;

	return(strdup((char*)scalar->data.scalar.value));
}

bool scalar_to_int(int32_t *dst, const yaml_node_t *scalar) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%d", dst) == 1)
		return true;

	log_invalid_value(scalar->data.scalar.value);
	return false;
}

bool scalar_to_float(float *dst, const yaml_node_t *scalar) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return false;

	if (sscanf((char*)scalar->data.scalar.value, "%f", dst) == 1)
		return true;

	log_invalid_value(scalar->data.scalar.value);
	return false;
}

bool scalar_to_float_def(float *dst, float def, const yaml_node_t *scalar) {
	bool ok = true;

	char def_str[10];
	snprintf(def_str, 10, "%.1f", def);

	ctx_def(def_str);

	if (!(ok = scalar_to_float(dst, scalar)))
		*dst = def;

	ctx_def(NULL);

	return ok;
}

int scalar_to_enum(const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val) {
	if (!check_node_type(scalar, YAML_SCALAR_NODE))
		return 0;

	int val = fn_val((char*)scalar->data.scalar.value);
	if (val)
		return val;

	log_invalid_value(scalar->data.scalar.value);
	return 0;
}

int scalar_to_enum_def(const int def, const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val, scalar_to_enum_fn_name fn_name) {
	ctx_def(fn_name(def));

	int ret = scalar_to_enum(scalar, fn_val);
	if (!ret)
		ret = def;

	ctx_def(NULL);

	return ret;
}

bool scalar_to_boolean(bool *dst, const yaml_node_t *scalar) {
	int val = scalar_to_enum(scalar, on_off_val);

	if (val) {
		*dst = val == ON;
		return true;
	}

	return false;
}

bool invalid_regex(const void *pattern, const void *unused) {
	bool rc = false;
	char *p = (char*)pattern;

	if (p && p[0] == '!') {
		regex_t regex;
		int result = regcomp(&regex, p + 1, REG_EXTENDED);
		if (result) {
			char err[1024];
			regerror(result, &regex, err, 1024);
			log_warn("Ignoring invalid %s regex '%s':  %s", log_ctx.top, p + 1, err);
			rc = true;
		}
		regfree(&regex);
	}
	return rc;
}

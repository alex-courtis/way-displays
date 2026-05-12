#include <regex.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml/unmarshal-context.h"

#include "log.h"

// TODO move to yaml directory

struct UnmarshalCtx ctx = { 0 };

struct UnmarshalLogCtx {
	enum LogThreshold t;
	char *prefix;
	char *def;
	char *key;
	char *name_desc;
	char *top;
} log_ctx;

void yaml_log_ctx_t(const enum LogThreshold t) {
	log_ctx.t = t;
}

void yaml_log_ctx_prefix(const char *action) {
	if (log_ctx.prefix)
		free(log_ctx.prefix);
	log_ctx.prefix = action ? strdup(action) : NULL;
}

void yaml_log_ctx_def(const char *def) {
	if (log_ctx.def)
		free(log_ctx.def);
	log_ctx.def = def ? strdup(def) : NULL;
}

void yaml_log_ctx_key(const char *key) {
	if (log_ctx.key)
		free(log_ctx.key);
	log_ctx.key = key ? strdup(key) : NULL;
}

void yaml_log_ctx_name_desc(const char *name_desc) {
	if (log_ctx.name_desc)
		free(log_ctx.name_desc);
	log_ctx.name_desc = name_desc ? strdup(name_desc) : NULL;
}

void yaml_log_ctx_top(const char *top) {
	if (log_ctx.top)
		free(log_ctx.top);
	log_ctx.top = top ? strdup(top) : NULL;
}

void yaml_log_ctx_reset(void) {
	yaml_log_ctx_t(WARNING);
	yaml_log_ctx_prefix(NULL);
	yaml_log_ctx_top(NULL);
	yaml_log_ctx_name_desc(NULL);
	yaml_log_ctx_key(NULL);
	yaml_log_ctx_def(NULL);
}

// return a static string for the node type
char *yaml_node_type_str(const yaml_node_type_t type) {
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

static void yaml_log_invalid(const yaml_char_t *value, const yaml_node_type_t type_expected, const yaml_node_type_t type_actual) {

	char *msg = NULL;

	if (log_ctx.prefix)
		msg = sprintf_append(msg, "\n%s:", log_ctx.prefix);
	else
		msg = sprintf_append(msg, "Ignoring");

	if (log_ctx.top)
		msg = sprintf_append(msg, " invalid %s", log_ctx.top);
	if (log_ctx.name_desc)
		msg = sprintf_append(msg, " %s", log_ctx.name_desc);
	if (log_ctx.key)
		msg = sprintf_append(msg, " %s", log_ctx.key);
	if (type_expected)
		msg = sprintf_append(msg, " expected %s, got %s", yaml_node_type_str(type_expected), yaml_node_type_str(type_actual));
	if (value)
		msg = sprintf_append(msg, " %s", value);
	if (log_ctx.def)
		msg = sprintf_append(msg, ", using default %s", log_ctx.def);

	if (msg) {
		log_(log_ctx.t, "%s", msg);
		free(msg);
	}
}

static void yaml_log_misssing(void) {

	char *msg = NULL;

	if (log_ctx.prefix)
		msg = sprintf_append(msg, "\n%s: missing %s", log_ctx.prefix, log_ctx.top);
	else
		msg = sprintf_append(msg, "%s: Ignoring missing", log_ctx.top);

	if (log_ctx.key)
		msg = sprintf_append(msg, " %s", log_ctx.key);

	if (log_ctx.name_desc)
		msg = sprintf_append(msg, " for '%s'", log_ctx.name_desc);

	if (msg) {
		log_(log_ctx.t, "%s", msg);
		free(msg);
	}
}

void yaml_log_invalid_value(const yaml_char_t *value) {
	yaml_log_invalid(value, YAML_NO_NODE, YAML_NO_NODE);
}

bool yaml_check_node_type(const yaml_node_t *node, const yaml_node_type_t expected) {
	if (node && node->type == expected)
		return true;

	yaml_log_invalid(NULL, expected, node ? node->type : YAML_NO_NODE);

	return false;
}

bool yaml_check_mandatory(const yaml_node_t *node) {
	if (node)
		return true;

	yaml_log_misssing();

	return false;
}

// fn_equals to valdate a regex pattern by attempting to compile it
bool yaml_valid_regex(const void *pattern) {
	bool rc = true;
	char *p = (char*)pattern;

	if (p && p[0] == '!') {
		regex_t regex;
		int result = regcomp(&regex, p + 1, REG_EXTENDED);
		if (result) {
			char err[1024];
			regerror(result, &regex, err, 1024);
			log_warn("Ignoring invalid %s regex '%s':  %s", log_ctx.top, p + 1, err);
			rc = false;
		}
		regfree(&regex);
	}
	return rc;
}


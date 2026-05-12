#ifndef YAML_UNMARSHAL_CONTEXT_H
#define YAML_UNMARSHAL_CONTEXT_H

#include <stdbool.h>
#include <yaml.h>

#include "log.h"

// TODO abstract this away
extern struct UnmarshalCtx {
	yaml_document_t *document;
} ctx;

// clear the yaml unmarshalling logging context
void yaml_log_ctx_reset(void);

// log threshold for unmarshalling failures
void yaml_log_ctx_t(const enum LogThreshold t);

// optional prefix for unmarshalling log messages
void yaml_log_ctx_prefix(const char *action);

// optional default value
void yaml_log_ctx_def(const char *def);

// optional key name
void yaml_log_ctx_key(const char *key);

// optional NAME_DESC for context
void yaml_log_ctx_name_desc(const char *name_desc);

// optional top level element name
void yaml_log_ctx_top(const char *top);

// return a static string for the node type
char *yaml_node_type_str(const yaml_node_type_t type);

// validate expected is of type actual, returning false and logging a warning if not
bool yaml_check_node_type(const yaml_node_t *node, const yaml_node_type_t expected);

// check that node is not null, logging a contextual warning
bool yaml_check_mandatory(const yaml_node_t *node);

void yaml_log_invalid_value(const yaml_char_t *value);

bool yaml_valid_regex(const void *pattern);

#endif // YAML_UNMARSHAL_CONTEXT_H


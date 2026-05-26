#ifndef YAML_UNMARSHAL_LOG_H
#define YAML_UNMARSHAL_LOG_H

#include <stdbool.h>
#include <yaml.h>

#include "convert.h"
#include "log.h"

/*
 * Controls logging for all unmarshalling failures
 * Context info is optional and added when available
 */

// clear the yaml unmarshalling logging context
void yaml_unmarshal_log_ctx_reset(void);

// set optional context information
void yaml_unmarshal_log_ctx_threshold(const enum LogThreshold t);
void yaml_unmarshal_log_ctx_prefix(const char *action); // message prefix
void yaml_unmarshal_log_ctx_def(const char *def); // default value
void yaml_unmarshal_log_ctx_key(const char *key); // failed key name
void yaml_unmarshal_log_ctx_name_desc(const char *name_desc); // NAME_DESC for context
void yaml_unmarshal_log_ctx_top(const char *top); // root map key
void yaml_unmarshal_log_ctx_valid_values_fn(enum_names_fn fn);

// explicitly log a value as invalid
void yaml_unmarshal_log_invalid_value(const yaml_char_t *value);

// validate actual is of type expected, returning false and logging a warning if not
bool yaml_check_node_type(const yaml_node_t *actual, const yaml_node_type_t expected);

// assert that node is not null
bool yaml_check_mandatory(const yaml_node_t *node);

// valdate a regex pattern by attempting to compile it
bool yaml_valid_regex(const void *pattern);

// return a static string for the node type
char *yaml_node_type_str(const yaml_node_type_t type);

#endif // YAML_UNMARSHAL_LOG_H


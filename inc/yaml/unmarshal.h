#ifndef YAML_UNMARSHAL_H
#define YAML_UNMARSHAL_H

#include <yaml.h>

#include "convert.h"
#include "log.h"

/*
 * Context available for the duration of marshalling
 */
struct UC {
	yaml_document_t d;

	enum LogThreshold t;
	char prefix[64];
	char def[64];
	char key[64];
	char name_desc[64];
	char top[64];
	enum_names_fn valid_names_fn;
};

/*
 * Unmarshal functions
 * Returns NULL and logs on failure
 */

typedef void *(*yaml_root_to_type_fn)(struct UC *c, const yaml_node_t *root);

// Unmarshal a yaml string, human is arbitrary and used for logging
void *yaml_unmarshal_str(const char *yaml, yaml_root_to_type_fn fn, char *human);

// Unmarshal a yaml file
void *yaml_unmarshal_file(const char *path, yaml_root_to_type_fn fn);

/*
 * Controls logging for all unmarshalling failures
 * Context info is optional and added when available
 */
void yaml_unmarshal_log_prefix         (struct UC *c, const char *prefix); // message prefix
void yaml_unmarshal_log_def            (struct UC *c, const char *def); // default value
void yaml_unmarshal_log_ctx_key        (struct UC *c, const char *key); // failed key name
void yaml_unmarshal_log_ctx_name_desc  (struct UC *c, const char *name_desc); // NAME_DESC for context
void yaml_unmarshal_log_ctx_top        (struct UC *c, const char *top); // root map key
void yaml_unmarshal_log_valid_values_fn(struct UC *c, enum_names_fn fn); // all valid enum values

// explicitly log a value as invalid
void yaml_unmarshal_log_invalid_value(struct UC *c, const yaml_char_t *value);

// validate actual is of type expected, returning false and logging a warning if not
bool yaml_check_node_type(struct UC *c, const yaml_node_t *actual, const yaml_node_type_t expected);

// assert that node is not null
bool yaml_check_mandatory(struct UC *c, const yaml_node_t *node);

// valdate a regex pattern by attempting to compile it
bool yaml_valid_regex(struct UC *c, const void *pattern);

// return a static string for the node type
char *yaml_node_type_str(const yaml_node_type_t type);

#endif // YAML_UNMARSHAL_H

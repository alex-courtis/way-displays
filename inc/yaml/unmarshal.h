#ifndef YAML_UNMARSHAL_H
#define YAML_UNMARSHAL_H

#include <stdbool.h>
#include <yaml.h>

#include "enum.h"

/*
 * Context available for the duration of marshalling
 */
struct UC {
	yaml_document_t d;

	enum LogThreshold t;
	char prefix[64];          // cfg file name or ipc operation
	char top[64];             // key e.g. MODE
	char key[64];             // sub-key e.g. WIDTH
	char name_desc[64];       // name_desc key for top e.g. 'DP-1'
	fn_enum_names enum_names; // accepted enums
	char def[64];             // default value
	bool v1_present;          // warn after unmarshal if v1 elements were parsed
};

/*
 * Unmarshal functions
 * Returns NULL and logs on failure
 */

typedef void *(*fn_yaml_root_to_type)(struct UC *c, const yaml_node_t *root);

// Unmarshal a yaml string, human is arbitrary and used for logging
void *yaml_unmarshal_str(const char *yaml, fn_yaml_root_to_type fn, char *human);

// Unmarshal a yaml file
void *yaml_unmarshal_file(const char *path, fn_yaml_root_to_type fn);

/*
 * Controls logging for all unmarshalling failures
 * Context info is optional and added when available
 */
void yaml_unmarshal_log_prefix       (struct UC *c, const char *prefix); // message prefix
void yaml_unmarshal_log_def          (struct UC *c, const char *def); // default value
void yaml_unmarshal_log_ctx_key      (struct UC *c, const char *key); // failed key name
void yaml_unmarshal_log_ctx_name_desc(struct UC *c, const char *name_desc); // NAME_DESC for context
void yaml_unmarshal_log_ctx_top      (struct UC *c, const char *top); // root map key
void yaml_unmarshal_log_enum_names   (struct UC *c, fn_enum_names fn); // all valid enum values

// explicitly log a value as invalid with free-form expectation message
void yaml_unmarshal_log_invalid_value(struct UC *c, const yaml_char_t *value, const char *expected);

// validate that the node is a scalar, return false and log a warning with free-form expected message if not
bool yaml_check_is_scalar(struct UC *c, const yaml_node_t *node, const char *expected);

// validate actual is one of expected type, returning false and logging an expected-got warning if not
bool yaml_check_node_type(struct UC *c, const yaml_node_t *node_actual, const yaml_node_type_t type1, const yaml_node_type_t type2);

// if pattern starts with '!' return false if it fails to compile, otherwise return true
bool yaml_valid_name_desc(struct UC *c, const char *pattern);

// return a static string for the node type
char *yaml_node_type_str(const yaml_node_type_t type);

#endif // YAML_UNMARSHAL_H

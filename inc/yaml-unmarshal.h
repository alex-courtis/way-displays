#ifndef YAML_UNMARSHAL_H
#define YAML_UNMARSHAL_H

#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

#include "log.h"

extern struct UnmarshalCtx {
	enum LogThreshold t;

	yaml_document_t *document;

	char *action;
	// char *top;
	// char *name_desc;
	// char *key;
	// char *def;
} ctx;

void ctx_action(const char *action);
void ctx_top(const char *top);
void ctx_name_desc(const char *name_desc);
void ctx_key(const char *key);
void ctx_def(const char *def);
void ctx_reset(void);

// return a static string for the node type
char *node_type_str(const yaml_node_type_t type);

// validate expected is of type actual, returning false and logging a warning if not
bool check_node_type(const yaml_node_t *node, const yaml_node_type_t expected);

// check that node is not null, logging a contextual warning
bool check_mandatory(const yaml_node_t *node);

// unmarshal a scalar to a strdup string
char *scalar_to_string(const yaml_node_t *scalar);

// unmarshal a scalar int to dst
bool scalar_to_int(int32_t *dst, const yaml_node_t *scalar);

// unmarshal a scalar float to dst
bool scalar_to_float(float *dst, const yaml_node_t *scalar);

// unmarshal a scalar float to dst, sets def on failure
bool scalar_to_float_def(float *dst, float def, const yaml_node_t *scalar);

// unmarshal a scalar enum
typedef unsigned int (*scalar_to_enum_fn_val)(const char *name);
typedef const char* (*scalar_to_enum_fn_name)(unsigned int val);
int scalar_to_enum(const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val);

// unmarshal an scalar enum, returns def on failure
int scalar_to_enum_def(const int def, const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val, scalar_to_enum_fn_name fn_name);

// unmarshal a scalar bool to dst
bool scalar_to_boolean(bool *dst, const yaml_node_t *scalar);

// fn_equals to valdate a regex pattern by attempting to compile it
bool invalid_regex(const void *pattern, const void *unused);

#endif // YAML_UNMARSHAL_H


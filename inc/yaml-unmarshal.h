#ifndef YAML_UNMARSHAL_H
#define YAML_UNMARSHAL_H

#include <stdbool.h>
#include <stdint.h>
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

// unmarshal a scalar to a strdup string
char *yaml_scalar_to_string(const yaml_node_t *scalar);

// unmarshal a scalar int to dst
bool yaml_scalar_to_int(int32_t *dst, const yaml_node_t *scalar);

// unmarshal a scalar float to dst
bool yaml_scalar_to_float(float *dst, const yaml_node_t *scalar);

// unmarshal a scalar float to dst, sets def on failure
bool yaml_scalar_to_float_def(float *dst, float def, const yaml_node_t *scalar);

// unmarshal a scalar enum
typedef unsigned int (*scalar_to_enum_fn_val)(const char *name);
typedef const char* (*scalar_to_enum_fn_name)(unsigned int val);
int yaml_scalar_to_enum(const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val);

// unmarshal an scalar enum, returns def on failure
int yaml_scalar_to_enum_def(const int def, const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val, scalar_to_enum_fn_name fn_name);

// unmarshal a scalar bool to dst
bool yaml_scalar_to_boolean(bool *dst, const yaml_node_t *scalar);

// unmarshal a scalar to a name_desc, validating regex
char *yaml_scalar_to_name_desc(const yaml_node_t *scalar);

// unmarshal a sequence of valid name_desc, removing duplicates and validating regex
struct SList *yaml_seq_to_name_desc_list(const yaml_node_t *seq);

#endif // YAML_UNMARSHAL_H


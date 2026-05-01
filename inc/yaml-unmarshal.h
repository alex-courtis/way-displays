#ifndef YAML_UNMARSHAL_H
#define YAML_UNMARSHAL_H

#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

#include "cfg.h"
#include "slist.h"

// TODO
// deal with unsigned char
// clean up test alternates, gitignore etc.
// consider SSet
// take another look at non-compact output

// global, set and unset by TODO
extern struct UnmarshalCtx {
	yaml_document_t *document;
	char *yaml;
	enum CfgElement element;
	yaml_node_type_t type_expected;
	yaml_node_type_t type_actual;
	char *name_desc;
	char *key;
	char *def;
} unmarshal_ctx;

void unmarshal_ctx_clear(void);

void unmarshal_ctx_yaml(char *yaml);

// // return a static string for the node type
// char* node_type_str(const yaml_node_type_t type);
//
// // check that node is not null, logging a contextual warning
// bool check_mandatory(const yaml_node_t *node);
//
// // validate expected is of type actual, returning false and logging a warning if not
// bool check_node_type(const yaml_node_t *node, const yaml_node_type_t expected);
//
// // unmarshal a scalar string to dst
// bool scalar_to_string(char **dst, const yaml_node_t *scalar);
//
// // unmarshal a scalar int to dst
// bool scalar_to_int(int32_t *dst, const yaml_node_t *scalar);
//
// // unmarshal a scalar float to dst
// bool scalar_to_float(float *dst, const yaml_node_t *scalar);
//
// unmarshal an scalar enum to dst
typedef unsigned int (*scalar_to_enum_fn_val)(const char *name);
typedef const char* (*scalar_to_enum_fn_name)(unsigned int val);
bool scalar_to_enum(int *dst, const yaml_node_t *scalar, scalar_to_enum_fn_val fn_val);
//
// // unmarshal a scalar bool to dst
// bool scalar_to_boolean(bool *dst, const yaml_node_t *scalar);
//
// void log_invalid_value(const yaml_char_t *value);
//
// void log_invalid(void);
//
// void log_misssing(void);

#endif // YAML_UNMARSHAL_H


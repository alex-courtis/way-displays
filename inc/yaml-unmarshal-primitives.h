#ifndef YAML_UNMARSHAL_PRIMITIVES_H
#define YAML_UNMARSHAL_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

/*
 * Primitives
 */

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

/*
 * Utility
 */

// unmarshal a map of nodes
const struct STable *yaml_map_to_node_table(const yaml_node_t *map);

// unmarshal a sequence into a list of structs
typedef void *(*node_to_type_fn)(const yaml_node_t *node);
struct SList *seq_to_type_list(const yaml_node_t *seq, node_to_type_fn fn);

#endif // YAML_UNMARSHAL_PRIMITIVES_H


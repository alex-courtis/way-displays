#ifndef YAML_UNMARSHAL_PRIMITIVES_H
#define YAML_UNMARSHAL_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

/*
 * Function pointer types
 */
typedef unsigned int (*yaml_scalar_to_enum_fn_val)(const char *name);

typedef const char* (*yaml_scalar_to_enum_fn_name)(unsigned int val);

typedef void *(*yaml_node_to_type_fn)(const yaml_node_t *node);

/*
 * Unmarshal primitives: NULL or false on failure
 */
char *yaml_scalar_to_string(const yaml_node_t *scalar);

bool yaml_scalar_to_int(int32_t *dst, const yaml_node_t *scalar);

bool yaml_scalar_to_float(float *dst, const yaml_node_t *scalar);

bool yaml_scalar_to_float_def(float *dst, float def, const yaml_node_t *scalar);

int yaml_scalar_to_enum(const yaml_node_t *scalar, yaml_scalar_to_enum_fn_val fn_val);

int yaml_scalar_to_enum_def(const int def, const yaml_node_t *scalar, yaml_scalar_to_enum_fn_val fn_val, yaml_scalar_to_enum_fn_name fn_name);

bool yaml_scalar_to_boolean(bool *dst, const yaml_node_t *scalar);

/*
 * Utility: NULL on failure
 */

const struct STable *yaml_map_to_node_table(const yaml_node_t *map);

struct SList *yaml_seq_to_type_list(const yaml_node_t *seq, yaml_node_to_type_fn fn);

#endif // YAML_UNMARSHAL_PRIMITIVES_H


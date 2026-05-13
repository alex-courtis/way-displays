#ifndef YAML_UNMARSHAL_PRIMITIVES_H
#define YAML_UNMARSHAL_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

#include "convert.h"

/*
 * Functions to extract primitives from yaml_document
 * Returns NULL or false and logs on failure
 */

// extract scalars
char *yaml_scalar_to_string(const yaml_node_t *scalar);
bool  yaml_scalar_to_int(int32_t *dst, const yaml_node_t *scalar);
bool  yaml_scalar_to_float(float *dst, const yaml_node_t *scalar);
bool  yaml_scalar_to_float_def(float *dst, float def, const yaml_node_t *scalar);
int   yaml_scalar_to_enum(const yaml_node_t *scalar, enum_val_fn val_fn);
int   yaml_scalar_to_enum_def(const int def, const yaml_node_t *scalar, enum_val_fn val_fn, enum_name_fn name_fn);
bool  yaml_scalar_to_boolean(bool *dst, const yaml_node_t *scalar);

// create a list of structs using fn to unmarshal each item
typedef void *(*yaml_node_to_type_fn)(const yaml_node_t *node);
struct SList *yaml_seq_to_type_list(const yaml_node_t *seq, yaml_node_to_type_fn fn);

// create a table of yaml_node_t indexed by key
const struct STable *yaml_map_to_node_table(const yaml_node_t *map);

#endif // YAML_UNMARSHAL_PRIMITIVES_H


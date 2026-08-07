#ifndef YAML_UNMARSHAL_PRIMITIVES_H
#define YAML_UNMARSHAL_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

#include "enum.h"
#include "yaml/unmarshal.h"

/*
 * Functions to extract primitives from yaml_document
 * Returns NULL or false and logs on failure
 */

// extract scalars
char *yaml_scalar_to_string    (                           const yaml_node_t *scalar);
char *yaml_scalar_to_string_def(          const char *def, const yaml_node_t *scalar);
bool  yaml_scalar_to_int       (int32_t *dst,              const yaml_node_t *scalar);
bool  yaml_scalar_to_int_def   (int32_t *dst, int32_t def, const yaml_node_t *scalar);
bool  yaml_scalar_to_float     (float *dst,                const yaml_node_t *scalar);
bool  yaml_scalar_to_float_def (float *dst,     float def, const yaml_node_t *scalar);
int   yaml_scalar_to_enum      (                           const yaml_node_t *scalar, fn_enum_val val,                    fn_enum_names names);
int   yaml_scalar_to_enum_def  (            const int def, const yaml_node_t *scalar, fn_enum_val val, fn_enum_name name, fn_enum_names names);
bool  yaml_scalar_to_boolean   (bool *dst,                 const yaml_node_t *scalar);
int   yaml_scalar_to_on_off_def(     const enum OnOff def, const yaml_node_t *scalar);

// put into col using fn to unmarshal each item
typedef void (*fn_yaml_node_into_col)(const void *col, const yaml_node_t *node);
bool yaml_seq_into_col(const yaml_node_t *seq, const void *col, fn_yaml_node_into_col fn);

// create a map of yaml_node_t indexed by key
const struct SPmap *yaml_map_to_spmap(const yaml_node_t *map);

#endif // YAML_UNMARSHAL_PRIMITIVES_H


#ifndef YAML_UNMARSHAL_PRIMITIVES_H
#define YAML_UNMARSHAL_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

#include "convert.h"
#include "yaml/unmarshal.h"

/*
 * Functions to extract primitives from yaml_document
 * Returns NULL or false and logs on failure
 */

// extract scalars
char *yaml_scalar_to_string    (struct UC *c,                                const yaml_node_t *scalar);
char *yaml_scalar_to_string_def(struct UC *c,               const char *def, const yaml_node_t *scalar);
bool  yaml_scalar_to_int       (struct UC *c, int32_t *dst,                  const yaml_node_t *scalar);
bool  yaml_scalar_to_int_def   (struct UC *c, int32_t *dst, int32_t def,     const yaml_node_t *scalar);
bool  yaml_scalar_to_float     (struct UC *c, float *dst,                    const yaml_node_t *scalar);
bool  yaml_scalar_to_float_def (struct UC *c, float *dst,   float def,       const yaml_node_t *scalar);
int   yaml_scalar_to_enum      (struct UC *c,                                const yaml_node_t *scalar, fn_enum_val val,                    fn_enum_names names);
int   yaml_scalar_to_enum_def  (struct UC *c, const int def,                 const yaml_node_t *scalar, fn_enum_val val, fn_enum_name name, fn_enum_names names);
bool  yaml_scalar_to_boolean   (struct UC *c, bool *dst,                     const yaml_node_t *scalar);

// put into col using fn to unmarshal each item
typedef void (*fn_yaml_node_into_col)(struct UC *c, const void *col, const yaml_node_t *node);
bool yaml_seq_into_col(struct UC *c, const yaml_node_t *seq, const void *col, fn_yaml_node_into_col fn);

// create a table of yaml_node_t indexed by key
const struct SMap *yaml_map_to_node_table(struct UC *c, const yaml_node_t *map);

#endif // YAML_UNMARSHAL_PRIMITIVES_H


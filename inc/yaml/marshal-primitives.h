#ifndef YAML_MARSHAL_PRIMITIVES_H
#define YAML_MARSHAL_PRIMITIVES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "convert.h"
#include "pmap.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "smapi.h"
#include "sset.h"
#include "yaml/marshal.h"

/*
 * Functions to add nodes to an existing yaml_document_t
 * NOP on failure to add to the document
 * NOP when NULL data
 */

// create a node from the pointer val, never null
typedef int (*fn_yaml_v_to_node)(struct MC *c, const void* const val);

// create a node from the key and pointer val, never null
typedef int (*fn_yaml_kv_to_node)(struct MC *c, const char* const key, const void* const val);

// create a node from the key and integer val
typedef int (*fn_yaml_ki_to_node)(struct MC *c, const char* const key, const size_t val);

// add a node or scalar pair to an existing maping node
void yaml_map_add_node    (struct MC *c, const char *key,       int     node,                       int mapping); // NOP on 0
void yaml_map_add_str     (struct MC *c, const char *key, const char    *str,                       int mapping); // NOP on NULL
void yaml_map_add_int     (struct MC *c, const char *key, const int32_t val,                        int mapping);
void yaml_map_add_int_nz  (struct MC *c, const char *key, const int32_t val,                        int mapping); // NOP on 0
void yaml_map_add_float_nz(struct MC *c, const char *key, const float   val,                        int mapping); // NOP on 0
void yaml_map_add_bool    (struct MC *c, const char *key, const bool    val,                        int mapping);
void yaml_map_add_enum    (struct MC *c, const char *key, const int     val,  fn_enum_name fn_name, int mapping); // NOP on 0 enum

// Create a new sequence node and add it to an existing mapping node
// New sequence node values are populated by evaluating fn on each item
void yaml_map_add_sset (struct MC *c, const char *key, const struct SSet*  const sset,                         int mapping);
void yaml_map_add_pset (struct MC *c, const char *key, const struct PSet*  const pset,  fn_yaml_v_to_node fn,  int mapping);
void yaml_map_add_list (struct MC *c, const char *key, const struct SList* const list,  fn_yaml_v_to_node fn,  int mapping);
void yaml_map_add_pmap (struct MC *c, const void *key, const struct PMap*  const pmap,  fn_yaml_kv_to_node fn, int mapping);
void yaml_map_add_smap (struct MC *c, const char *key, const struct SMap*  const smap,  fn_yaml_kv_to_node fn, int mapping);
void yaml_map_add_smapi(struct MC *c, const char *key, const struct SMapI* const smapi, fn_yaml_ki_to_node fn, int mapping);

#endif // YAML_MARSHAL_PRIMITIVES_H

#ifndef YAML_MARSHAL_PRIMITIVES_H
#define YAML_MARSHAL_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>

#include "enum.h"
#include "plist.h"
#include "ppmap.h"
#include "pset.h"
#include "sset.h"
#include "yaml/marshal.h"

/*
 * Functions to add nodes to an existing yaml_document_t
 * NOP on failure to add to the document
 * NOP when NULL data
 */

// add a node to an existing maping node
void yaml_map_add_node    (struct MC *c, const char *key,       int     node,                       int mapping); // NOP on 0
																												  //
// add a scalar pair to an existing maping node
void yaml_map_add_str     (struct MC *c, const char *key, const char    *str,                       int mapping); // NOP on NULL
void yaml_map_add_int     (struct MC *c, const char *key, const int32_t val,                        int mapping);
void yaml_map_add_int_nz  (struct MC *c, const char *key, const int32_t val,                        int mapping); // NOP on 0
void yaml_map_add_float_nz(struct MC *c, const char *key, const float   val,                        int mapping); // NOP on 0
void yaml_map_add_bool    (struct MC *c, const char *key, const bool    val,                        int mapping);
void yaml_map_add_enum    (struct MC *c, const char *key, const int     val,  fn_enum_name fn_name, int mapping); // NOP on 0 enum

// Create a new sequence node popluated by evaluated fn on each item and add it to an existing mapping node
typedef int (*fn_yaml_node_from_type)(struct MC *c, const void* const val);
void yaml_map_add_plist(struct MC *c, const char *key, const struct Plist* const plist, fn_yaml_node_from_type fn, int mapping);

// Create a new sequence node populated with scalars and add it to an existing mapping node
void yaml_map_add_sset(struct MC *c, const char *key, const struct Sset* const sset, int mapping);

#endif // YAML_MARSHAL_PRIMITIVES_H

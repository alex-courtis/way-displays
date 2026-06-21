#ifndef YAML_MARSHAL_PRIMITIVES_H
#define YAML_MARSHAL_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>

#include "convert.h"
#include "slist.h"
#include "smap.h"
#include "yaml/marshal.h"

// TODO rename x_fn to fn_x

/*
 * Functions to add nodes to an existing yaml_document_t
 * Returns false on failure to add to the document
 */

// add a scalar pair to an existing maping node
bool yaml_map_add_str     (struct MC *c, const char *key, const char *str,                         int mapping); // returns true and does nothing on NULL
bool yaml_map_add_int     (struct MC *c, const char *key, const int32_t val,                       int mapping);
bool yaml_map_add_int_nz  (struct MC *c, const char *key, const int32_t val,                       int mapping); // returns true and does nothing on 0
bool yaml_map_add_float_nz(struct MC *c, const char *key, const float val,                         int mapping); // returns true and does nothing on 0
bool yaml_map_add_bool    (struct MC *c, const char *key, const bool val,                          int mapping);
bool yaml_map_add_enum    (struct MC *c, const char *key, const int val,     enum_name_fn fn_name, int mapping); // returns true on 0 enum

// Create a new map node and add it to an existing maping node
// New map node is populated by evaluating fn on data
typedef bool (*yaml_map_populate_fn)(struct MC *c, const void *data, const int mapping);
bool yaml_map_add_map(struct MC *c, const char *key, const void *data, yaml_map_populate_fn fn, int mapping);

// Create a new sequence node and add it to an existing mapping node
// New sequence node values are populated by evaluating fn on each item
typedef bool (*yaml_seq_append_fn)(struct MC *c, const void *data, const int sequence);
bool yaml_map_add_seq_list(struct MC *c, const char *key, const struct SList *list, yaml_seq_append_fn fn, int mapping);
bool yaml_map_add_seq_smap(struct MC *c, const char *key, const struct SMap* smap, yaml_seq_append_fn fn, int mapping);

// yaml_seq_append_fn: append a scalar item to an existing sequence node
bool yaml_seq_append_str(struct MC *c, const void *str, int sequence);

#endif // YAML_MARSHAL_PRIMITIVES_H

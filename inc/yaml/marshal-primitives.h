#ifndef YAML_MARSHAL_PRIMITIVES_H
#define YAML_MARSHAL_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>

#include "convert.h"
#include "slist.h"

/*
 * Functions to add entries to an existing yaml document nodes
 * Returns false on failure to add to the document
 */

// yaml_map_populate_fn: add a scalar to a map node
bool yaml_map_add_str(const char *key, const char *str, int mapping);
bool yaml_map_add_int(const char *key, const int32_t val, int mapping);
bool yaml_map_add_float(const char *key, const float val, int mapping);
bool yaml_map_add_bool(const char *key, const bool val, int mapping);
bool yaml_map_add_enum(const char *key, const int val, enum_name_fn fn_name, int mapping);

// yaml_seq_append_fn: add a scalar to a sequence node
bool yaml_seq_append_str(const void *str, int sequence);

// Add a new map to a map node by evaluating fn on data
typedef bool (*yaml_map_populate_fn)(const void *data, const int mapping);
bool yaml_map_add_map(const char *key, const void *data, yaml_map_populate_fn fn, int mapping);

// Add a new sequence to a map node by evaluating fn on each item in list
typedef bool (*yaml_seq_append_fn)(const void *data, const int sequence);
bool yaml_map_add_seq(const char *key, const struct SList *list, yaml_seq_append_fn fn, int mapping);

#endif // YAML_MARSHAL_PRIMITIVES_H

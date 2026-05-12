#ifndef YAML_MARSHAL_PRIMITIVES_H
#define YAML_MARSHAL_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>

#include "convert.h"
#include "slist.h"

/*
 * Marshal primitives: false on failure
 */

bool yaml_map_add_str(const char *k, const char *v, int mapping);

bool yaml_map_add_int(const char *k, const int32_t v, int mapping);

bool yaml_map_add_float(const char *k, const float v, int mapping);

bool yaml_map_add_bool(const char *k, const bool v, int mapping);

bool yaml_map_add_enum(const char *k, const int v, enum_name_fn fn_name, int mapping);

/*
 * yaml_seq_append_fn: false on failure
 */

bool yaml_seq_append_str(const void *data, int sequence);

/*
 * Marshal nodes: false on failure
 */

typedef bool (*yaml_map_populate_fn)(const void *data, const int mapping);
bool yaml_map_add_map(const char *k, const void *data, yaml_map_populate_fn fn, int mapping);

typedef bool (*yaml_seq_append_fn)(const void *data, const int sequence);
bool yaml_map_add_seq(const char *k, const struct SList *list, yaml_seq_append_fn fn, int mapping);

#endif // YAML_MARSHAL_PRIMITIVES_H

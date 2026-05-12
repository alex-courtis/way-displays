#ifndef YAML_MARSHAL_PRIMITIVES_H
#define YAML_MARSHAL_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>

#include "slist.h"


// TODO annotate all implementations
/*
 * Function pointer types
 */
typedef const char* (*yaml_enum_name_fn)(const unsigned int v);

typedef bool (*yaml_type_to_map_fn)(const void *data, const int mapping);

typedef bool (*yaml_list_to_seq_fn)(const void *list, const int sequence);

/*
 * Marshal primitives: false on failure
 */

bool yaml_map_add_str(const char *k, const char *v, int mapping);

bool yaml_map_add_int(const char *k, const int32_t v, int mapping);

bool yaml_map_add_float(const char *k, const float v, int mapping);

bool yaml_map_add_bool(const char *k, const bool v, int mapping);

bool yaml_map_add_enum(const char *k, const int v, yaml_enum_name_fn fn_name, int mapping);

bool yaml_seq_add_str(const void *data, int sequence);

/*
 * Utility: false on failure
 */

bool yaml_map_add_map(const char *k, const void *data, yaml_type_to_map_fn fn, int mapping);

bool yaml_map_add_seq(const char *k, const struct SList *list, yaml_list_to_seq_fn fn, int mapping);

#endif // YAML_MARSHAL_PRIMITIVES_H

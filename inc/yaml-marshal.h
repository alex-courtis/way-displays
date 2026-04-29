#ifndef YAML_MARSHAL_H
#define YAML_MARSHAL_H

#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

#include "slist.h"

struct MarshallingContext {
	yaml_document_t *document;
};
// TODO disambiguate
extern struct MarshallingContext ctx;

typedef bool (*map_fn)(const void *data, const int mapping);
typedef bool (*seq_fn)(const void *list, const int sequence);
typedef const char* (*enum_key_fn)(const unsigned int v);

char *marshal_yaml(const void *data, map_fn fn, const char *name);

bool map_key_to_map(const char *k, const void *data, map_fn fn, int mapping);

bool map_key_to_list(const char *k, const struct SList *list, seq_fn fn, int mapping);

bool map_key_to_str(const char *k, const char *v, int mapping);

bool map_key_to_int(const char *k, const int32_t v, int mapping);

bool map_key_to_float(const char *k, const float v, int mapping);

bool map_key_to_bool(const char *k, const bool v, int mapping);

bool map_key_to_enum(const char *k, const int v, enum_key_fn fn_name, int mapping);

bool seq_str(const void *data, int sequence);

#endif // YAML_MARSHAL_H


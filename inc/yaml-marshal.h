#ifndef YAML_MARSHAL_H
#define YAML_MARSHAL_H

#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

#include "cfg.h"
#include "ipc.h"
#include "slist.h"

struct MarshallingContext {
	yaml_document_t *document;
};
// TODO disambiguate
extern struct MarshallingContext ctx;

char *yaml_document_to_string(yaml_document_t *document);

typedef bool (*map_mapping_fn)(const void *data, int mapping);
bool map_key_to_map(const char *k, const void *data, map_mapping_fn fn, int mapping);

typedef bool (*map_list_fn)(const void *list, int sequence);
bool map_key_to_list(const char *k, const struct SList *list, map_list_fn fn, int mapping);

bool map_key_to_str(const char *k, const char *v, int mapping);

bool map_key_to_int(const char *k, const int32_t v, int mapping);

bool map_key_to_float(const char *k, const float v, int mapping);

bool map_key_to_bool(const char *k, const bool v, int mapping);

typedef const char* (*map_enum_fn_name)(unsigned int v);
bool map_key_to_enum(const char *k, const int v, map_enum_fn_name fn_name, int mapping);

bool seq_str(const void *data, int sequence);


// TODO move to own files
char *marshal_cfg_2(const struct Cfg *cfg);

bool map_cfg(const void *data, int mapping);

bool unmarshal_cfg_from_file_2(struct Cfg *cfg);

char *marshal_ipc_response_2(struct IpcOperation *operation);


char *marshal_ipc_request_2(const struct IpcRequest *request);

#endif // YAML_MARSHAL_H


#ifndef MARSHALLING_H
#define MARSHALLING_H

#include <yaml.h>

#ifdef __cplusplus
extern "C" { //}
#else
#include <stdbool.h>

#include "cfg.h"
#include "ipc.h"
#endif

char *marshal_ipc_request(struct IpcRequest *request);

struct IpcRequest *unmarshal_ipc_request(char *yaml);

// marshal globals; map for GET, sequence of maps otherwise
char *marshal_ipc_response(struct IpcOperation *operation);

// unmarshal all responses
struct SList *unmarshal_ipc_responses(const char *yaml);

char *marshal_cfg(struct Cfg *cfg);

char *yaml_document_to_string(yaml_document_t *document);

bool cfg_to_yaml_document(yaml_document_t *document, const struct Cfg * const cfg);

bool yaml_file_to_document(yaml_document_t *document, const char *file_path);

bool unmarshal_cfg_from_file(struct Cfg *cfg);

bool unmarshal_cfg_from_file_2(struct Cfg *cfg);

#if __cplusplus
} // extern "C"
#endif

#endif // MARSHALLING_H


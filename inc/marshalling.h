#ifndef MARSHALLING_H
#define MARSHALLING_H

#ifdef __cplusplus
extern "C" { //}
#else
#include <stdbool.h>

#include "cfg.h"
#include "ipc.h"
#endif

char *marshal_ipc_request(struct IpcRequest *request);

struct IpcRequest *unmarshal_ipc_request(char *yaml);

// marshal globals
char *marshal_ipc_response(struct IpcOperation *operation);

// unmarshal all responses
struct SList *unmarshal_ipc_responses(const char *yaml);

// TODO use unmarshal_ipc_responses
// unmarshal and aggregate all responses status, logging messages
struct IpcResponseStatus *unmarshal_ipc_responses_print(const char *yaml);

char *marshal_cfg(struct Cfg *cfg);

bool unmarshal_cfg_from_file(struct Cfg *cfg);

#if __cplusplus
} // extern "C"
#endif

#endif // MARSHALLING_H


#ifndef MARSHALLING_H
#define MARSHALLING_H

#ifdef __cplusplus
#include <yaml-cpp/node/node.h>
#endif

#ifdef __cplusplus
extern "C" { //}
#else
#include <stdbool.h>

#include "cfg.h"
#include "ipc.h"
#endif

char *marshal_ipc_request(struct IpcRequest *request);

struct IpcRequest *unmarshal_ipc_request(char *yaml);

char *marshal_ipc_response(struct IpcResponse *response);

struct IpcResponse *unmarshal_ipc_response(char *yaml);

char *marshal_cfg(struct Cfg *cfg);

bool unmarshal_cfg_from_file(struct Cfg *cfg);

#if __cplusplus
} // extern "C"
#endif

#endif // MARSHALLING_H


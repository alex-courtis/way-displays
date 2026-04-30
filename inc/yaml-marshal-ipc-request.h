#ifndef YAML_MARSHAL_IPC_REQUEST_H
#define YAML_MARSHAL_IPC_REQUEST_H

#include <stdbool.h>

// marshal_fn to be executed via marshal_yaml, data is IpcRequest
bool marshal_ipc_request_fn(const void *data);

#endif // YAML_MARSHAL_IPC_REQUEST_H


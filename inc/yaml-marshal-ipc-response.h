#ifndef YAML_MARSHAL_IPC_RESPONSE_H
#define YAML_MARSHAL_IPC_RESPONSE_H

#include <stdbool.h>

// marshal_fn to be executed via marshal_yaml, data is IpcOperation
bool marshal_ipc_response_fn(const void *data);

#endif // YAML_MARSHAL_IPC_RESPONSE_H


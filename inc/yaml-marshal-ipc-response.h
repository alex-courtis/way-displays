#ifndef YAML_MARSHAL_IPC_RESPONSE_H
#define YAML_MARSHAL_IPC_RESPONSE_H

#include "ipc.h"

// mutates rc
char *ipc_response_to_yaml(struct IpcOperation *ipc_operation);

#endif // YAML_MARSHAL_IPC_RESPONSE_H


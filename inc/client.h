#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>

#include "ipc.h"

int client(struct IpcRequest *ipc_request, bool yaml);

#endif // CLIENT_H


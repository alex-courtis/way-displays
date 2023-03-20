#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

#include "ipc.h"

void parse_args(int argc, char **argv, struct IpcRequest **ipc_request, char **cfg_path);

#endif // CLI_H

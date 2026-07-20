#ifndef CLI_H
#define CLI_H

#include <stdio.h>

#include "ipc.h"

// print usage message only
void cli_usage(FILE *stream);

// populate ipc_request and cfg_path, set log threshold, exits on failure
void cli_parse(int argc, char **argv, struct IpcRequest **ipc_request, char **cfg_path);

#endif // CLI_H

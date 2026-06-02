#ifndef CLI_H
#define CLI_H

#include <stdio.h>

#include "ipc.h"

void parse_args(int argc, char **argv, struct IpcRequest **ipc_request, char **cfg_path);

void usage(FILE *stream);

#endif // CLI_H

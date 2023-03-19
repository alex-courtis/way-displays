#ifndef CLI_H
#define CLI_H

#include "ipc.h"

struct IpcRequest *parse_write(int argc, char **argv);

struct IpcRequest *parse_set(int argc, char **argv);

struct IpcRequest *parse_del(int argc, char **argv);

void parse_args(int argc, char **argv, struct IpcRequest **ipc_request, char **cfg_path);

#endif // CLI_H

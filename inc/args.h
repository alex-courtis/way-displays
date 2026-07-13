#ifndef ARGS_H
#define ARGS_H

#include "cfg.h"
#include "ipc.h"
#include "log.h"

// parse a complete CLI command into a new Cfg, exits on failure
struct Cfg *args_cfg(enum IpcCommand command, enum CfgElement element, int argc, char **argv);

// parse a specific CLI command into a new IpcRequest, NULL on failure
struct IpcRequest *args_ipc_write(int argc);
struct IpcRequest *args_ipc_reapply(int argc);
struct IpcRequest *args_ipc_set(int argc, char **argv);
struct IpcRequest *args_ipc_get(int argc);
struct IpcRequest *args_ipc_list(int argc);
struct IpcRequest *args_ipc_del(int argc, char **argv);
struct IpcRequest *args_ipc_toggle(int argc, char **argv);

// parse log threshold, 0 on failure
enum LogThreshold args_log_threshold(char *optarg);

#endif // ARGS_H

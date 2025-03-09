#ifndef CLI_H
#define CLI_H

#include "cfg.h"
#include "ipc.h"

void parse_args(int argc, char **argv, struct IpcRequest **ipc_request, char **cfg_path);

//
// visible for testing
//
struct Cfg *parse_element(enum IpcCommand command, enum CfgElement element, int argc, char **argv);
struct IpcRequest *parse_write(int argc, char **argv);
struct IpcRequest *parse_set(int argc, char **argv);
struct IpcRequest *parse_del(int argc, char **argv);
enum LogThreshold parse_log_threshold(char *optarg);

#endif // CLI_H

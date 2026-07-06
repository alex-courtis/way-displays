#ifndef CLI_H
#define CLI_H

#include <stdio.h>

#include "cfg.h"
#include "ipc.h"
#include "log.h"

// print usage message only
void cli_usage(FILE *stream);

// populate ipc_request and cfg_path, set log threshold, exits on failure
void cli_parse_args(int argc, char **argv, struct IpcRequest **ipc_request, char **cfg_path);

// parse a complete CLI command into a new Cfg, exits on failure
struct Cfg *cli_parse_element(enum IpcCommand command, enum CfgElement element, int argc, char **argv);

// parse a specific CLI command into a new IpcRequest, NULL on failure
struct IpcRequest *cli_parse_write(int argc, char **argv);
struct IpcRequest *cli_parse_reapply(int argc, char **argv);
struct IpcRequest *cli_parse_set(int argc, char **argv);
struct IpcRequest *cli_parse_get(int argc, char **argv);
struct IpcRequest *cli_parse_list(int argc, char **argv);
struct IpcRequest *cli_parse_del(int argc, char **argv);
struct IpcRequest *cli_parse_toggle(int argc, char **argv);

// parse log threshold, 0 on failure
enum LogThreshold cli_parse_log_threshold(char *optarg);

#endif // CLI_H

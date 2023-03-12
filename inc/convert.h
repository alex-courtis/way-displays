#ifndef CONVERT_H
#define CONVERT_H

#include "cfg.h"
#include "ipc.h"
#include "log.h"

enum CfgElement cfg_element_val(const char *name);
const char *cfg_element_name(enum CfgElement cfg_element);

enum Arrange arrange_val_start(const char *str);
const char *arrange_name(enum Arrange arrange);

enum Align align_val_start(const char *name);
const char *align_name(enum Align align);

enum AutoScale auto_scale_val(const char *name);
const char *auto_scale_name(enum AutoScale auto_scale);

enum IpcRequestOperation ipc_request_op_val(const char *name);
const char *ipc_request_op_name(enum IpcRequestOperation ipc_request_op);
const char *ipc_request_op_friendly(enum IpcRequestOperation ipc_request_op);

enum LogThreshold log_threshold_val(const char *name);
const char *log_threshold_name(enum LogThreshold log_threshold);

#endif // CONVERT_H


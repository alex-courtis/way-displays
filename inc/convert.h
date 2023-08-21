#ifndef CONVERT_H
#define CONVERT_H

#include <wayland-client-protocol.h>

#include "cfg.h"
#include "ipc.h"
#include "log.h"

enum CfgElement cfg_element_val(const char *name);
const char *cfg_element_name(enum CfgElement cfg_element);

enum Arrange arrange_val_start(const char *str);
const char *arrange_name(enum Arrange arrange);

enum Align align_val_start(const char *name);
const char *align_name(enum Align align);

enum OnOff on_off_val(const char *name);
const char *on_off_name(enum OnOff on_off);

enum IpcCommand ipc_command_val(const char *name);
const char *ipc_command_name(enum IpcCommand ipc_command);
const char *ipc_command_friendly(enum IpcCommand ipc_command);

enum wl_output_transform output_transform_val(const char *name);
const char *output_transform_name(enum wl_output_transform output_transform);

enum LogThreshold log_threshold_val(const char *name);
const char *log_threshold_name(enum LogThreshold log_threshold);

#endif // CONVERT_H


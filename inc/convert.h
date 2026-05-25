#ifndef CONVERT_H
#define CONVERT_H

#include <wayland-client-protocol.h>

#include "cfg.h"
#include "displ.h"
#include "ipc.h"
#include "log.h"

typedef unsigned int (*enum_val_fn)(const char *name);
typedef const char* (*enum_name_fn)(unsigned int val);

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

enum wl_output_transform transform_val(const char *name);
const char *transform_name(enum wl_output_transform transform);

enum LogThreshold log_threshold_val(const char *name);
const char *log_threshold_name(enum LogThreshold log_threshold);

enum DisplState displ_state_val(const char *name);
const char *displ_state_name(enum DisplState displ_state);

enum ScaleRoundStrategy scale_round_strategy_val(const char *name);
const char *scale_round_strategy_name(enum ScaleRoundStrategy scale_round_strategy);

unsigned int scale_round_to_val(const float scale_round_to);
const char *scale_round_to_name(const unsigned int scale_round_to);

#endif // CONVERT_H


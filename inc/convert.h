#ifndef CONVERT_H
#define CONVERT_H

#include <wayland-client-protocol.h>

#include "cfg.h"
#include "conditions.h"
#include "displ.h"
#include "ipc.h"
#include "log.h"

// enum conversions enum value <-> enum key/name
// _name matches via strcasecmp
// _val_start matches strcasestr
// _friendly returns a human string instead of key

// value from key
typedef unsigned int (*enum_val_fn)(const char *name);

// key from value, static
typedef const char* (*enum_name_fn)(unsigned int val);

// pipe separated string of valid keys, caller frees
typedef char* (*enum_names_fn)(void);

enum CfgElement cfg_element_val(const char *name);
const char *cfg_element_name(enum CfgElement cfg_element);

enum Arrange arrange_val_start(const char *str);
const char *arrange_name(enum Arrange arrange);
char *arrange_names(void);

enum Align align_val_start(const char *name);
const char *align_name(enum Align align);
char *align_names(void);

enum OnOff on_off_val(const char *name);
const char *on_off_name(enum OnOff on_off);
char *on_off_names(void);

enum IpcCommand ipc_command_val(const char *name);
const char *ipc_command_name(enum IpcCommand ipc_command);
char *ipc_command_names(void);
const char *ipc_command_friendly(enum IpcCommand ipc_command);

enum wl_output_transform transform_val(const char *name);
const char *transform_name(enum wl_output_transform transform);
char *transform_names(void);

enum LogThreshold log_threshold_val(const char *name);
const char *log_threshold_name(enum LogThreshold log_threshold);
char *log_threshold_names(void);

enum DisplState displ_state_val(const char *name);
const char *displ_state_name(enum DisplState displ_state);

enum ConditionLid condition_lid_val(const char *name);
const char *condition_lid_name(enum ConditionLid condition_lid);
char *condition_lid_names(void);

enum ScaleRoundStrategy scale_round_strategy_val(const char *name);
const char *scale_round_strategy_name(enum ScaleRoundStrategy scale_round_strategy);
char *scale_round_strategy_names(void);

// not an enum however follows most semantics
unsigned int scale_round_to_val(const float scale_round_to);
const char *scale_round_to_name(const unsigned int scale_round_to);
char *scale_round_to_names(void);

#endif // CONVERT_H


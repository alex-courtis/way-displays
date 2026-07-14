#ifndef ENUM_H
#define ENUM_H

#include <wayland-client-protocol.h>

/*
 * enum conversions enum value <-> enum key/name
 * _name matches via strcasecmp
 * _val_start matches strcasestr
 * _friendly returns a human string instead of key
 */

// value from key
typedef unsigned int (*fn_enum_val)(const char *name);

// key from value, static string
typedef const char* (*fn_enum_name)(unsigned int val);

// pipe separated string of valid keys, caller frees
typedef char* (*fn_enum_names)(void);

/*
 * Cfg - wl_output_transform
 */
enum wl_output_transform transform_val(const char *name);
const char *transform_name(enum wl_output_transform transform);
char *transform_names(void);

/*
 * Cfg - Arrange
 */
enum Arrange {
	ROW = 1,
	COL,
	ARRANGE_DEFAULT = ROW,
};
enum Arrange arrange_val_start(const char *name);
const char *arrange_name(enum Arrange arrange);
char *arrange_names(void);

/*
 * Cfg - Align
 */
enum Align {
	TOP = 1,
	MIDDLE,
	BOTTOM,
	LEFT,
	RIGHT,
	ALIGN_DEFAULT = TOP,
};
enum Align align_val_start(const char *name);
const char *align_name(enum Align align);
char *align_names(void);

/*
 * Cfg - OnOff
 */
enum OnOff {
	ON = 1,
	OFF,
	SCALING_DEFAULT = ON,
	AUTO_SCALE_DEFAULT = ON,
	LAPTOP_LID_MONITOR_DEFAULT = ON,
};
enum OnOff on_off_val(const char *name);
const char *on_off_name(enum OnOff on_off);
char *on_off_names(void);

/*
 * Cfg - ConditionLid
 */
enum ConditionLid {
	LID_CLOSED = 1,
	LID_OPEN,
	LID_NOT_PRESENT,
};
enum ConditionLid condition_lid_val(const char *name);
const char *condition_lid_name(enum ConditionLid condition_lid);
char *condition_lid_names(void);

/*
 * Cfg - scaling
 */
#define AUTO_SCALE_DPI_DEFAULT 96
#define AUTO_SCALE_DPI_MIN SCALE_ROUND_TO_DEFAULT
#define AUTO_SCALE_MIN_DEFAULT 1.0f
#define AUTO_SCALE_MAX_DEFAULT -1.0f

/*
 * Cfg - ScaleRoundStrategy
 */
enum ScaleRoundStrategy {
	NEAREST = 1,
	UP,
	DOWN,
	SCALE_ROUND_STRATEGY_DEFAULT = NEAREST,
};
enum ScaleRoundStrategy scale_round_strategy_val(const char *name);
const char *scale_round_strategy_name(enum ScaleRoundStrategy scale_round_strategy);
char *scale_round_strategy_names(void);

// not an enum however follows most semantics
#define SCALE_ROUND_TO_DEFAULT 8
unsigned int scale_round_to_val(const float scale_round_to);
const char *scale_round_to_name(const unsigned int scale_round_to);
char *scale_round_to_names(void);

/*
 * Cfg - CALLBACK_CMD
 */
#define CALLBACK_CMD_DEFAULT "notify-send \"way-displays ${CALLBACK_LEVEL}\" \"${CALLBACK_MSG}\""

/*
 * Cfg - CfgElement
 */
enum CfgElement {
	ARRANGE = 1,
	ALIGN,
	ORDER,
	SCALING,
	AUTO_SCALE,
	SCALE,
	MODE,
	TRANSFORM,
	VRR_OFF,
	CALLBACK_CMD,
	LAPTOP_DISPLAY_PREFIX,
	LAPTOP_LID_MONITOR,
	MAX_PREFERRED_REFRESH,
	LOG_THRESHOLD,
	DISABLED,
	PLUGGED,
	UNPLUGGED,
	ARRANGE_ALIGN,
	AUTO_SCALE_DPI,
	AUTO_SCALE_MIN,
	AUTO_SCALE_MAX,
	SCALE_ROUND_TO,
	SCALE_ROUND_STRATEGY,
	// legacy
	CHANGE_SUCCESS_CMD,
};
enum CfgElement cfg_element_val(const char *name);
const char *cfg_element_name(enum CfgElement cfg_element);

/*
 * IPC - IpcCommand
 */
enum IpcCommand {
	GET = 1,
	LIST,
	REAPPLY,
	CFG_SET,
	CFG_DEL,
	CFG_WRITE,
	CFG_TOGGLE,
};
enum IpcCommand ipc_command_val(const char *name);
const char *ipc_command_name(enum IpcCommand ipc_command);
char *ipc_command_names(void);
const char *ipc_command_friendly(enum IpcCommand ipc_command);

/*
 * Log - LogThreshold
 */
enum LogThreshold {
	DEBUG = 1,
	INFO,
	WARNING,
	ERROR,
	FATAL,
	LOG_THRESHOLD_DEFAULT = INFO,
};
enum LogThreshold log_threshold_val(const char *name);
const char *log_threshold_name(enum LogThreshold log_threshold);
char *log_threshold_names(void);

/*
 * Head - Manual Override
 */
enum ManualOverride {
	NoOverride = 0,
	OverrideTrue,
	OverrideFalse,
};

/*
 * info - DisplState
 */
enum DisplState {
	IDLE = 0,
	SUCCEEDED,
	OUTSTANDING,
	CANCELLED,
	FAILED,
};
enum DisplState displ_state_val(const char *name);
const char *displ_state_name(enum DisplState displ_state);

/*
 * info - InfoEvent
 */
enum InfoEvent {
	ARRIVED,
	DEPARTED,
	DELTA,
	NONE,
};

#endif // ENUM_H


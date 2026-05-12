#ifndef YAML_MARSHAL_TYPES_H
#define YAML_MARSHAL_TYPES_H

#include <stdbool.h>

/*
 * yaml_map_populate_fn: false on failure
 */

// Cfg
bool yaml_map_populate_cfg(const void *cfg, int mapping);

// IpcOperation, mutates rc
bool yaml_map_populate_ipc_operation(void *ipc_operation, int mapping);

// Mode
bool yaml_map_populate_mode(const void *mode, int mapping);

// HeadState
bool yaml_map_populate_head_state(const void *head_state, int mapping);

// Head
bool yaml_map_populate_head_overrides(const void *head, int mapping);

// populates from global Lid
bool yaml_map_populate_lid(const void *unused, int mapping);

// IpcOperation, mutates rc
bool yaml_map_populate_messages(void *ipc_operation, int mapping);

// populates from global heads and lid
bool yaml_map_populate_state(const void *unused, int mapping);

/*
 * yaml_seq_append_fn: false on failure
 */

// UserScale
bool yaml_seq_append_user_scale(const void *user_scale, int sequence);

// UserMode
bool yaml_seq_append_user_mode(const void *user_mode, int sequence);

// UserTransform
bool yaml_seq_append_user_transform(const void *user_transform, int sequence);

// Condition
bool yaml_seq_append_condition(const void *condition, int sequence);

// Disabled
bool yaml_seq_append_disabled(const void *disabled, int sequence);

// Mode
bool yaml_seq_append_mode(const void *mode, int sequence);

// Head
bool yaml_seq_append_head(const void *head, int sequence);

// LogCapLine
bool yaml_seq_append_log_cap_line(const void *log_cap_line, int sequence);

#endif // YAML_MARSHAL_TYPES_H

#ifndef YAML_MARSHAL_TYPES_H
#define YAML_MARSHAL_TYPES_H

#include <stdbool.h>

/*
 * yaml_map_populate_fn: false on failure
 */

// Cfg
bool yaml_map_populate_cfg(const void *data, int mapping);

// Mode
bool yaml_map_populate_mode(const void *data, int mapping);

// HeadState
bool yaml_map_populate_head_state(const void *data, int mapping);

// Head.overrided_enabled
bool yaml_map_populate_head_overrides(const void *data, int mapping);

// Lid
bool yaml_map_populate_lid(const void *data, int mapping);

/*
 * yaml_seq_append_fn: false on failure
 */

// UserScale
bool yaml_seq_append_user_scale(const void *data, int sequence);

// UserMode
bool yaml_seq_append_user_mode(const void *data, int sequence);

// UserTransform
bool yaml_seq_append_user_transform(const void *data, int sequence);

// Condition
bool yaml_seq_append_condition(const void *data, int sequence);

// Disabled
bool yaml_seq_append_disabled(const void *data, int sequence);

// Mode
bool yaml_seq_append_mode(const void *data, int sequence);

// Head
bool yaml_seq_append_head(const void *data, int sequence);

#endif // YAML_MARSHAL_TYPES_H

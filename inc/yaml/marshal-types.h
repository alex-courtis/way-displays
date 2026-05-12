#ifndef YAML_MARSHAL_TYPES_H
#define YAML_MARSHAL_TYPES_H

#include <stdbool.h>

/*
 * Functions to add to an existing yaml_document_t from structs
 * Returns false on failure to add to the document
 */

// yaml_map_populate_fn: add pairs to an existing mapping node
bool yaml_map_populate_cfg(const void *cfg, int mapping);                   // Cfg
bool yaml_map_populate_ipc_operation(void *ipc_operation, int mapping);     // IpcOperation, mutates IpcOperation.rc
bool yaml_map_populate_mode(const void *mode, int mapping);                 // Mode
bool yaml_map_populate_head_state(const void *head_state, int mapping);     // HeadState
bool yaml_map_populate_head_overrides(const void *head, int mapping);       // Head
bool yaml_map_populate_messages(void *ipc_operation, int mapping);          // IpcOperation.log_cap_lines, mutates IpcOperation.rc
bool yaml_map_populate_state(const void *unused, int mapping);              // global: heads and lid
bool yaml_map_populate_lid(const void *unused, int mapping);                // global: lid

// yaml_seq_append_fn: create and append a new item node to an existing sequence node
bool yaml_seq_append_user_scale(const void *user_scale, int sequence);          // UserScale
bool yaml_seq_append_user_mode(const void *user_mode, int sequence);            // UserMode
bool yaml_seq_append_user_transform(const void *user_transform, int sequence);  // UserTransform
bool yaml_seq_append_condition(const void *condition, int sequence);            // Condition
bool yaml_seq_append_disabled(const void *disabled, int sequence);              // Disabled
bool yaml_seq_append_mode(const void *mode, int sequence);                      // Mode
bool yaml_seq_append_head(const void *head, int sequence);                      // Head
bool yaml_seq_append_log_cap_line(const void *log_cap_line, int sequence);      // LogCapLine

#endif // YAML_MARSHAL_TYPES_H

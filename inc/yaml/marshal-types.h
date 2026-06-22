#ifndef YAML_MARSHAL_TYPES_H
#define YAML_MARSHAL_TYPES_H

#include <stdbool.h>

#include "yaml/marshal.h"

/*
 * Functions to add to yaml_document from structs
 * Returns false on failure to add to the document
 * Returns true and does nothing when NULL data
 */

// yaml_doc_fn: create the document's contents
bool yaml_doc_cfg          (struct MC *c, const void *cfg);
bool yaml_doc_ipc_operation(struct MC *c, const void *ipc_operation);
bool yaml_doc_ipc_request  (struct MC *c, const void *ipc_request);

// yaml_map_populate_fn: add pairs to an existing mapping node
bool yaml_map_populate_cfg           (struct MC *c, const void *cfg,         int mapping);
bool yaml_map_populate_ipc_operation (struct MC *c, void *ipc_operation,     int mapping); // IpcOperation, mutates IpcOperation.rc
bool yaml_map_populate_ipc_request   (struct MC *c, const void *ipc_request, int mapping);
bool yaml_map_populate_mode          (struct MC *c, const void *mode,        int mapping);
bool yaml_map_populate_head_state    (struct MC *c, const void *head_state,  int mapping);
bool yaml_map_populate_head_overrides(struct MC *c, const void *head,        int mapping);
bool yaml_map_populate_lid           (struct MC *c, const void *lid,         int mapping);
bool yaml_map_populate_messages      (struct MC *c, void *ipc_operation,     int mapping); // IpcOperation, mutates IpcOperation.rc
bool yaml_map_populate_state         (struct MC *c, const void *g_heads,     int mapping); // g_heads and g_lid

// yaml_seq_append_val_fn: create and append a new item node to an existing sequence node
bool yaml_seq_append_user_scale    (struct MC *c, const void *user_scale,     int sequence);
bool yaml_seq_append_user_transform(struct MC *c, const void *user_transform, int sequence);
bool yaml_seq_append_condition     (struct MC *c, const void *condition,      int sequence);
bool yaml_seq_append_disabled      (struct MC *c, const void *disabled,       int sequence);
bool yaml_seq_append_mode          (struct MC *c, const void *mode,           int sequence);
bool yaml_seq_append_head          (struct MC *c, const void *head,           int sequence);
bool yaml_seq_append_log_cap_line  (struct MC *c, const void *log_cap_line,   int sequence);

// yaml_seq_append_key_val_fn: create and append a new item node to an existing sequence node
bool yaml_seq_append_user_mode(struct MC *c, const void *name_desc, const void *user_mode, int sequence);

#endif // YAML_MARSHAL_TYPES_H

#ifndef YAML_MARSHAL_TYPES_IPC_H
#define YAML_MARSHAL_TYPES_IPC_H

#include <stdbool.h>

#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "mode.h"
#include "yaml/marshal.h"

/*
 * Functions to add nodes to a yaml_document_t from structs and collections
 * NOP on failure to add to the document
 * NOP when NULL data
 */

// fn_yaml_root_from_type: populate an empty document
bool yaml_root_from_ipc_operation(struct MC *c, const struct IpcOperation* const ipc_operation);
bool yaml_root_from_ipc_request  (struct MC *c, const struct IpcRequest*   const ipc_request);

// explicitly called
int yaml_map_from_ipc_operation (struct MC *c, const struct IpcOperation* const ipc_operation);
int yaml_map_from_ipc_request   (struct MC *c, const struct IpcRequest*   const ipc_request);
int yaml_map_from_head_overrides(struct MC *c, const struct Head*         const head);
int yaml_map_from_head_state    (struct MC *c, const struct HeadState*    const head_state, const struct Head* const head);
int yaml_map_from_lid           (struct MC *c, const struct Lid*          const lid);
int yaml_seq_from_messages      (struct MC *c, const struct IpcOperation* const ipc_operation);
int yaml_map_from_state         (struct MC *c); // g_displ->heads and g_lid

// yaml_map_add_pset: fn_yaml_node_from_type
int yaml_map_from_head     (struct MC *c, const struct Head* const head);

// yaml_map_add_spmap: fn_yaml_node_from_key_type
int yaml_map_from_head_mode(struct MC *c, const void* const unused,    const struct Mode* const mode); // Head.modes

#endif // YAML_MARSHAL_TYPES_IPC_H

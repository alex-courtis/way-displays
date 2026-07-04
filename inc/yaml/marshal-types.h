#ifndef YAML_MARSHAL_TYPES_H
#define YAML_MARSHAL_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#include "cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
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
bool yaml_root_from_cfg          (struct MC *c, const struct Cfg*          const cfg);
bool yaml_root_from_ipc_operation(struct MC *c, const struct IpcOperation* const ipc_operation);
bool yaml_root_from_ipc_request  (struct MC *c, const struct IpcRequest*   const ipc_request);

// explicitly called
int yaml_map_from_cfg           (struct MC *c, const struct Cfg*          const cfg);
int yaml_map_from_ipc_operation (struct MC *c, const struct IpcOperation* const ipc_operation);
int yaml_map_from_ipc_request   (struct MC *c, const struct IpcRequest*   const ipc_request);
int yaml_map_from_head_overrides(struct MC *c, const struct Head*         const head);
int yaml_map_from_head_state    (struct MC *c, const struct HeadState*    const head_state);
int yaml_map_from_lid           (struct MC *c, const struct Lid*          const lid);
int yaml_seq_from_messages      (struct MC *c, const struct IpcOperation* const ipc_operation);
int yaml_map_from_state         (struct MC *c); // g_heads and g_lid

// yaml_map_add_pset: fn_yaml_node_from_type
int yaml_map_from_condition(struct MC *c, const struct Condition* const condition);
int yaml_node_from_disabled(struct MC *c, const struct Disabled*  const disabled);
int yaml_map_from_wlr_mode (struct MC *c, const struct WlrMode*   const wlr_mode);

// yaml_map_add_list: fn_yaml_node_from_type
int yaml_map_from_head(struct MC *c, const struct Head* const head);

// yaml_map_add_smap: fn_yaml_node_from_key_type
int yaml_map_from_user_mode(struct MC *c, const char* const name_desc, const struct WlrMode* const user_mode);

// yaml_map_add_smapi: fn_node_from_yaml_key_size_t
int yaml_map_from_scale    (struct MC *c, const char* const name_desc, const size_t scale);
int yaml_map_from_transform(struct MC *c, const char* const name_desc, const size_t transform);

#endif // YAML_MARSHAL_TYPES_H

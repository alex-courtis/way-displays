#ifndef YAML_MARSHAL_TYPES_H
#define YAML_MARSHAL_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#include "cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "cfg/user-mode.h"
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

// fn_yaml_type_to_root: populate an empty document
bool yaml_cfg_to_root          (struct MC *c, const struct Cfg*          const cfg);
bool yaml_ipc_operation_to_root(struct MC *c, const struct IpcOperation* const ipc_operation);
bool yaml_ipc_request_to_root  (struct MC *c, const struct IpcRequest*   const ipc_request);

// explicitly called
int yaml_cfg_to_map           (struct MC *c, const struct Cfg*          const cfg);
int yaml_ipc_operation_to_map (struct MC *c, const struct IpcOperation* const ipc_operation);
int yaml_ipc_request_to_map   (struct MC *c, const struct IpcRequest*   const ipc_request);
int yaml_head_overrides_to_map(struct MC *c, const struct Head*         const head);
int yaml_head_state_to_map    (struct MC *c, const struct HeadState*    const head_state);
int yaml_lid_to_map           (struct MC *c, const struct Lid*          const lid);
int yaml_messages_to_seq      (struct MC *c, const struct IpcOperation* const ipc_operation);
int yaml_mode_to_map          (struct MC *c, const struct Mode*         const mode);
int yaml_state_to_map         (struct MC *c); // g_heads and g_lid

// yaml_map_add_pset: fn_yaml_v_to_node
int yaml_condition_to_map(struct MC *c, const struct Condition* const condition);
int yaml_disabled_to_node(struct MC *c, const struct Disabled*  const disabled);

// yaml_map_add_list: fn_yaml_v_to_node
int yaml_head_to_map(struct MC *c, const struct Head* const head);

// yaml_map_add_smap: fn_yaml_kv_to_node
int yaml_user_mode_to_map(struct MC *c, const char* const name_desc, const struct UserMode* const user_mode);

// yaml_map_add_smapi: fn_yaml_ki_to_node
int yaml_scale_to_map    (struct MC *c, const char* const name_desc, const size_t scale);
int yaml_transform_to_map(struct MC *c, const char* const name_desc, const size_t transform);

#endif // YAML_MARSHAL_TYPES_H

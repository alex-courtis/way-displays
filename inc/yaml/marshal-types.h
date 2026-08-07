#ifndef YAML_MARSHAL_TYPES_H
#define YAML_MARSHAL_TYPES_H

#include <stdbool.h>

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "mode.h"
#include "simap.h"
#include "spmap.h"

/*
 * Functions to add nodes to a yaml_document_t from structs and collections
 * NOP on failure to add to the document
 * NOP when NULL data
 */

// fn_yaml_root_from_type: populate an empty document
bool yaml_root_from_cfg          (const struct Cfg*          const cfg);
bool yaml_root_from_ipc_operation(const struct IpcOperation* const ipc_operation);
bool yaml_root_from_ipc_request  (const struct IpcRequest*   const ipc_request);

// explicitly called
int yaml_map_from_ipc_operation (const struct IpcOperation* const ipc_operation);
int yaml_map_from_ipc_request   (const struct IpcRequest*   const ipc_request);
int yaml_map_from_cfg           (const struct Cfg*          const cfg);
int yaml_map_from_cfg_modes     (const struct SPmap*        const modes);
int yaml_map_from_disableds     (const struct SPmap*        const disableds);
int yaml_map_from_head_overrides(const struct Head*         const head);
int yaml_map_from_head_state    (const struct HeadState*    const head_state, const struct Head* const head);
int yaml_map_from_lid           (const struct Lid*          const lid);
int yaml_map_from_scales        (const struct SImap*        const scales);
int yaml_map_from_state         (void);                                                    // g_displ->heads and g_lid
int yaml_map_from_transforms    (const struct SImap*        const transforms);
int yaml_seq_from_messages      (const struct IpcOperation* const ipc_operation);

// fn_yaml_node_from_type: called for each item in a collection
int yaml_map_from_condition(const struct CfgCondition* const condition);
int yaml_map_from_head     (const struct Head*         const head);
int yaml_map_from_head_mode(const struct Mode*         const mode);

#endif // YAML_MARSHAL_TYPES_H

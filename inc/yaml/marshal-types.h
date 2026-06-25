#ifndef YAML_MARSHAL_TYPES_H
#define YAML_MARSHAL_TYPES_H

#include <stdbool.h>

#include "cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "cfg/user-mode.h"
#include "cfg/user-scale.h"
#include "cfg/user-transform.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "yaml/marshal.h"

/*
 * Functions to add to yaml_document from structs
 * Returns false on failure to add to the document
 * Returns true and does nothing when NULL data
 */

// fn_yaml_doc: create the document's contents
bool yaml_doc_cfg          (struct MC *c, const struct Cfg*          const cfg);
bool yaml_doc_ipc_operation(struct MC *c,       struct IpcOperation* const ipc_operation); // yaml_map_populate_messages mutates IpcOperation.rc
bool yaml_doc_ipc_request  (struct MC *c, const struct IpcRequest*   const ipc_request);

// fn_yaml_map_pop: add pairs to an existing mapping node
bool yaml_map_populate_cfg           (struct MC *c, const struct Cfg*          const cfg,           int mapping);
bool yaml_map_populate_ipc_operation (struct MC *c,       struct IpcOperation* const ipc_operation, int mapping); // yaml_map_populate_messages mutates IpcOperation.rc
bool yaml_map_populate_ipc_request   (struct MC *c, const struct IpcRequest*   const ipc_request,   int mapping);
bool yaml_map_populate_mode          (struct MC *c, const struct Mode*         const mode,          int mapping);
bool yaml_map_populate_head_state    (struct MC *c, const struct HeadState*    const head_state,    int mapping);
bool yaml_map_populate_head_overrides(struct MC *c, const struct Head*         const head,          int mapping);
bool yaml_map_populate_lid           (struct MC *c, const struct Lid*          const lid,           int mapping);
bool yaml_map_populate_messages      (struct MC *c,       struct IpcOperation* const ipc_operation, int mapping); // mutates IpcOperation.rc
bool yaml_map_populate_state         (struct MC *c, const        void*         const unused,        int mapping); // g_heads and g_lid

// fn_yaml_seq_app_v: create and append a new item node to an existing sequence node
bool yaml_seq_append_condition     (struct MC *c, const struct Condition*  const condition,   int sequence);
bool yaml_seq_append_disabled      (struct MC *c, const struct Disabled*   const disabled,    int sequence);
bool yaml_seq_append_mode          (struct MC *c, const struct Mode*       const mode,        int sequence);
bool yaml_seq_append_head          (struct MC *c, const struct Head*       const head,        int sequence);
bool yaml_seq_append_log_cap_line  (struct MC *c, const struct LogCapLine* const log_cap_line,int sequence);

// fn_yaml_seq_app_kv: create and append a new item node to an existing sequence node
bool yaml_seq_append_user_mode     (struct MC *c, const char* const name_desc, const struct UserMode*      const user_mode,      int sequence);
bool yaml_seq_append_user_scale    (struct MC *c, const char* const name_desc, const struct UserScale*     const user_scale,     int sequence);
bool yaml_seq_append_user_transform(struct MC *c, const char* const name_desc, const struct UserTransform* const user_transform, int sequence);

#endif // YAML_MARSHAL_TYPES_H

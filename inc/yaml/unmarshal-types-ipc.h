#ifndef YAML_UNMARSHAL_TYPES_IPC_H
#define YAML_UNMARSHAL_TYPES_IPC_H

#include <yaml.h>

#include "head.h"
#include "plist.h"
#include "ppmap.h"
#include "yaml/unmarshal.h"

/*
 * Functions to extract structs from yaml_document
 * Returns NULL or false and logs on failure
 */

// fn_yaml_root_to_type: create a struct from the document root
void *yaml_root_to_ipc_request       (struct UC *c, const yaml_node_t *root); // IpcRequest
void *yaml_root_to_ipc_response_plist(struct UC *c, const yaml_node_t *root); // Plist of IpcResponse

// create a struct from a map
struct Lid  *yaml_map_to_lid      (struct UC *c, const yaml_node_t *map);  // Lid
struct Mode *yaml_map_to_head_mode(struct UC *c, const yaml_node_t *map);  // Head mode

// fn_yaml_node_into_col: create a struct and add to collection
void yaml_map_into_ipc_responses(struct UC *c, const struct Plist* const ipc_responses, const yaml_node_t *map);
void yaml_map_into_heads        (struct UC *c, const struct Plist* const heads,         const yaml_node_t *map);
void yaml_map_into_head_modes   (struct UC *c, const struct PPmap* const modes,         const yaml_node_t *map);
void yaml_map_into_log_cap_lines(struct UC *c, const struct Plist* const log_cap_lines, const yaml_node_t *map);

// into an existing HeadState struct
void yaml_map_into_head_state(struct UC *c, struct HeadState *head_state, const struct Head * const head, const yaml_node_t *map);

#endif // YAML_UNMARSHAL_TYPES_IPC_H


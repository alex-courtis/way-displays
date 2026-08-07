#ifndef YAML_UNMARSHAL_TYPES_H
#define YAML_UNMARSHAL_TYPES_H

#include <yaml.h>

#include "head.h"
#include "plist.h"
#include "ppmap.h"
#include "pset.h"
#include "simap.h"
#include "spmap.h"
#include "sset.h"
#include "yaml/unmarshal.h"

/*
 * Functions to extract structs from yaml_document
 * Returns NULL or false and logs on failure
 */

// fn_yaml_root_to_type: create a struct from the document root
void *yaml_root_to_cfg               (const yaml_node_t *root); // Cfg
void *yaml_root_to_ipc_request       (const yaml_node_t *root); // IpcRequest
void *yaml_root_to_ipc_response_plist(const yaml_node_t *root); // Plist of IpcResponse

// create a struct from a map

struct Cfg  *yaml_map_to_cfg      (const yaml_node_t *map);  // Cfg
struct Lid  *yaml_map_to_lid      (const yaml_node_t *map);  // Lid
struct Mode *yaml_map_to_cfg_mode (const yaml_node_t *map);  // Cfg mode
struct Mode *yaml_map_to_head_mode(const yaml_node_t *map);  // Head mode

// fn_yaml_node_into_col: create a struct and add to collection
void yaml_map_into_conditions   (const struct Pset*  const conditions,    const yaml_node_t *map);
void yaml_map_into_head_modes   (const struct PPmap* const modes,         const yaml_node_t *map);
void yaml_map_into_heads        (const struct Plist* const heads,         const yaml_node_t *map);
void yaml_map_into_ipc_responses(const struct Plist* const ipc_responses, const yaml_node_t *map);
void yaml_map_into_log_cap_lines(const struct Plist* const log_cap_lines, const yaml_node_t *map);

// into an existing map
void yaml_map_into_cfg_modes (const struct SPmap* const modes,      const yaml_node_t *map);
void yaml_map_into_disableds (const struct SPmap* const disableds,  const yaml_node_t *map);
void yaml_map_into_scales    (const struct SImap* const scales,     const yaml_node_t *map);
void yaml_map_into_transforms(const struct SImap* const transforms, const yaml_node_t *map);

// into an existing HeadState struct
void yaml_map_into_head_state(struct HeadState *head_state, const struct Head * const head, const yaml_node_t *map);

// unmarshal a scalar to a name_desc, validating regex
char *yaml_scalar_to_name_desc(const yaml_node_t *scalar);

// unmarshal a scalar float to a scale_round_to
unsigned int yaml_scalar_to_scale_round_to(const yaml_node_t *scalar);

// unmarshal a sequence of valid name_desc, removing duplicates and validating regex
void yaml_seq_into_name_desc_sset(const struct Sset* const sset, const yaml_node_t *seq);

#endif // YAML_UNMARSHAL_TYPES_H


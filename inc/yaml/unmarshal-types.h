#ifndef YAML_UNMARSHAL_TYPES_H
#define YAML_UNMARSHAL_TYPES_H

#include <stdbool.h>
#include <yaml.h>

#include "head.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "smapi.h"
#include "sset.h"
#include "yaml/unmarshal.h"

/*
 * Functions to extract structs from yaml_document
 * Returns NULL or false and logs on failure
 */

// fn_yaml_root_to: create a struct from the document root
void *yaml_root_to_cfg              (struct UC *c, const yaml_node_t *root); // Cfg
void *yaml_root_to_ipc_request      (struct UC *c, const yaml_node_t *root); // IpcRequest
void *yaml_root_to_ipc_response_list(struct UC *c, const yaml_node_t *root); // list of IpcResponse

// create a struct from a map
struct Cfg  *yaml_map_to_cfg (struct UC *c, const yaml_node_t *map);  // Cfg
struct Lid  *yaml_map_to_lid (struct UC *c, const yaml_node_t *map);  // Lid
struct Mode *yaml_map_to_mode(struct UC *c, const yaml_node_t *map);  // Mode

// fn_yaml_node_into_col: create a struct and add to collection
void yaml_map_into_ipc_responses  (struct UC *c, struct SList **ipc_responses, const yaml_node_t *map);
void yaml_map_into_heads          (struct UC *c, struct SList **heads,         const yaml_node_t *map);
void yaml_map_into_modes          (struct UC *c, struct SList **modes,         const yaml_node_t *map);
void yaml_map_into_log_cap_lines  (struct UC *c, struct SList **log_cap_lines, const yaml_node_t *map);
void yaml_map_into_conditions     (struct UC *c, const struct PSet* const conditions,       const yaml_node_t *map);
void yaml_map_into_user_scales    (struct UC *c, const struct SMapI* const user_scales,     const yaml_node_t *map);
void yaml_map_into_user_modes     (struct UC *c, const struct SMap* const user_modes,       const yaml_node_t *map);
void yaml_map_into_user_transforms(struct UC *c, const struct SMapI* const user_transforms, const yaml_node_t *map);
void yaml_node_into_disableds     (struct UC *c, const struct PSet* const disableds,        const yaml_node_t *node); // scalar or map

// into HeadState
bool yaml_map_to_head_state(struct UC *c, struct HeadState *head_state, const yaml_node_t *map);

// unmarshal a scalar to a name_desc, validating regex
char *yaml_scalar_to_name_desc(struct UC *c, const yaml_node_t *scalar);

// unmarshal a scalar float to a scale_round_to
unsigned int yaml_scalar_to_scale_round_to(struct UC *c, const yaml_node_t *scalar);

// unmarshal a sequence of valid name_desc, removing duplicates and validating regex
void yaml_seq_into_name_desc_sset(struct UC *c, const struct SSet *sset, const yaml_node_t *seq);

#endif // YAML_UNMARSHAL_TYPES_H


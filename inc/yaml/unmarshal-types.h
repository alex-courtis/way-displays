#ifndef YAML_UNMARSHAL_TYPES_H
#define YAML_UNMARSHAL_TYPES_H

#include <yaml.h>

#include "head.h"
#include "ppmap.h"
#include "pset.h"
#include "pslist.h"
#include "simap.h"
#include "spmap.h"
#include "sset.h"
#include "yaml/unmarshal.h"

/*
 * Functions to extract structs from yaml_document
 * Returns NULL or false and logs on failure
 */

// fn_yaml_root_to_type: create a struct from the document root
void *yaml_root_to_cfg              (struct UC *c, const yaml_node_t *root); // Cfg
void *yaml_root_to_ipc_request      (struct UC *c, const yaml_node_t *root); // IpcRequest
void *yaml_root_to_ipc_response_pset(struct UC *c, const yaml_node_t *root); // Pset of IpcResponse

// create a struct from a map
struct Cfg  *yaml_map_to_cfg (struct UC *c, const yaml_node_t *map);  // Cfg
struct Lid  *yaml_map_to_lid (struct UC *c, const yaml_node_t *map);  // Lid
struct Mode *yaml_map_to_mode(struct UC *c, const yaml_node_t *map);  // Mode

// fn_yaml_node_into_col: create a struct and add to collection
void yaml_map_into_ipc_responses(struct UC *c, const struct Pset *ipc_responses,     const yaml_node_t *map);
void yaml_map_into_heads        (struct UC *c, const struct Pset *heads,             const yaml_node_t *map);
void yaml_map_into_modes        (struct UC *c, const struct PPmap *modes,            const yaml_node_t *map);
void yaml_map_into_log_cap_lines(struct UC *c, struct Pslist **log_cap_lines,        const yaml_node_t *map);
void yaml_map_into_conditions   (struct UC *c, const struct Pset* const conditions,  const yaml_node_t *map);
void yaml_map_into_scales       (struct UC *c, const struct SImap* const scales,     const yaml_node_t *map);
void yaml_map_into_named_modes  (struct UC *c, const struct SPmap* const modes,      const yaml_node_t *map);
void yaml_map_into_transforms   (struct UC *c, const struct SImap* const transforms, const yaml_node_t *map);
void yaml_node_into_disableds   (struct UC *c, const struct Pset* const disableds,   const yaml_node_t *node); // scalar or map

// into an existing HeadState struct
void yaml_map_into_head_state(struct UC *c, struct HeadState *head_state, const struct Head * const head, const yaml_node_t *map);

// unmarshal a scalar to a name_desc, validating regex
char *yaml_scalar_to_name_desc(struct UC *c, const yaml_node_t *scalar);

// unmarshal a scalar float to a scale_round_to
unsigned int yaml_scalar_to_scale_round_to(struct UC *c, const yaml_node_t *scalar);

// unmarshal a sequence of valid name_desc, removing duplicates and validating regex
void yaml_seq_into_name_desc_sset(struct UC *c, const struct Sset *sset, const yaml_node_t *seq);

#endif // YAML_UNMARSHAL_TYPES_H


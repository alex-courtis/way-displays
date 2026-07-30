#ifndef YAML_UNMARSHAL_TYPES_CFG_H
#define YAML_UNMARSHAL_TYPES_CFG_H

#include <yaml.h>

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
void *yaml_root_to_cfg               (struct UC *c, const yaml_node_t *root); // Cfg

// create a struct from a map
struct Cfg  *yaml_map_to_cfg      (struct UC *c, const yaml_node_t *map);  // Cfg
struct Mode *yaml_map_to_cfg_mode (struct UC *c, const yaml_node_t *map);  // Cfg mode

// fn_yaml_node_into_col: create a struct and add to collection
void yaml_map_into_conditions   (struct UC *c, const struct Pset*  const conditions,    const yaml_node_t *map);
void yaml_map_into_scales_v1    (struct UC *c, const struct SImap* const scales,        const yaml_node_t *map);
void yaml_map_into_cfg_modes_v1 (struct UC *c, const struct SPmap* const modes,         const yaml_node_t *map);
void yaml_map_into_transforms_v1(struct UC *c, const struct SImap* const transforms,    const yaml_node_t *map);
void yaml_node_into_disableds   (struct UC *c, const struct Pset*  const disableds,     const yaml_node_t *node); // scalar or map

// called directly
void yaml_map_into_scales       (struct UC *c, const struct SImap* const scales,        const yaml_node_t *map);
void yaml_map_into_cfg_modes    (struct UC *c, const struct SPmap* const modes,         const yaml_node_t *map);
void yaml_map_into_transforms   (struct UC *c, const struct SImap* const transforms,    const yaml_node_t *map);

// unmarshal a scalar to a name_desc, validating regex
char *yaml_scalar_to_name_desc(struct UC *c, const yaml_node_t *scalar);

// unmarshal a scalar float to a scale_round_to
unsigned int yaml_scalar_to_scale_round_to(struct UC *c, const yaml_node_t *scalar);

// unmarshal a sequence of valid name_desc, removing duplicates and validating regex
void yaml_seq_into_name_desc_sset(struct UC *c, const struct Sset* const sset, const yaml_node_t *seq);

#endif // YAML_UNMARSHAL_TYPES_CFG_H


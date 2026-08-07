#ifndef YAML_UNMARSHAL_TYPES_V1_H
#define YAML_UNMARSHAL_TYPES_V1_H

#include <yaml.h>

#include "simap.h"
#include "spmap.h"
#include "yaml/unmarshal.h"

extern const char *v1_deprecation_log_text;
extern const char *v1_deprecation_callback_text;

/*
 * schema v1.2.0 compatibility
 */

// put the deprecated field into context for later processing
void yaml_scalar_into_laptop_display_prefix_v1(const yaml_node_t *node);

// fn_yaml_node_into_col: create a struct and add to collection
void yaml_map_into_cfg_modes_v1 (const struct SPmap* const modes,      const yaml_node_t *map);
void yaml_map_into_scales_v1    (const struct SImap* const scales,     const yaml_node_t *map);
void yaml_map_into_transforms_v1(const struct SImap* const transforms, const yaml_node_t *map);
void yaml_node_into_disableds_v1(const struct SPmap* const disableds,  const yaml_node_t *node); // scalar or map

#endif // YAML_UNMARSHAL_TYPES_V1_H


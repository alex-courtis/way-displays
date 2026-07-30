#ifndef YAML_MARSHAL_TYPES_CFG_H
#define YAML_MARSHAL_TYPES_CFG_H

#include <stdbool.h>

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "simap.h"
#include "spmap.h"
#include "yaml/marshal.h"

/*
 * Functions to add nodes to a yaml_document_t from structs and collections
 * NOP on failure to add to the document
 * NOP when NULL data
 */

// fn_yaml_root_from_type: populate an empty document
bool yaml_root_from_cfg          (struct MC *c, const struct Cfg*          const cfg);

// explicitly called
int yaml_map_from_cfg           (struct MC *c, const struct Cfg*          const cfg);
int yaml_map_from_cfg_modes     (struct MC *c, const struct SPmap*        const modes);      // Cfg.modes
int yaml_map_from_scales        (struct MC *c, const struct SImap*        const scales);
int yaml_map_from_transforms    (struct MC *c, const struct SImap*        const transforms);

// yaml_map_add_pset: fn_yaml_node_from_type
int yaml_map_from_condition(struct MC *c, const struct CfgCondition* const condition);
int yaml_node_from_disabled(struct MC *c, const struct CfgDisabled*  const disabled);

#endif // YAML_MARSHAL_TYPES_CFG_H

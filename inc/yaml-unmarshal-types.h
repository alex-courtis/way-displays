#ifndef YAML_UNMARSHAL_TYPES_H
#define YAML_UNMARSHAL_TYPES_H

#include <stdbool.h>
#include <yaml.h>

#include "cfg.h"
#include "head.h"

/*
 * Cfg
 */

// TODO try and generif all seq_to__list

// unmarshal Condition
// struct Condition *map_to_condition(const yaml_node_t *map);
// struct SList *seq_to_conditions_list(const yaml_node_t *seq);

// unmarshal Disabled
// struct Disabled *map_to_disabled(const yaml_node_t *map);
// struct SList *seq_to_disabled_list(const yaml_node_t *seq);

// unmarshal UserScale
// struct UserScale *map_to_user_scale(const yaml_node_t *map);
// struct SList *seq_to_user_scale_list(const yaml_node_t *seq);

// unmarshal UserMode
// struct UserMode *map_to_user_mode(const yaml_node_t *map);
// struct SList *seq_to_user_mode_list(const yaml_node_t *seq);

// unmarshal UserTransform
// struct UserTransform *map_to_user_transform(const yaml_node_t *map);
// struct SList *seq_to_user_transform_list(const yaml_node_t *seq);

// unmarshal a CALLBACK_CMD, frees first, sets NULL on empty string, otherwise default
// void scalar_to_callback_cmd(char **dst, const yaml_node_t *scalar);

// unmarshal into existing Cfg
bool map_to_cfg(struct Cfg *cfg, const yaml_node_t *map);

/*
 * IPC
 */

// unmarshal Lid
struct Lid *map_to_lid(const yaml_node_t *map);

// unmarshal Mode
struct Mode *map_to_mode(const yaml_node_t *map);
struct SList *seq_to_mode_list(const yaml_node_t *seq);

// unmarshal HeadState
void map_to_head_state(struct HeadState *head_state, const yaml_node_t *map);

// unmarshal Head
struct Head *map_to_head(const yaml_node_t *map);
struct SList *seq_to_head_list(const yaml_node_t *seq);

// unmarshal LogCapLine
struct SList *seq_to_log_cap_lines(const yaml_node_t *seq);

#endif // YAML_UNMARSHAL_TYPES_H


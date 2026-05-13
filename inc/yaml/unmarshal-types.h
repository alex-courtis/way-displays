#ifndef YAML_UNMARSHAL_TYPES_H
#define YAML_UNMARSHAL_TYPES_H

#include <stdbool.h>
#include <yaml.h>

#include "cfg.h"
#include "head.h"

/*
 * Functions to extract structs from yaml_document
 * Returns NULL or false and logs on failure
 */

// yaml_node_to_type_fn: create a struct from a node
void *map_to_condition(const yaml_node_t *map);      // Condition
void *map_to_user_scale(const yaml_node_t *map);     // UserScale
void *map_to_user_mode(const yaml_node_t *map);      // UserMode
void *map_to_user_transform(const yaml_node_t *map); // UserTransform
void *map_to_lid(const yaml_node_t *map);            // Lid
void *map_to_mode(const yaml_node_t *map);           // Mode
void *map_to_head(const yaml_node_t *map);           // Head
void *node_to_disabled(const yaml_node_t *node);     // Disabled

// into Cfg
bool map_to_cfg(struct Cfg *cfg, const yaml_node_t *map);

// into HeadState
bool map_to_head_state(struct HeadState *head_state, const yaml_node_t *map);

// into Head.callback_cmd, frees first, sets NULL on empty string, otherwise default
void scalar_to_callback_cmd(char **dst, const yaml_node_t *scalar);

// create LogCapLine list
struct SList *seq_to_log_cap_lines(const yaml_node_t *seq);

// unmarshal a scalar to a name_desc, validating regex
char *yaml_scalar_to_name_desc(const yaml_node_t *scalar);

// unmarshal a sequence of valid name_desc, removing duplicates and validating regex
struct SList *yaml_seq_to_name_desc_list(const yaml_node_t *seq);

#endif // YAML_UNMARSHAL_TYPES_H


#ifndef YAML_UNMARSHAL_H
#define YAML_UNMARSHAL_H

#include <yaml.h>

// Unmarshal a yaml document string
// name is arbitrary and used for logging
// Returns NULL and logs on failure
typedef void *(*yaml_root_to_type_fn)(const yaml_node_t *root);
void *yaml_unmarshal_str(const char *yaml, yaml_root_to_type_fn fn, char *name);

#endif // YAML_UNMARSHAL_H

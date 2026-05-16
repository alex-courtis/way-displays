#ifndef YAML_UNMARSHAL_H
#define YAML_UNMARSHAL_H

#include <yaml.h>

/*
 * Unmarshal functions
 * Returns NULL and logs on failure
 */

typedef void *(*yaml_root_to_type_fn)(const yaml_node_t *root);

// Unmarshal a yaml string, human is arbitrary and used for logging
void *yaml_unmarshal_str(const char *yaml, yaml_root_to_type_fn fn, char *human);

// Unmarshal a yaml file
void *yaml_unmarshal_file(const char *path, yaml_root_to_type_fn fn);

#endif // YAML_UNMARSHAL_H

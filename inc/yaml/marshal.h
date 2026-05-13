#ifndef YAML_MARSHAL_H
#define YAML_MARSHAL_H

#include <stdbool.h>
#include <yaml.h>

// TODO explain context lifecycles

// global, set and unset by struct_to_yaml
extern struct MarshalCtx {
	yaml_document_t *doc;
} marshal_ctx;

// Create a new yaml document and render as a string
// Contents are populated by evaluating fn on data
typedef bool (*yaml_marshal_fn)(const void *data);
char *yaml_marshal(const void *data, yaml_marshal_fn fn, const char *name);

#endif // YAML_MARSHAL_H

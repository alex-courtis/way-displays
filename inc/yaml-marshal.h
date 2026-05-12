#ifndef YAML_MARSHAL_H
#define YAML_MARSHAL_H

#include <stdbool.h>
#include <yaml.h>

// TODO compress into one header
// TODO use static doc

// global, set and unset by struct_to_yaml
extern struct MarshalCtx {
	yaml_document_t *doc;
} marshal_ctx;

// marshal data to a yaml string via fn, logs use name
typedef bool (*struct_to_yaml_fn)(const void *data);
char *struct_to_yaml(const void *data, struct_to_yaml_fn fn, const char *name);

#endif // YAML_MARSHAL_H


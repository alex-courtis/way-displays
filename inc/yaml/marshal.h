#ifndef YAML_MARSHAL_H
#define YAML_MARSHAL_H

#include <stdbool.h>

// TODO collapse (un)marshal into one header, maybe interface.h

// Marshal a yaml document and render as a string
// Contents are populated by evaluating fn on data
// name is arbitrary and used for logging
// Returns NULL and logs on failure
typedef bool (*yaml_marshal_fn)(const void *data);
char *yaml_marshal(const void *data, yaml_marshal_fn fn, const char *name);

#endif // YAML_MARSHAL_H

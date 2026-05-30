#ifndef YAML_MARSHAL_H
#define YAML_MARSHAL_H

#include <stdbool.h>
#include <yaml.h>

/*
 * Context available for the duration of marshalling
 */
struct MC {
	yaml_document_t d;
};

/*
 * Unmarshal functions
 * Returns NULL and logs on failure
 */

typedef bool (*yaml_marshal_fn)(struct MC *c, const void *data);

// Marshal a yaml document and render it as a string, human is arbitrary and used for logging
char *yaml_marshal(const void *data, yaml_marshal_fn fn, const char *human);

#endif // YAML_MARSHAL_H

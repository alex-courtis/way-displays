#ifndef YAML_MARSHAL_H
#define YAML_MARSHAL_H

#include <stdbool.h>
#include <stddef.h>
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

// Marshal a yaml document and render it as a string, human is arbitrary and used for logging
typedef bool (*fn_yaml_root_from_type)(struct MC *c, const void *data);
char *yaml_marshal(const void *data, fn_yaml_root_from_type fn, const char *human);

// yaml_write_handler_t
int yaml_write_handler(void *data, const unsigned char* const buffer, const size_t size);

#endif // YAML_MARSHAL_H

#ifndef YAML_INTERFACE_H
#define YAML_INTERFACE_H

#include <yaml.h>

// yaml document used for all operations, not thread safe
// set/reset by yaml_marshal TODO unmarshalling functions
extern yaml_document_t *yaml_document;

#endif // YAML_INTERFACE_H

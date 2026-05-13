#ifndef YAML_CONTEXT_H
#define YAML_CONTEXT_H

#include <yaml.h>

// yaml document used for all operations, not thread safe
// set during yaml_marshal and yaml_unmarshal_*
extern yaml_document_t *yaml_document;

#endif // YAML_CONTEXT_H

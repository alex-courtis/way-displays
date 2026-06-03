#ifndef WRAP_LIBYAML_H
#define WRAP_LIBYAML_H

#include <stdbool.h>

// one shots to have the function fail, returning 0, otherwise call the real
//
extern bool yaml_document_initialize__fail;

extern bool yaml_emitter_initialize__fail;
extern bool yaml_emitter_open__fail;
extern bool yaml_emitter_dump__fail;
extern bool yaml_emitter_close__fail;

extern bool yaml_parser_initialize__fail;

// reset all the above
void reset_yaml_fails(void);

#endif // WRAP_LIBYAML_H

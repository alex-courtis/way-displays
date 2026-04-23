#include <stdbool.h>

// one shots to have the function fail, returning 0, otherwise call the real
//
extern bool yaml_document_initialize__fail;
extern bool yaml_document_add_mapping__fail;

extern bool yaml_emitter_initialize__fail;
extern bool yaml_emitter_open__fail;
extern bool yaml_emitter_dump__fail;
extern bool yaml_emitter_close__fail;

// reset all the above
void reset_yaml_fails(void);

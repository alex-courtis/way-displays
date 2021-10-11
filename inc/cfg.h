#ifndef CFG_H
#define CFG_H

#include "types.h"

#ifdef __cplusplus
extern "C" { //}
#endif

void print_cfg(struct Cfg *cfg);

struct Cfg *load_cfg();

struct Cfg *reload_cfg(struct Cfg *cfg);

#if __cplusplus
} // extern "C"
#endif

#endif // CFG_H


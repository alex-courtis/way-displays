#ifndef YAML_MARSHAL_CFG_H
#define YAML_MARSHAL_CFG_H

#include <stdbool.h>

#include "cfg.h"

char *marshal_cfg_2(const struct Cfg *cfg);

bool map_cfg(const void *data, int mapping);

#endif // YAML_MARSHAL_CFG_H


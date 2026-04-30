#ifndef YAML_MARSHAL_CFG_H
#define YAML_MARSHAL_CFG_H

#include <stdbool.h>

// marshal_fn to be executed via marshal_yaml, data is Cfg
bool marshal_cfg_fn(const void *data);

bool map_cfg(const void *data, int mapping);

#endif // YAML_MARSHAL_CFG_H


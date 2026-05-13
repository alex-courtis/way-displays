#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>

#include "yaml-marshal-cfg.h"

#include "cfg.h"
#include "log.h"
#include "yaml-marshal.h"
#include "yaml/marshal-types.h"

char *cfg_to_yaml(const struct Cfg *cfg) {
	return struct_to_yaml(cfg, yaml_doc_cfg, "cfg");
}

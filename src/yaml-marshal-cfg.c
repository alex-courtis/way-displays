#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>

#include "yaml-marshal-cfg.h"

#include "cfg.h"
#include "log.h"
#include "yaml-marshal.h"
#include "yaml/marshal-types.h"

static bool marshal_cfg_fn(const void *data) {
	if (!data)
		return false;

	int mapping = yaml_document_add_mapping(marshal_ctx.doc, NULL, YAML_BLOCK_MAPPING_STYLE);
	if (!mapping) {
		log_error("unable to marshal cfg: yaml_document_add_mapping for root failed");
		return false;
	}

	return yaml_map_populate_cfg(data, mapping);
}

char *cfg_to_yaml(const struct Cfg *cfg) {
	return struct_to_yaml(cfg, marshal_cfg_fn, "cfg");
}

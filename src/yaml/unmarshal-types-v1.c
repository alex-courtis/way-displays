#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <wayland-client-protocol.h>
#include <yaml.h>

#include "yaml/unmarshal-types-v1.h"

#include "cfg/disabled.h"
#include "enum.h"
#include "info/callback.h"
#include "info/print.h"
#include "log.h"
#include "mode.h"
#include "simap.h"
#include "spmap.h"
#include "yaml/unmarshal-primitives.h"
#include "yaml/unmarshal-types.h"
#include "yaml/unmarshal.h"

const char *v1_deprecation_log_text = "Deprecation:\n"
"way-displays 2.0 has deprecated some arrays, replacing NAME_DESC with keyed maps.\n"
"\n"
"This affects: DISABLED, MODE, SCALE and TRANSFORM\n"
"e.g.\n"
"\n"
"  SCALE:\n"
"  - NAME_DESC: DP-2\n"
"    SCALE: 1.5\n"
"\n"
"is now expressed as\n"
"\n"
"  SCALE:\n"
"    'DP-2': 1.5\n"
"This is a non-breaking change however compatibility will be removed in version 3.0\n"
"\n"
"See https://github.com/alex-courtis/way-displays/wiki/Version-2.0.0-Changes for complete details.\n"
"\n"
"Please way-displays --write to upgrade your cfg.yaml\n"
;

const char *v1_deprecation_callback_text =
"Deprecation:\n"
"NAME_DESC arrays are replaced with keyed maps.\n"
"See logs or https://github.com/alex-courtis/way-displays/wiki/Version-2.0.0-Changes\n"
"\n"
"Please way-displays --write to upgrade your cfg.yaml\n"
;

static void yaml_unmarshal_log_v1(struct UC *c) {
	// warn once only
	if (!c->v1_warn)
		return;

	print_v1_deprecation();
	callback_v1_deprecation();

	c->v1_warn = false;
}

void yaml_map_into_cfg_modes_v1(struct UC *c, const struct SPmap* const modes, const yaml_node_t *map) {
	yaml_unmarshal_log_v1(c);

	const struct SPmap *m;
	if (!modes || !(m = yaml_map_to_spmap(c, map)))
		return;

	struct Mode *mode = NULL;

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	const yaml_node_t *scalar = spmap_get(m, "NAME_DESC");
	if (!(name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto err;

	yaml_unmarshal_log_ctx_name_desc(c, name_desc);

	mode = yaml_map_to_cfg_mode(c, map);
	if (!mode)
		goto err;

	if (spmap_put_if_absent(modes, name_desc, mode)) {
		log_warn("Removing duplicate MODE %s", name_desc);
		goto err;
	}

	goto end;

err:
	mode_free(mode);

end:
	free(name_desc);
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);
}

void yaml_map_into_scales_v1(struct UC *c, const struct SImap* const scales, const yaml_node_t *map) {
	yaml_unmarshal_log_v1(c);

	const struct SPmap *m;
	if (!scales || !(m = yaml_map_to_spmap(c, map)))
		return;

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	const yaml_node_t *scalar = spmap_get(m, "NAME_DESC");
	if (!(name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto end;

	yaml_unmarshal_log_ctx_name_desc(c, name_desc);

	yaml_unmarshal_log_ctx_key(c, "SCALE");
	scalar = spmap_get(m, "SCALE");
	float scale;
	if (!yaml_scalar_to_float(c, &scale, scalar))
		goto end;

	if (scale <= 0) {
		yaml_unmarshal_log_invalid_value(c, scalar->data.scalar.value, "positive number");
		goto end;
	}

	if (simap_put_if_absent(scales, name_desc, round(scale * 1000))) {
		log_warn("Removing duplicate SCALE %s", name_desc);
	}

end:
	free(name_desc);
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);

	return;
}

void yaml_map_into_transforms_v1(struct UC *c, const struct SImap* const transforms, const yaml_node_t *map) {
	yaml_unmarshal_log_v1(c);

	const struct SPmap *m;
	if (!transforms || !(m = yaml_map_to_spmap(c, map)))
		return;

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
	const yaml_node_t *scalar = spmap_get(m, "NAME_DESC");
	if (!(name_desc = yaml_scalar_to_name_desc(c, scalar)))
		goto end;

	yaml_unmarshal_log_ctx_name_desc(c, name_desc);

	yaml_unmarshal_log_ctx_key(c, "TRANSFORM");
	scalar = spmap_get(m, "TRANSFORM");
	enum wl_output_transform transform;
	if (!(transform = yaml_scalar_to_enum(c, scalar, transform_val, transform_names)))
		goto end;

	if (simap_put_if_absent(transforms, name_desc, transform)) {
		log_warn("Removing duplicate TRANSFORM %s", name_desc);
	}

end:
	free(name_desc);
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);
}

void yaml_node_into_disableds_v1(struct UC *c, const struct SPmap* const disableds, const yaml_node_t *node) {
	yaml_unmarshal_log_v1(c);

	if (!disableds || !yaml_check_node_type(c, node, YAML_SCALAR_NODE, YAML_MAPPING_NODE))
		return;

	struct CfgDisabled *disabled = NULL;

	const struct SPmap *m = NULL;

	if (node->type == YAML_SCALAR_NODE) {
		char *name_desc = NULL;

		disabled = cfg_disabled_init();
		if (!(name_desc = yaml_scalar_to_name_desc(c, node)))
			goto err;

		if (spmap_put_if_absent(disableds, name_desc, disabled)) {
			cfg_disabled_free(disabled);
		}

		free(name_desc);

	} else if (node->type == YAML_MAPPING_NODE && (m = yaml_map_to_spmap(c, node))) {

		char *name_desc = NULL;

		yaml_unmarshal_log_ctx_key(c, "NAME_DESC");
		const yaml_node_t *scalar = spmap_get(m, "NAME_DESC");
		if (!(name_desc = yaml_scalar_to_name_desc(c, scalar))) {
			free(name_desc);
			goto err;
		}

		yaml_unmarshal_log_ctx_name_desc(c, name_desc);

		disabled = (struct CfgDisabled*)spmap_get(disableds, name_desc);

		if (!disabled) {
			disabled = cfg_disabled_init();
			spmap_put(disableds, name_desc, disabled);
		}

		free(name_desc);

		yaml_unmarshal_log_ctx_key(c, "IF");
		const yaml_node_t *map = spmap_get(m, "IF");
		if (map)
			yaml_seq_into_col(c, map, disabled->conditions, (fn_yaml_node_into_col)yaml_map_into_conditions);
	}

	goto end;

err:
	cfg_disabled_free(disabled);

end:
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(c, NULL);
	yaml_unmarshal_log_ctx_name_desc(c, NULL);
}


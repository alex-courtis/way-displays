#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <yaml.h>

#include "yaml/unmarshal-types-v1.h"

#include "cfg/disabled.h"
#include "enum.h"
#include "log.h"
#include "mode.h"
#include "simap.h"
#include "spmap.h"
#include "yaml/unmarshal-primitives.h"
#include "yaml/unmarshal-types.h"
#include "yaml/unmarshal.h"

void yaml_scalar_into_laptop_display_prefix_v1(const yaml_node_t *node) {
	char *laptop_display_prefix = yaml_scalar_to_string(node);

	if (!laptop_display_prefix)
		return;

	strncpy(uc.v1_laptop_display_prefix, laptop_display_prefix, sizeof(uc.v1_laptop_display_prefix) - 1);
	free(laptop_display_prefix);
}

void yaml_map_into_cfg_modes_v1(const struct SPmap* const modes, const yaml_node_t *map) {
	uc.v1_present = true;

	const struct SPmap *m;
	if (!modes || !(m = yaml_map_to_spmap(map)))
		return;

	struct Mode *mode = mode_init();

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key("NAME_DESC");
	const yaml_node_t *scalar = spmap_remove(m, "NAME_DESC");
	if (!(name_desc = yaml_scalar_to_name_desc(scalar)))
		goto err;

	yaml_unmarshal_log_ctx_name_desc(name_desc);

	yaml_unmarshal_log_ctx_key("WIDTH");
	scalar = spmap_remove(m, "WIDTH");
	if (scalar && !yaml_scalar_to_int(&mode->width, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key("HEIGHT");
	scalar = spmap_remove(m, "HEIGHT");
	if (scalar && !yaml_scalar_to_int(&mode->height, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key("HZ");
	scalar = spmap_remove(m, "HZ");
	if (scalar) {
		float hz = 0;
		if (!yaml_scalar_to_float(&hz, scalar))
			goto err;
		mode->refresh_mhz = lround(hz * 1000);
	}

	yaml_unmarshal_log_ctx_key("MAX");
	scalar = spmap_remove(m, "MAX");
	if (scalar && !yaml_scalar_to_boolean(&mode->max, scalar))
		goto err;

	yaml_unmarshal_log_ctx_key("MAX_PREFERRED_REFRESH");
	scalar = spmap_remove(m, "MAX_PREFERRED_REFRESH");
	if (scalar && !yaml_scalar_to_boolean(&mode->max_preferred_refresh, scalar))
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
	yaml_unmarshal_log_ctx_key(NULL);
	yaml_unmarshal_log_ctx_name_desc(NULL);
}

void yaml_map_into_scales_v1(const struct SImap* const scales, const yaml_node_t *map) {
	uc.v1_present = true;

	const struct SPmap *m;
	if (!scales || !(m = yaml_map_to_spmap(map)))
		return;

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key("NAME_DESC");
	const yaml_node_t *scalar = spmap_remove(m, "NAME_DESC");
	if (!(name_desc = yaml_scalar_to_name_desc(scalar)))
		goto end;

	yaml_unmarshal_log_ctx_name_desc(name_desc);

	yaml_unmarshal_log_ctx_key("SCALE");
	scalar = spmap_remove(m, "SCALE");
	float scale;
	if (!yaml_scalar_to_float(&scale, scalar))
		goto end;

	if (scale <= 0) {
		yaml_log_invalid_value(scalar->data.scalar.value, "positive number");
		goto end;
	}

	if (simap_put_if_absent(scales, name_desc, round(scale * 1000))) {
		log_warn("Removing duplicate SCALE %s", name_desc);
	}

end:
	free(name_desc);
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(NULL);
	yaml_unmarshal_log_ctx_name_desc(NULL);

	return;
}

void yaml_map_into_transforms_v1(const struct SImap* const transforms, const yaml_node_t *map) {
	uc.v1_present = true;

	const struct SPmap *m;
	if (!transforms || !(m = yaml_map_to_spmap(map)))
		return;

	char *name_desc = NULL;

	yaml_unmarshal_log_ctx_key("NAME_DESC");
	const yaml_node_t *scalar = spmap_remove(m, "NAME_DESC");
	if (!(name_desc = yaml_scalar_to_name_desc(scalar)))
		goto end;

	yaml_unmarshal_log_ctx_name_desc(name_desc);

	yaml_unmarshal_log_ctx_key("TRANSFORM");
	scalar = spmap_remove(m, "TRANSFORM");
	enum wl_output_transform transform;
	if (!(transform = yaml_scalar_to_enum(scalar, transform_val, transform_names)))
		goto end;

	if (simap_put_if_absent(transforms, name_desc, transform)) {
		log_warn("Removing duplicate TRANSFORM %s", name_desc);
	}

end:
	free(name_desc);
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(NULL);
	yaml_unmarshal_log_ctx_name_desc(NULL);
}

void yaml_node_into_disableds_v1(const struct SPmap* const disableds, const yaml_node_t *node) {
	uc.v1_present = true;

	if (!disableds || !yaml_check_node_type(node, YAML_SCALAR_NODE, YAML_MAPPING_NODE))
		return;

	struct CfgDisabled *disabled = NULL;

	const struct SPmap *m = NULL;

	if (node->type == YAML_SCALAR_NODE) {
		char *name_desc = NULL;

		disabled = cfg_disabled_init();
		if (!(name_desc = yaml_scalar_to_name_desc(node)))
			goto err;

		if (spmap_put_if_absent(disableds, name_desc, disabled)) {
			cfg_disabled_free(disabled);
		}

		free(name_desc);

	} else if (node->type == YAML_MAPPING_NODE && (m = yaml_map_to_spmap(node))) {

		char *name_desc = NULL;

		yaml_unmarshal_log_ctx_key("NAME_DESC");
		const yaml_node_t *scalar = spmap_remove(m, "NAME_DESC");
		if (!(name_desc = yaml_scalar_to_name_desc(scalar))) {
			free(name_desc);
			goto err;
		}

		yaml_unmarshal_log_ctx_name_desc(name_desc);

		disabled = (struct CfgDisabled*)spmap_get(disableds, name_desc);

		if (!disabled) {
			disabled = cfg_disabled_init();
			spmap_put(disableds, name_desc, disabled);
		}

		free(name_desc);

		yaml_unmarshal_log_ctx_key("IF");
		const yaml_node_t *map = spmap_remove(m, "IF");
		if (map)
			yaml_seq_into_col(map, disabled->conditions, (fn_yaml_node_into_col)yaml_map_into_conditions);
	}

	goto end;

err:
	cfg_disabled_free(disabled);

end:
	spmap_free(m);
	yaml_unmarshal_log_ctx_key(NULL);
	yaml_unmarshal_log_ctx_name_desc(NULL);
}


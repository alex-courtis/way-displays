#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "output.h"

#include "displ.h"
#include "fn.h"
#include "ipmap.h"
#include "listeners.h"
#include "log.h"
#include "wlr-output-power-management-unstable-v1.h"
#include "xdg-output-unstable-v1.h"

struct Output *output_init(struct wl_output *wl_output, const uint32_t name, const struct Displ *displ) {
	if (!displ)
		return NULL;

	struct Output *output = calloc(1, sizeof(struct Output));
	output->wl_output = wl_output;
	output->wl_output_name = name;

	if (displ->zxdg_output_manager) {
		struct zxdg_output_v1 *zxdg_output = zxdg_output_manager_v1_get_xdg_output(displ->zxdg_output_manager, wl_output);
		if (zxdg_output) {
			output->zxdg_output = zxdg_output;

			zxdg_output_v1_add_listener(zxdg_output, zxdg_output_listener(), output);
		}
	}

	if (displ->zwlr_output_power_manager) {
		struct zwlr_output_power_v1 *zwlr_output_power = zwlr_output_power_manager_v1_get_output_power(displ->zwlr_output_power_manager, wl_output);
		log_debug("output_init zwlr_output_power=%p", (void*)zwlr_output_power);
		if (zwlr_output_power) {
			output->zwlr_output_power = zwlr_output_power;

			zwlr_output_power_v1_add_listener(zwlr_output_power, zwlr_output_power_listener(), output);

			// zwlr_output_power_v1_destroy(zwlr_output_power);

			// zwlr_output_power_manager_v1_destroy(displ->zwlr_output_power_manager);
		}
	}

	return output;
}

// frees and releases resources
static void output_destroy(struct Output *output) {
	if (!output)
		return;

	if (output->zxdg_output)
		zxdg_output_v1_destroy(output->zxdg_output);

	if (output->wl_output)
		wl_output_destroy(output->wl_output);

	free(output->name);

	free(output->description);

	free(output);
}

const struct IPmap *output_ipmap_init(void) {
	const struct IPmapParams params = { .free_val = (fn_free)output_destroy, };
	return ipmap_init_with(params);
}

bool output_matches_name(const struct Output* const output, const void* const name) {
	return name && output && output->name && strcmp(output->name, name) == 0;
}

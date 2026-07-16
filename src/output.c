#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "output.h"

#include "fn.h"
#include "ipmap.h"
#include "listeners.h"
#include "xdg-output-unstable-v1.h"

struct Output *output_init(struct wl_output *wl_output, const uint32_t name, struct zxdg_output_manager_v1 *zxdg_output_manager) {
	if (!zxdg_output_manager)
		return NULL;

	struct zxdg_output_v1 *zxdg_output = zxdg_output_manager_v1_get_xdg_output(zxdg_output_manager, wl_output);
	if (!zxdg_output)
		return NULL;

	struct Output *output = calloc(1, sizeof(struct Output));
	output->wl_output = wl_output;
	output->wl_output_name = name;
	output->zxdg_output = zxdg_output;

	zxdg_output_v1_add_listener(zxdg_output, zxdg_output_listener(), output);

	return output;
}

const struct IPmap *output_ipmap_init(void) {
	const struct IPmapParams params = { .free_val = (fn_free)output_destroy, };
	return ipmap_init_with(params);
}

void output_destroy(struct Output *output) {
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

bool output_matches_name(const struct Output* const output, const void* const name) {
	return name && output && output->name && strcmp(output->name, name) == 0;
}

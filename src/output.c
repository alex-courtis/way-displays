#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "output.h"

#include "listeners.h"
#include "imap.h"
#include "xdg-output-unstable-v1.h"

const struct IMap *g_outputs; // by wl_output_name

static void destroy(const void *o) {
	if (!o)
		return;

	struct Output *output = (struct Output*)o;

	zxdg_output_v1_destroy(output->zxdg_output);
	wl_output_destroy(output->wl_output);

	free(output->name);
	free(output->description);

	free(output);
}

struct Output *output_init(struct wl_output *wl_output, const uint32_t wl_output_name, struct zxdg_output_manager_v1 *zxdg_output_manager) {
	if (!wl_output || !zxdg_output_manager)
		return NULL;

	struct zxdg_output_v1 *zxdg_output = zxdg_output_manager_v1_get_xdg_output(zxdg_output_manager, wl_output);
	if (!zxdg_output)
		return NULL;

	struct Output *output = calloc(1, sizeof(struct Output));
	output->wl_output = wl_output;
	output->wl_output_name = wl_output_name;
	output->zxdg_output = zxdg_output;

	if (!g_outputs) {
		const struct IMapParams params = { .free_val = destroy, };
		g_outputs = imap_init_with(params);
	}
	imap_put(g_outputs, wl_output_name, output);

	zxdg_output_v1_add_listener(zxdg_output, zxdg_output_listener(), output);

	return output;
}

bool output_matches_name(const size_t key, const struct Output* const output, const void* const name) {
	return name && output && output->name && strcmp(output->name, name) == 0;
}

void output_destroy_all(void) {
	imap_free_vals(g_outputs);
}

void output_destroy_by_wl_output_name(const uint32_t wl_output_name) {
	destroy(imap_remove(g_outputs, wl_output_name));
}


#include <stdint.h>
#include <stdlib.h>
#include <wayland-client-protocol.h>

#include "output.h"

#include "displ.h"
#include "listeners.h"
#include "itable.h"
#include "xdg-output-unstable-v1.h"

const struct ITable *op;

struct Output *output_init(struct wl_output *wl_output, const uint32_t wl_output_name, struct zxdg_output_manager_v1 *zxdg_output_manager) {
	if (!wl_output || !zxdg_output_manager)
		return NULL;

	if (!op)
		op = itable_init(5, 5);

	struct zxdg_output_v1 *zxdg_output = zxdg_output_manager_v1_get_xdg_output(zxdg_output_manager, wl_output);
	if (!zxdg_output)
		return NULL;

	struct Output *output = calloc(1, sizeof(struct Output));
	output->wl_output = wl_output;
	output->wl_output_name = wl_output_name;
	output->zxdg_output = zxdg_output;

	if (!op)
		op = itable_init(5, 5);
	itable_put(op, wl_output_name, output);

	zxdg_output_v1_add_listener(zxdg_output, zxdg_output_listener(), output);

	return output;
}

void destroy(const void *o) {
	if (!o)
		return;

	struct Output *output = (struct Output*)o;

	zxdg_output_v1_destroy(output->zxdg_output);
	wl_output_destroy(output->wl_output);

	free(output->name);
	free(output->description);

	free(output);
}

void output_destroy_all(void) {
	itable_free_vals(op, destroy);
}

void output_destroy_by_wl_output_name(const uint32_t wl_output_name) {
	destroy(itable_remove(op, wl_output_name));
}


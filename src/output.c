#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "output.h"

#include "listeners.h"
#include "itable.h"
#include "xdg-output-unstable-v1.h"

const struct ITable *outputs; // by wl_output_name

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

	if (!outputs)
		outputs = itable_init(5, 5);
	itable_put(outputs, wl_output_name, output);

	zxdg_output_v1_add_listener(zxdg_output, zxdg_output_listener(), output);

	return output;
}

const struct Output *output_for_name(const char *name) {
	const struct Output *output = NULL;

	const struct ITableIter *i = NULL;
	for (i = itable_iter(outputs); i; i = itable_next(i)) {
		output = i->val;
		if (output && output->name && strcmp(output->name, name) == 0) {
			break;
		} else {
			output = NULL;
		}
	}
	itable_iter_free(i);

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
	itable_free_vals(outputs, destroy);
}

void output_destroy_by_wl_output_name(const uint32_t wl_output_name) {
	destroy(itable_remove(outputs, wl_output_name));
}


#include <stdlib.h>

#include "slist.h"

#include "output.h"

struct SList *outputs = NULL;

struct Output *output_init(struct wl_output *wl_output, struct zxdg_output_v1 *zxdg_output) {
	if (!wl_output || !zxdg_output)
		return NULL;

	struct Output *output = calloc(1, sizeof(struct Output));

	output->wl_output = wl_output;
	output->zxdg_output = zxdg_output;

	slist_append(&outputs, output);

	return output;
}

void output_destroy(struct Output *output) {
	if (!output)
		return;

	free(output);
}


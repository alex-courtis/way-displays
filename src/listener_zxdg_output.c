#include <stdint.h>
#include <string.h>

#include "listeners.h"

#include "log.h"
#include "output.h"
#include "xdg-output-unstable-v1.h"

// Output data

void zxdg_output_logical_position(void *data,
		struct zxdg_output_v1 *zxdg_output_v1,
		int32_t x,
		int32_t y) {
	log_debug("zxdg_output_logical_position %d, %d", x, y);

	struct Output *output = data;

	output->logical_x = x;
	output->logical_y = y;
}

void zxdg_output_logical_size(void *data,
		struct zxdg_output_v1 *zxdg_output_v1,
		int32_t width,
		int32_t height) {
	log_debug("zxdg_output_logical_size %d, %d", width, height);

	struct Output *output = data;

	output->logical_width = width;
	output->logical_height = height;
}

// deprecated
void zxdg_output_done(void *data,
		struct zxdg_output_v1 *zxdg_output_v1) {
	log_debug("zxdg_output_done");
}

void zxdg_output_name(void *data,
		struct zxdg_output_v1 *zxdg_output_v1,
		const char *name) {
	log_debug("zxdg_output_name '%s'", name);

	struct Output *output = data;

	output->name = strdup(name);
}

void zxdg_output_description(void *data,
		struct zxdg_output_v1 *zxdg_output_v1,
		const char *description) {

	log_debug("zxdg_output_description '%s'", description);

	struct Output *output = data;

	output->description = strdup(description);
}

static const struct zxdg_output_v1_listener listener = {
	.logical_position = zxdg_output_logical_position,
	.logical_size = zxdg_output_logical_size,
	.done = zxdg_output_done,
	.name = zxdg_output_name,
	.description = zxdg_output_description,
};

const struct zxdg_output_v1_listener *zxdg_output_listener(void) {
	return &listener;
}


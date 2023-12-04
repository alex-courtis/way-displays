#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>
#include <stdbool.h>

extern struct SList *outputs;

struct Output {
	struct wl_output *wl_output;
	struct zxdg_output_v1 *zxdg_output;

	// wlr-output-management-unstable-v1.xml states that the names and descriptions must match hence we can map them
	char *name;
	char *description;

	int32_t logical_x;
	int32_t logical_y;
	int32_t logical_width;
	int32_t logical_height;
};

// instantiate a new output with two mandatory fields populated, appending to outputs
struct Output *output_init(struct wl_output *wl_output, struct zxdg_output_v1 *zxdg_output);

// destroy an output, removing from outputs
void output_destroy(struct Output *output);

#endif // OUTPUT_H

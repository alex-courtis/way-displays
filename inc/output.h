#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>
#include <wayland-client-protocol.h>

#include "displ.h"

struct Output {
	struct wl_output *wl_output;
	uint32_t wl_output_name;

	struct zxdg_output_v1 *zxdg_output;

	// wlr-output-management-unstable-v1.xml states that the names and descriptions must match hence we can map them
	char *name;
	char *description;

	int32_t logical_x;
	int32_t logical_y;
	int32_t logical_width;
	int32_t logical_height;
};

// instantiate a new output and create an xdg output listener, appending to outputs
// NULL on failure to retrieve xdg output
struct Output *output_init(struct wl_output *wl_output, const uint32_t wl_output_name, struct zxdg_output_manager_v1 *zxdg_output_manager);

// destroy all outputs, clearing outputs
void output_destroy_all(void);

// destroy an output matching name, removing from outputs
void output_destroy_by_wl_output_name(const uint32_t wl_output_name);

#endif // OUTPUT_H

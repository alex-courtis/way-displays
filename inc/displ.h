#ifndef DISPL_H
#define DISPL_H

#include <stdint.h>
#include <stdbool.h>

// IWYU pragma: begin_keep
#include "wlr-output-management-unstable-v1.h"
#include "xdg-output-unstable-v1.h"
// IWYU pragma: end_keep

enum ConfigState {
	IDLE = 0,
	SUCCEEDED,
	OUTSTANDING,
	CANCELLED,
	FAILED,
};

struct Displ {
	// global
	struct wl_display *display;
	struct wl_registry *registry;
	uint32_t name;

	// output manager
	struct zwlr_output_manager_v1 *output_manager;
	uint32_t serial;
	char *interface;
	uint32_t output_manager_version;

	bool have_fractional_scale_v1;

	// output
	struct zxdg_output_manager_v1 *zxdg_output_manager;
	uint32_t zxdg_output_manager_name;
	uint32_t zxdg_output_manager_version;

	enum ConfigState config_state;
};

void displ_init(void);

void displ_destroy(void);

#endif // DISPL_H

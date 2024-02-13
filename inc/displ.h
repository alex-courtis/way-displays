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
	struct wl_registry *registry;
	struct wl_display *display;

	// wlroots output manager
	struct zwlr_output_manager_v1 *zwlr_output_manager;
	uint32_t zwlr_output_manager_name;
	uint32_t zwlr_output_manager_version;
	char *zwlr_output_manager_interface;
	uint32_t zwlr_output_manager_serial;

	// wayland output manager
	struct zxdg_output_manager_v1 *zxdg_output_manager;
	uint32_t zxdg_output_manager_name;
	uint32_t zxdg_output_manager_version;
	char *zxdg_output_manager_interface;

	enum ConfigState config_state;
};

void displ_init(void);

void displ_destroy(void);

#endif // DISPL_H

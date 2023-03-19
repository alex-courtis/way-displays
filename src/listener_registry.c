#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "listeners.h"

#include "displ.h"
#include "log.h"
#include "process.h"
#include "wlr-output-management-unstable-v1.h"

// Displ data

static void global(void *data,
		struct wl_registry *wl_registry,
		uint32_t name,
		const char *interface,
		uint32_t version) {

	// only register for WLR output manager events
	if (strcmp(interface, zwlr_output_manager_v1_interface.name) != 0)
		return;

	struct Displ *displ = data;
	displ->name = name;
	displ->interface = strdup(interface);

	if (version < ZWLR_OUTPUT_MANAGER_V1_VERSION_MIN) {
		log_error("\nwlr-output-management version %d found, minimum %d required, exiting. Consider upgrading your compositor.", version, ZWLR_OUTPUT_MANAGER_V1_VERSION_MIN);
		wd_exit(EXIT_FAILURE);
	} else if (version < ZWLR_OUTPUT_MANAGER_V1_VERSION) {
		log_warn("\nwlr-output-management version %d found; %d required for full functionality. Consider upgrading your compositor.", version, ZWLR_OUTPUT_MANAGER_V1_VERSION);
		displ->output_manager_version = ZWLR_OUTPUT_MANAGER_V1_VERSION_MIN;
	} else {
		displ->output_manager_version = ZWLR_OUTPUT_MANAGER_V1_VERSION;
	}

	displ->output_manager = wl_registry_bind(wl_registry, name, &zwlr_output_manager_v1_interface, displ->output_manager_version);

	zwlr_output_manager_v1_add_listener(displ->output_manager, output_manager_listener(), displ);
}

static void global_remove(void *data,
		struct wl_registry *wl_registry,
		uint32_t name) {
	struct Displ *displ = data;

	// only interested in the WLR interface
	if (!displ || displ->name != name)
		return;

	// a "who cares?" situation in the WLR examples
	log_info("\nDisplay's output manager has been removed, exiting");
	wd_exit(EXIT_SUCCESS);
}

static const struct wl_registry_listener listener = {
	.global = global,
	.global_remove = global_remove,
};

const struct wl_registry_listener *registry_listener(void) {
	return &listener;
}


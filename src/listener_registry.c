#include <stdio.h>
#include <string.h>

#include "wlr-output-management-unstable-v1.h"

#include "listeners.h"

static void global(void *data,
		struct wl_registry *wl_registry,
		uint32_t name,
		const char *interface,
		uint32_t version) {
	if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0) {
		struct zwlr_output_manager_v1 *output_manager = wl_registry_bind(wl_registry, name, &zwlr_output_manager_v1_interface, 1);
		zwlr_output_manager_v1_add_listener(output_manager, output_manager_listener(), data);
	}
}
static void global_remove(void *data,
		struct wl_registry *wl_registry,
		uint32_t name) {
	// todo: release
}

static const struct wl_registry_listener listener = {
	.global = global,
	.global_remove = global_remove,
};

const struct wl_registry_listener *registry_listener() {
	return &listener;
}


#include <string.h>

#include "wlr-output-management-unstable-v1.h"

#include "listeners.h"
#include "types.h"

// OutputManager data

static void global(void *data,
		struct wl_registry *wl_registry,
		uint32_t name,
		const char *interface,
		uint32_t version) {
	if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0) {
		fprintf(stderr, "LR global zwlr\n");
		struct OutputManager *output_manager = data;

		output_manager->name = name;
		output_manager->interface = strdup(interface);

		output_manager->zwlr_output_manager = wl_registry_bind(wl_registry, name, &zwlr_output_manager_v1_interface, version);

		zwlr_output_manager_v1_add_listener(output_manager->zwlr_output_manager, output_manager_listener(), data);
	}
}

static void global_remove(void *data,
		struct wl_registry *wl_registry,
		uint32_t name) {
	struct OutputManager *output_manager = data;
	if (name == output_manager->name) {
		fprintf(stderr, "LR global_remove zwlr\n");
	}

	// TODO release
}

static const struct wl_registry_listener listener = {
	.global = global,
	.global_remove = global_remove,
};

const struct wl_registry_listener *registry_listener() {
	return &listener;
}


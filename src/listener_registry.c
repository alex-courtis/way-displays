#include <string.h>

#include "wlr-output-management-unstable-v1.h"

#include "listeners.h"
#include "types.h"

// Displ data

static void global(void *data,
		struct wl_registry *wl_registry,
		uint32_t name,
		const char *interface,
		uint32_t version) {

	// only register for WLR output manager events
	if (strcmp(interface, zwlr_output_manager_v1_interface.name) != 0)
		return;

	fprintf(stderr, "LR global zwlr data %p\n", (void*)data);
	struct Displ *displ = data;
	displ->name = name;

	displ->output_manager = calloc(1, sizeof(struct OutputManager));
	displ->output_manager->displ = displ;
	displ->output_manager->interface = strdup(interface);

	displ->output_manager->zwlr_output_manager = wl_registry_bind(wl_registry, name, &zwlr_output_manager_v1_interface, version);

	zwlr_output_manager_v1_add_listener(displ->output_manager->zwlr_output_manager, output_manager_listener(), displ->output_manager);
}

static void global_remove(void *data,
		struct wl_registry *wl_registry,
		uint32_t name) {
	struct Displ *displ = data;

	// TODO fake a cleanup test by removing anything
	if (!displ || displ->name != name)
		return;

	// TODO call zwlr destroy on each of the objects, to prevent their callbacks

	fprintf(stderr, "LR global_remove freeing OM\n");

	free_output_manager(displ->output_manager);
	displ->output_manager = NULL;

	fprintf(stderr, "LR global_remove freed OM\n");

	// TODO release like zwlr
}

static const struct wl_registry_listener listener = {
	.global = global,
	.global_remove = global_remove,
};

const struct wl_registry_listener *registry_listener() {
	return &listener;
}


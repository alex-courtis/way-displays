#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "listeners.h"

#include "displ.h"
#include "log.h"
#include "output.h"
#include "process.h"
#include "fractional-scale-v1.h"
#include "xdg-output-unstable-v1.h"
#include "wlr-output-management-unstable-v1.h"

static void bind_zwlr_output_manager(struct Displ *displ,
		struct wl_registry *wl_registry,
		uint32_t name,
		const char *interface,
		uint32_t version) {

	displ->display_name = name;
	displ->zwlr_output_manager_interface = strdup(interface);
	displ->zwlr_output_manager = wl_registry_bind(wl_registry, name, &zwlr_output_manager_v1_interface, displ->zwlr_output_manager_version);

	zwlr_output_manager_v1_add_listener(displ->zwlr_output_manager, output_manager_listener(), displ);
}

static void bind_zxdg_output_manager(struct Displ *displ,
		struct wl_registry *wl_registry,
		uint32_t name,
		const char *interface,
		uint32_t version) {

	displ->zxdg_output_manager_name = name;
	displ->zxdg_output_manager_version = version;
	displ->zxdg_output_manager = wl_registry_bind(wl_registry, name, &zxdg_output_manager_v1_interface, displ->zxdg_output_manager_version);
}

static void bind_wl_output(struct Displ *displ,
		struct wl_registry *wl_registry,
		uint32_t name,
		const char *interface,
		uint32_t version) {

	struct wl_output *wl_output = wl_registry_bind(wl_registry, name, &wl_output_interface, version);

	if (!output_init(wl_output, name, displ->zxdg_output_manager)) {
		wl_output_destroy(wl_output);
	}
}

// Displ data

static void global(void *data,
		struct wl_registry *wl_registry,
		uint32_t name,
		const char *interface,
		uint32_t version) {
	struct Displ *displ = data;

	if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0) {
		bind_zwlr_output_manager(data, wl_registry, name, interface, version);
	} else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
		bind_zxdg_output_manager(data, wl_registry, name, interface, version);
	} else if (strcmp(interface, wl_output_interface.name) == 0) {
		bind_wl_output(data, wl_registry, name, interface, version);
	} else if (strcmp(interface, wp_fractional_scale_manager_v1_interface.name) == 0) {
		displ->have_fractional_scale_v1 = true;
		log_debug("\nCompositor supports %s version %d", interface, version);
	}
}

static void global_remove(void *data,
		struct wl_registry *wl_registry,
		uint32_t name) {
	struct Displ *displ = data;

	output_destroy_by_wl_output_name(name);

	// a "who cares?" situation in the WLR examples
	if (displ && displ->display_name == name) {
		log_info("\nDisplay's output manager has been removed, exiting");
		wd_exit(EXIT_SUCCESS);
	}
}

static const struct wl_registry_listener listener = {
	.global = global,
	.global_remove = global_remove,
};

const struct wl_registry_listener *registry_listener(void) {
	return &listener;
}


#include <stdio.h>
#include <wayland-client-protocol.h>

#include "displ.h"

#include "lid.h"
#include "listeners.h"

void connect_display(struct Displ *displ) {

	if (!(displ->display = wl_display_connect(NULL))) {
		fprintf(stderr, "\nERROR: Unable to connect to the compositor. Check or set the WAYLAND_DISPLAY environment variable. exiting\n");
		exit(EXIT_FAILURE);
	}

	displ->registry = wl_display_get_registry(displ->display);

	wl_registry_add_listener(displ->registry, registry_listener(), displ);

	if (wl_display_roundtrip(displ->display) == -1) {
		fprintf(stderr, "\nERROR: wl_display_roundtrip failed -1, exiting");
		exit(EXIT_FAILURE);
	}

	if (!displ->output_manager) {
		fprintf(stderr, "\nERROR: compositor does not support WLR output manager protocol, exiting\n");
		exit(EXIT_FAILURE);
	}
}

void destroy_display(struct Displ *displ) {
	if (!displ)
		return;

	if (displ->output_manager && displ->output_manager->zwlr_output_manager) {
		wl_proxy_destroy((struct wl_proxy*) displ->output_manager->zwlr_output_manager);
	}

	wl_registry_destroy(displ->registry);

	wl_display_disconnect(displ->display);

	destroy_lid(displ);

	free_displ(displ);
}


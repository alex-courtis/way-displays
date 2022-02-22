#include <stdbool.h>
#include <stdlib.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "displ.h"

#include "info.h"
#include "lid.h"
#include "list.h"
#include "listeners.h"
#include "log.h"
#include "types.h"

void connect_display(struct Displ *displ) {

	if (!(displ->display = wl_display_connect(NULL))) {
		log_error("\nUnable to connect to the compositor. Check or set the WAYLAND_DISPLAY environment variable. exiting");
		exit(EXIT_FAILURE);
	}

	displ->registry = wl_display_get_registry(displ->display);

	wl_registry_add_listener(displ->registry, registry_listener(), displ);

	if (wl_display_roundtrip(displ->display) == -1) {
		log_error("\nwl_display_roundtrip failed -1, exiting");
		exit(EXIT_FAILURE);
	}

	if (!displ->output_manager) {
		log_error("\ncompositor does not support WLR output manager protocol, exiting");
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

bool consume_arrived_departed(struct OutputManager *output_manager) {
	if (!output_manager)
		return false;

	bool user_changes = output_manager->heads_arrived || output_manager->heads_departed;

	print_heads(ARRIVED, output_manager->heads_arrived);
	slist_free(&output_manager->heads_arrived);

	print_heads(DEPARTED, output_manager->heads_departed);
	slist_free_vals(&output_manager->heads_departed, free_head);

	return user_changes;
}


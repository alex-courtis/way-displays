#include <stdlib.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "displ.h"

#include "global.h"
#include "listeners.h"
#include "log.h"
#include "output.h"
#include "process.h"
#include "wlr-output-management-unstable-v1.h"
#include "xdg-output-unstable-v1.h"

void displ_init(void) {

	displ = calloc(1, sizeof(struct Displ));

	if (!(displ->display = wl_display_connect(NULL))) {
		log_error("\nUnable to connect to the compositor. Check or set the WAYLAND_DISPLAY environment variable. exiting");
		wd_exit(EXIT_FAILURE);
		return;
	}

	displ->registry = wl_display_get_registry(displ->display);

	wl_registry_add_listener(displ->registry, registry_listener(), displ);

	if (wl_display_roundtrip(displ->display) == -1) {
		log_error("\nwl_display_roundtrip failed -1, exiting");
		wd_exit_message(EXIT_FAILURE);
		return;
	}

	if (!displ->zwlr_output_manager) {
		log_error("\ncompositor does not support WLR output manager protocol, exiting");
		wd_exit(EXIT_FAILURE);
		return;
	}
}

void displ_delta_init(enum CfgElement element, struct Head *head) {
	displ_delta_destroy();

	displ->delta.element = element;

	displ->delta.head = head;
}

void displ_delta_destroy(void) {

	displ->delta.element = 0;

	displ->delta.head = NULL;

	free(displ->delta.human);
	displ->delta.human = NULL;
}

void displ_destroy(void) {

	output_destroy_all();

	if (displ->zwlr_output_manager) {
		zwlr_output_manager_v1_destroy(displ->zwlr_output_manager);
	}

	if (displ->zxdg_output_manager) {
		zxdg_output_manager_v1_destroy(displ->zxdg_output_manager);
	}

	wl_registry_destroy(displ->registry);

	wl_display_disconnect(displ->display);

	free(displ->zwlr_output_manager_interface);

	free(displ->zxdg_output_manager_interface);

	free(displ);
	displ = NULL;
}


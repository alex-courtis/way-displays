#include <stdlib.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "displ.h"

#include "cfg.h"
#include "head.h"
#include "listeners.h"
#include "log.h"
#include "output.h"
#include "process.h"
#include "wlr-output-management-unstable-v1.h"
#include "xdg-output-unstable-v1.h"

struct Displ *g_displ = NULL;

void displ_init(void) {

	g_displ = calloc(1, sizeof(struct Displ));

	if (!(g_displ->display = wl_display_connect(NULL))) {
		log_fatal(NULL);
		log_fatal("Unable to connect to the compositor. Check or set the WAYLAND_DISPLAY environment variable. exiting");
		wd_exit(EXIT_FAILURE);
		return;
	}

	g_displ->registry = wl_display_get_registry(g_displ->display);

	wl_registry_add_listener(g_displ->registry, registry_listener(), g_displ);

	if (wl_display_roundtrip(g_displ->display) == -1) {
		log_fatal(NULL);
		log_fatal("wl_display_roundtrip failed -1, exiting");
		wd_exit_message(EXIT_FAILURE);
		return;
	}

	if (!g_displ->zwlr_output_manager) {
		log_fatal(NULL);
		log_fatal("compositor does not support WLR output manager protocol, exiting");
		wd_exit(EXIT_FAILURE);
		return;
	}
}

void displ_delta_init(enum CfgElement element, struct Head *head) {
	displ_delta_destroy();

	g_displ->delta.element = element;

	g_displ->delta.head = head;
}

void displ_delta_destroy(void) {

	g_displ->delta.element = 0;

	g_displ->delta.head = NULL;

	free(g_displ->delta.human);
	g_displ->delta.human = NULL;
}

void displ_destroy(void) {

	output_destroy_all();

	if (g_displ->zwlr_output_manager) {
		zwlr_output_manager_v1_destroy(g_displ->zwlr_output_manager);
	}

	if (g_displ->zxdg_output_manager) {
		zxdg_output_manager_v1_destroy(g_displ->zxdg_output_manager);
	}

	wl_registry_destroy(g_displ->registry);

	wl_display_disconnect(g_displ->display);

	free(g_displ->zwlr_output_manager_interface);

	free(g_displ->zxdg_output_manager_interface);

	free(g_displ);
	g_displ = NULL;
}


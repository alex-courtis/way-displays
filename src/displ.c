#include <stdlib.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "displ.h"

#include "enum.h"
#include "head.h"
#include "ipmap.h"
#include "listeners.h"
#include "log.h"
#include "output.h"
#include "ppmap.h"
#include "process.h"
#include "pset.h"
#include "wlr-output-management-unstable-v1.h"
#include "xdg-output-unstable-v1.h"

struct Displ *g_displ = NULL;

struct Displ *displ_init(void) {
	struct Displ *displ = calloc(1, sizeof(struct Displ));

	displ->outputs = output_ipmap_init();

	displ->heads = head_ppmap_init();

	displ->heads_arrived = head_pset_init();
	displ->heads_departed = head_pset_init();

	return displ;
}

void g_displ_init(void) {
	g_displ = displ_init();

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

void displ_free(struct Displ *displ) {
	if (!displ)
		return;

	displ_delta_destroy();

	ipmap_free_vals(displ->outputs);

	pset_free(displ->heads_arrived);
	ppmap_free_vals(displ->heads);
	pset_free_vals(displ->heads_departed);

	free(displ->zwlr_output_manager_interface);

	free(displ->zxdg_output_manager_interface);

	free(displ);
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

void g_displ_destroy(void) {

	// destroy outputs before zxdg_output_manager
	ipmap_free_vals(g_displ->outputs);
	g_displ->outputs = NULL;

	if (g_displ->zwlr_output_manager)
		zwlr_output_manager_v1_destroy(g_displ->zwlr_output_manager);

	if (g_displ->zxdg_output_manager)
		zxdg_output_manager_v1_destroy(g_displ->zxdg_output_manager);

	wl_registry_destroy(g_displ->registry);

	wl_display_disconnect(g_displ->display);

	displ_free(g_displ);

	g_displ = NULL;
}


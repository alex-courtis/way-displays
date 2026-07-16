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

	displ_delta_destroy(g_displ);

	ipmap_free_vals(displ->outputs);

	pset_free(displ->heads_arrived);
	ppmap_free_vals(displ->heads);
	pset_free_vals(displ->heads_departed);

	free(displ->zwlr_output_manager_interface);

	free(displ->zxdg_output_manager_interface);

	free(displ);
}

void displ_delta_init(struct Displ *displ, enum CfgElement element, struct Head *head) {
	displ_delta_destroy(g_displ);

	displ->delta.element = element;

	displ->delta.head = head;
}

void displ_delta_destroy(struct Displ *displ) {

	displ->delta.element = 0;

	displ->delta.head = NULL;

	free(displ->delta.human);
	displ->delta.human = NULL;
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

void displ_add_head(const struct Displ *displ, struct zwlr_output_head_v1 *zwlr_head) {
	if (!zwlr_head)
		return;

	struct Head *head = head_init();

	ppmap_put(displ->heads, zwlr_head, head);
	pset_add(displ->heads_arrived, head);

	zwlr_output_head_v1_add_listener(zwlr_head, zwlr_output_head_listener(), head);
}

void displ_finished_head(const struct Displ *displ, const struct zwlr_output_head_v1 * const zwlr_head) {
	const struct Head *head = ppmap_get(displ->heads, zwlr_head);
	if (!head)
		return;

	// dummy Head, just for printing
	pset_add(displ->heads_departed, head_dummy_init(head));

	pset_remove(displ->heads_arrived, head);
	pset_remove(displ->heads_departed, head);

	ppmap_remove_free(displ->heads, zwlr_head);
}


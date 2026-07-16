#ifndef DISPL_H
#define DISPL_H

#include <stdint.h>

#include "enum.h"
#include "head.h"

// global singleton
extern struct Displ *g_displ;

struct DisplDelta {
	enum CfgElement element; // 0 for many changes, VRR_OFF indicates toggle
	struct Head *head;       // only when element set
	char *human;             // collected for callbacks
};

struct Displ {
	// global
	struct wl_registry *registry;
	struct wl_display *display;

	// wlroots output manager
	struct zwlr_output_manager_v1 *zwlr_output_manager;
	uint32_t zwlr_output_manager_name;
	uint32_t zwlr_output_manager_version;
	char *zwlr_output_manager_interface;
	uint32_t zwlr_output_manager_serial;

	const struct PPmap *heads;          // head_ppmap_init - Heads by zwlr_output_head_v1
	const struct Pset *heads_arrived;   // head_pset_init  - pointers to heads
	const struct Pset *heads_departed;  // head_pset_init  - dummy heads for printing

	// wayland output manager
	struct zxdg_output_manager_v1 *zxdg_output_manager;
	uint32_t zxdg_output_manager_name;
	uint32_t zxdg_output_manager_version;
	char *zxdg_output_manager_interface;

	const struct IPmap *outputs; // output_ipmap_init - Outputs by uint32_t name

	enum DisplState state;
	struct DisplDelta delta;
};

struct Displ *displ_init(void);

// instantiates g_displ, connects to the WL display/registry, adds the listener and performs one round trip for registration
void g_displ_init(void);

// free outputs and delta
void displ_free(struct Displ *displ);

// TODO should some of these be g_ ?
void displ_delta_init(enum CfgElement element, struct Head *head);

void displ_delta_destroy(void);

// free and release all resources
void g_displ_destroy(void);

// instantiate an arrived head
void displ_add_head(const struct Displ *displ, struct zwlr_output_head_v1 *zwlr_head);

// remove and free a head, putting a head_dummy_init in g_displ->heads_departed
void g_displ_finished_head(const struct zwlr_output_head_v1 * const zwlr_head);

#endif // DISPL_H

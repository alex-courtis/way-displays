#ifndef DISPL_H
#define DISPL_H

#include <stdint.h>

// IWYU pragma: begin_keep
#include "wlr-output-management-unstable-v1.h"
#include "xdg-output-unstable-v1.h"
// IWYU pragma: end_keep

#include "cfg.h"

enum ConfigState {
	IDLE = 0,
	SUCCEEDED,
	OUTSTANDING,
	CANCELLED,
	FAILED,
};

struct DisplDelta {
	enum CfgElement element; // VRR_OFF indicates toggle

	// only when element set
	struct Head *head;

	char *human;
};

extern struct LayoutDelta layout_delta;

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

	// wayland output manager
	struct zxdg_output_manager_v1 *zxdg_output_manager;
	uint32_t zxdg_output_manager_name;
	uint32_t zxdg_output_manager_version;
	char *zxdg_output_manager_interface;

	enum ConfigState state;
	struct DisplDelta delta;
};

void displ_init(void);

void displ_delta_init(enum CfgElement element, struct Head *head);

void displ_delta_destroy(void);

void displ_destroy(void);

#endif // DISPL_H

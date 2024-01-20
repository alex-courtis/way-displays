#ifndef DISPL_H
#define DISPL_H

#include <stdint.h>

#define MAX_SEQUENTIAL_CANCELLATIONS 25

enum ConfigState {
	IDLE = 0,
	SUCCEEDED,
	OUTSTANDING,
	CANCELLED,
	FAILED,
};

struct Displ {
	// global
	struct wl_display *display;
	struct wl_registry *registry;
	uint32_t name;

	// output manager
	struct zwlr_output_manager_v1 *output_manager;
	uint32_t serial;
	char *interface;
	uint32_t output_manager_version;

	enum ConfigState config_state;

	uint32_t sequential_cancellations;
};

void displ_init(void);

void displ_destroy(void);

#endif // DISPL_H

#include <stdint.h>

#include "listeners.h"

#include "displ.h"
#include "wlr-output-management-unstable-v1.h"

// Displ data

static void head(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
		struct zwlr_output_head_v1 *zwlr_output_head_v1) {

	displ_add_head(data, zwlr_output_head_v1);
}

static void done(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
		uint32_t serial) {
	struct Displ *displ = data;

	displ->zwlr_output_manager_serial = serial;
}

static void finished(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1) {
	struct Displ *displ = data;

	if (displ->zwlr_output_manager) {
		zwlr_output_manager_v1_destroy(displ->zwlr_output_manager);
	}
}

static const struct zwlr_output_manager_v1_listener listener = {
	.head = head,
	.done = done,
	.finished = finished,
};

const struct zwlr_output_manager_v1_listener *zwlr_output_manager_listener(void) {
	return &listener;
}


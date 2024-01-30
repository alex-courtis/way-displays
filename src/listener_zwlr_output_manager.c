#include <stdint.h>
#include <stdlib.h>

#include "listeners.h"

#include "displ.h"
#include "head.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"

// Displ data

static void head(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
		struct zwlr_output_head_v1 *zwlr_output_head_v1) {

	struct Displ *displ = data;

	struct Head *head = calloc(1, sizeof(struct Head));
	head->zwlr_head = zwlr_output_head_v1;
	head->scaling_base = HEAD_DEFAULT_SCALING_BASE;

	slist_append(&heads, head);
	slist_append(&heads_arrived, head);

	if (displ->zwlr_output_manager_version == ZWLR_OUTPUT_MANAGER_V1_VERSION_MIN) {
		zwlr_output_head_v1_add_listener(zwlr_output_head_v1, zwlr_output_head_listener_min(), head);
	} else {
		zwlr_output_head_v1_add_listener(zwlr_output_head_v1, zwlr_output_head_listener(), head);
	}
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


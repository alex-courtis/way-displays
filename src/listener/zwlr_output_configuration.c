#include <stddef.h>

#include "listeners.h"

#include "displ.h"
#include "enum.h"
#include "head.h"
#include "log.h"
#include "ppmap.h"
#include "wlr-output-management-unstable-v1.h"

// Displ data

static void cleanup(struct Displ *displ,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1,
		enum DisplState state) {

	for (const struct PPmapIt *it = ppmap_it(displ->heads); it; it = ppmap_it_next(it)) {
		struct Head *head = (struct Head*)it->val;
		if (head->zconfig) {
			zwlr_output_configuration_head_v1_destroy(head->zconfig);
			head->zconfig = NULL;
		}
	}

	zwlr_output_configuration_v1_destroy(zwlr_output_configuration_v1);

	displ->state = state;
}

static void succeeded(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	log_warn("EVENT: zwlr_output_configuration_v1 succeeded zwlr_output_manager_serial %u (%u)", g_displ->zwlr_output_manager_serial, g_displ->zwlr_output_manager_serial_prev);
	cleanup(data, zwlr_output_configuration_v1, SUCCEEDED);
}

static void failed(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	log_warn("EVENT: zwlr_output_configuration_v1 failed zwlr_output_manager_serial %u (%u)", g_displ->zwlr_output_manager_serial, g_displ->zwlr_output_manager_serial_prev);
	cleanup(data, zwlr_output_configuration_v1, FAILED);
}

static void cancelled(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	log_warn("EVENT: zwlr_output_configuration_v1 cancelled zwlr_output_manager_serial %u (%u)", g_displ->zwlr_output_manager_serial, g_displ->zwlr_output_manager_serial_prev);
	cleanup(data, zwlr_output_configuration_v1, CANCELLED);
}

static const struct zwlr_output_configuration_v1_listener listener = {
	.succeeded = succeeded,
	.failed = failed,
	.cancelled = cancelled,
};

const struct zwlr_output_configuration_v1_listener *zwlr_output_configuration_listener(void) {
	return &listener;
}


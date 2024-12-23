#include <stddef.h>

#include "listeners.h"

#include "displ.h"
#include "slist.h"
#include "head.h"
#include "wlr-output-management-unstable-v1.h"

// Displ data

void cleanup(struct Displ *displ,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1,
		enum ConfigState state) {

	for (struct SList *i = heads; i; i = i->nex) {
		struct Head *head = i->val;
		if (head->zwlr_config_head) {
			zwlr_output_configuration_head_v1_destroy(head->zwlr_config_head);
			head->zwlr_config_head = NULL;
		}
	}

	zwlr_output_configuration_v1_destroy(zwlr_output_configuration_v1);

	displ->state = state;
}

static void succeeded(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	cleanup(data, zwlr_output_configuration_v1, SUCCEEDED);
}

static void failed(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	cleanup(data, zwlr_output_configuration_v1, FAILED);
}

static void cancelled(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
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


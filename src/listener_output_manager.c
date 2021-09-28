#include "info.h"
#include "listeners.h"
#include "types.h"

// OutputManager data

static void head(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
		struct zwlr_output_head_v1 *zwlr_output_head_v1) {
	struct OutputManager *output_manager = data;

	output_manager->dirty = true;

	struct Head *head = calloc(1, sizeof(struct Head));
	head->output_manager = output_manager;
	head->zwlr_head = zwlr_output_head_v1;

	slist_append(&output_manager->heads, head);
	slist_append(&output_manager->heads_arrived, head);

	zwlr_output_head_v1_add_listener(zwlr_output_head_v1, head_listener(), head);
}

static void done(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
		uint32_t serial) {
	struct OutputManager *output_manager = data;

	output_manager->serial = serial;
}

static void finished(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1) {
	struct OutputManager *output_manager = data;

	if (output_manager->displ) {
		output_manager->displ->output_manager = NULL;
	}
	free_output_manager(output_manager);

	zwlr_output_manager_v1_destroy(zwlr_output_manager_v1);
}

static const struct zwlr_output_manager_v1_listener listener = {
	.head = head,
	.done = done,
	.finished = finished,
};

const struct zwlr_output_manager_v1_listener *output_manager_listener() {
	return &listener;
}


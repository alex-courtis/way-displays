#include <stdio.h>
#include <stdlib.h>

#include "wlr-output-management-unstable-v1.h"

#include "listeners.h"
#include "types.h"

static void head(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
		struct zwlr_output_head_v1 *zwlr_output_head_v1) {
	struct wl_list *heads = data;

	struct Head *head = calloc(1, sizeof(*head));
	wl_list_init(&head->modes);

	wl_list_insert(heads->prev, &head->link);

	zwlr_output_head_v1_add_listener(zwlr_output_head_v1, head_listener(), head);
}

static void done(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
		uint32_t serial) {
	// todo: release
}

static void finished(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1) {
	// todo: release
}

static const struct zwlr_output_manager_v1_listener listener = {
	.head = head,
	.done = done,
	.finished = finished,
};

const struct zwlr_output_manager_v1_listener *output_manager_listener() {
	return &listener;
}


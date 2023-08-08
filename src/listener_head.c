#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>

#include "listeners.h"

#include "head.h"
#include "slist.h"
#include "mode.h"
#include "wlr-output-management-unstable-v1.h"

// Head data

static void name(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *name) {
	struct Head *head = data;

	head->name = strdup(name);
}

static void description(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *description) {
	struct Head *head = data;

	head->description = strdup(description);
}

static void physical_size(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t width,
		int32_t height) {
	struct Head *head = data;

	head->width_mm = width;
	head->height_mm = height;
}

static void mode(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	struct Head *head = data;

	struct Mode *mode = calloc(1, sizeof(struct Mode));
	mode->head = head;
	mode->zwlr_mode = zwlr_output_mode_v1;

	slist_append(&head->modes, mode);

	zwlr_output_mode_v1_add_listener(zwlr_output_mode_v1, mode_listener(), mode);
}

static void enabled(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t enabled) {
	struct Head *head = data;

	head->current.enabled = enabled;
}

static void current_mode(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	struct Head *head = data;

	struct Mode *mode = NULL;
	for (struct SList *i = head->modes; i; i = i->nex) {
		mode = i->val;
		if (mode && mode->zwlr_mode == zwlr_output_mode_v1) {
			head->current.mode = mode;
			break;
		}
	}
}

static void position(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t x,
		int32_t y) {
	struct Head *head = data;

	head->current.x = x;
	head->current.y = y;
}

static void transform(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t transform) {
	struct Head *head = data;

	head->transform = transform;
}

static void scale(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		wl_fixed_t scale) {
	struct Head *head = data;

	head->current.scale = scale;
}

static void make(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *make) {
	struct Head *head = data;

	head->make = strdup(make);
}

static void model(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *model) {
	struct Head *head = data;

	head->model = strdup(model);
}

static void serial_number(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *serial_number) {
	struct Head *head = data;

	head->serial_number = strdup(serial_number);
}

static void adaptive_sync(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		uint32_t state) {
	struct Head *head = data;

	head->current.adaptive_sync = state;
}

static void finished(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1) {
	struct Head *head = data;

	// dummy Head, just for printing
	struct Head *head_departed = calloc(1, sizeof(struct Head));
	head_departed->name = strdup(head->name);
	head_departed->description = strdup(head->description);
	slist_append(&heads_departed, head_departed);

	heads_release_head(head);
	head_free(head);

	zwlr_output_head_v1_destroy(zwlr_output_head_v1);
}

static const struct zwlr_output_head_v1_listener listener_min = {
	.name = name,
	.description = description,
	.physical_size = physical_size,
	.mode = mode,
	.enabled = enabled,
	.current_mode = current_mode,
	.position = position,
	.transform = transform,
	.scale = scale,
	.serial_number = serial_number,
	.model = model,
	.make = make,
	.adaptive_sync = adaptive_sync,
	.finished = finished,
};

static const struct zwlr_output_head_v1_listener listener = {
	.name = name,
	.description = description,
	.physical_size = physical_size,
	.mode = mode,
	.enabled = enabled,
	.current_mode = current_mode,
	.position = position,
	.transform = transform,
	.scale = scale,
	.serial_number = serial_number,
	.model = model,
	.make = make,
	.finished = finished,
	.adaptive_sync = adaptive_sync,
};

const struct zwlr_output_head_v1_listener *head_listener_min(void) {
	return &listener_min;
}

const struct zwlr_output_head_v1_listener *head_listener(void) {
	return &listener;
}


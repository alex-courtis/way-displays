#include <stdlib.h>
#include <string.h>

#include "wlr-output-management-unstable-v1.h"

#include "listeners.h"
#include "types.h"

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
	mode->zwlr_mode = zwlr_output_mode_v1;

	slist_append(&head->modes, mode);

	zwlr_output_mode_v1_add_listener(zwlr_output_mode_v1, mode_listener(), mode);
}

static void enabled(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t enabled) {
	struct Head *head = data;

	head->enabled = enabled;
}

static void current_mode(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	struct Head *head = data;

	head->zwlr_current_mode = zwlr_output_mode_v1;
}

static void position(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t x,
		int32_t y) {
	struct Head *head = data;

	head->x = x;
	head->y = y;
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

	head->scale = scale;
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

static void finished(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1) {
	// TODO release
}

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
};

const struct zwlr_output_head_v1_listener *head_listener() {
	return &listener;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wlr-output-management-unstable-v1.h"

#include "listener_mode.h"

// begin head listening
static void name(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *name) {
	printf("AMC name %s\n", name);
}

static void description(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *description) {
	printf("AMC description %s\n", description);
}

static void physical_size(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t width,
		int32_t height) {
	printf("AMC physical_size %dx%d\n", width, height);
}

static void mode(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		struct zwlr_output_mode_v1 *mode) {
	printf("AMC mode %p\n", (void*)mode);
	zwlr_output_mode_v1_add_listener(mode, mode_listener(), 0);
}

static void enabled(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t enabled) {
}

static void current_mode(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		struct zwlr_output_mode_v1 *mode) {
	printf("AMC current_mode %p\n", (void*)mode);
}

static void position(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t x,
		int32_t y) {
}

static void transform(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t transform) {
}

static void scale(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		wl_fixed_t scale) {
	printf("AMC scale %f\n", wl_fixed_to_double(scale));
}

static void finished(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1) {
	printf("AMC finished\n");
	// todo: release
}

static void make(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *make) {
}

static void model(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *model) {
}

static void serial_number(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *serial_number) {
}

static const struct zwlr_output_head_v1_listener head_listener = {
	.name = name,
	.description = description,
	.physical_size = physical_size,
	.mode = mode,
	.enabled = enabled,
	.current_mode = current_mode,
	.position = position,
	.transform = transform,
	.scale = scale,
	.finished = finished,
	.serial_number = serial_number,
	.model = model,
	.make = make,
};
// end head listening

// begin wlr listening
static void head(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
		struct zwlr_output_head_v1 *head) {
	zwlr_output_head_v1_add_listener(head, &head_listener, 0);
}

static void done(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1,
		uint32_t serial) {
	// todo: release
	printf("AMC done\n");
}

static void finito(void *data,
		struct zwlr_output_manager_v1 *zwlr_output_manager_v1) {
	// todo: release
	printf("AMC finito\n");
}

static const struct zwlr_output_manager_v1_listener output_manager_listener = {
	.head = head,
	.done = done,
	.finished = finito,
};
// end wlr listening

// begin wl listening
static void global(void *data,
		struct wl_registry *wl_registry,
		uint32_t name,
		const char *interface,
		uint32_t version) {
	if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0) {
		printf("AMC %s\n", interface);
		struct zwlr_output_manager_v1 *output_manager = wl_registry_bind(wl_registry, name, &zwlr_output_manager_v1_interface, 1);
		zwlr_output_manager_v1_add_listener(output_manager, &output_manager_listener, 0);
	}
}
static void global_remove(void *data,
		struct wl_registry *wl_registry,
		uint32_t name) {
	printf("AMC global remove\n");
	// todo: release
}

static const struct wl_registry_listener registry_listener = {
	.global = global,
	.global_remove = global_remove,
};
// end wl listening

int
main(int argc, const char **argv) {

	struct wl_display *display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "failed to connect to display\n");
		return EXIT_FAILURE;
	}

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, 0);

	wl_display_dispatch(display);

	wl_display_roundtrip(display);

	// todo
	/* if (state.output_manager == NULL) { */
	/* 	fprintf(stderr, "compositor doesn't support " */
	/* 		"wlr-output-management-unstable-v1\n"); */
	/* 	ret = EXIT_FAILURE; */
	/* 	goto done; */
	/* } */

	return EXIT_SUCCESS;
}


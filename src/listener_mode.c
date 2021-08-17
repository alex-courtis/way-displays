#include <stdio.h>

#include "wlr-output-management-unstable-v1.h"

#include "listener_mode.h"

static void size(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1,
		int32_t width,
		int32_t height) {
	printf("AMC %p size %dx%d\n", (void*)zwlr_output_mode_v1, width, height);
}

static void refresh(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1,
		int32_t refresh) {
	printf("AMC %p refresh %d\n", (void*)zwlr_output_mode_v1, refresh);
}

static void preferred(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	printf("AMC %p preferred\n", (void*)zwlr_output_mode_v1);
}

static void fini(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	printf("AMC %p fini\n", (void*)zwlr_output_mode_v1);
	// todo: release
}

static const struct zwlr_output_mode_v1_listener listener = {
	.size = size,
	.refresh = refresh,
	.preferred = preferred,
	.finished = fini,
};

const struct zwlr_output_mode_v1_listener *mode_listener() {
	return &listener;
}


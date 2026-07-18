#include <stdint.h>

#include "listeners.h"

#include "head.h"
#include "mode.h"
#include "ppmap.h"
#include "wlr-output-management-unstable-v1.h"

// Head data

static void size(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1,
		int32_t width,
		int32_t height) {
	const struct Head *head = data;

	struct Mode *mode = (struct Mode*)ppmap_get(head->modes, zwlr_output_mode_v1);

	if (mode) {
		mode->width = width;
		mode->height = height;
	}
}

static void refresh(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1,
		int32_t refresh_mhz) {
	const struct Head *head = data;

	struct Mode *mode = (struct Mode*)ppmap_get(head->modes, zwlr_output_mode_v1);

	if (mode) {
		mode->refresh_mhz = refresh_mhz;
	}
}

static void preferred(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {

	head_set_mode_pref(data, zwlr_output_mode_v1);
}

static void finished(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	head_release_mode(data, zwlr_output_mode_v1);

	zwlr_output_mode_v1_destroy(zwlr_output_mode_v1);
}

static const struct zwlr_output_mode_v1_listener listener = {
	.size = size,
	.refresh = refresh,
	.preferred = preferred,
	.finished = finished,
};

const struct zwlr_output_mode_v1_listener *zwlr_output_mode_listener(void) {
	return &listener;
}


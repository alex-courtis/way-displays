#include <stdbool.h>
#include <stdint.h>

#include "listeners.h"

#include "head.h"
#include "info.h"
#include "log.h"
#include "mode.h"
#include "wlr-output-management-unstable-v1.h"

// Mode data

static void size(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1,
		int32_t width,
		int32_t height) {
	struct Mode *mode = data;

	mode->width = width;
	mode->height = height;
}

static void refresh(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1,
		int32_t refresh) {
	struct Mode *mode = data;

	mode->refresh_mhz = refresh;
}

static void preferred(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	struct Mode *mode = data;

	// some heads may advertise multiple preferred modes; ignore subsequent
	if (mode->head) {
		struct Mode *preferred_mode;
		if ((preferred_mode = head_preferred_mode(mode->head))) {
			static char mode_buf[2048];
			info_mode_string(mode, mode_buf, sizeof(mode_buf));
			static char preferred_mode_buf[2048];
			info_mode_string(preferred_mode, preferred_mode_buf, sizeof(preferred_mode_buf));
			log_warn("\n%s already specified for '%s', ignoring %s", mode_buf, mode->head->name, preferred_mode_buf);
			return;
		}
	}

	mode->preferred = true;
}

static void finished(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	struct Mode *mode = data;

	head_release_mode(mode->head, mode);
	mode_free(mode);

	zwlr_output_mode_v1_destroy(zwlr_output_mode_v1);
}

static const struct zwlr_output_mode_v1_listener listener = {
	.size = size,
	.refresh = refresh,
	.preferred = preferred,
	.finished = finished,
};

const struct zwlr_output_mode_v1_listener *mode_listener(void) {
	return &listener;
}


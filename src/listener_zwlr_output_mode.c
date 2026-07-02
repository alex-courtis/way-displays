#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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
	struct WlrMode *wlr_mode = data;

	wlr_mode->width = width;
	wlr_mode->height = height;
}

static void refresh(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1,
		int32_t refresh) {
	struct WlrMode *wlr_mode = data;

	wlr_mode->refresh_mhz = refresh;
}

static void preferred(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	struct WlrMode *wlr_mode = data;

	// some heads may advertise multiple preferred modes; ignore subsequent
	if (wlr_mode->head) {
		const struct WlrMode *wlr_mode_pref;
		if ((wlr_mode_pref = head_preferred_wlr_mode(wlr_mode->head))) {

			char *mode_preferred_str = info_wlr_mode_string(wlr_mode_pref);

			char *mode_str = info_wlr_mode_string(wlr_mode);

			if (wlr_mode_pref->head && wlr_mode_pref->head->name) {
				log_info(NULL);
				log_info("%s: multiple preferred modes advertised: using initial %s, ignoring %s", wlr_mode_pref->head->name, mode_preferred_str, mode_str);
			} else {
				log_info(NULL);
				log_info("???: multiple preferred modes advertised: using initial %s, ignoring %s", mode_preferred_str, mode_str);
			}

			free(mode_preferred_str);
			free(mode_str);

			return;
		}
	}

	wlr_mode->preferred = true;
}

static void finished(void *data,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	if (!data)
		return;

	struct WlrMode *wlr_mode = data;

	head_release_mode(wlr_mode);

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


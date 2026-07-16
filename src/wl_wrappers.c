#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "wl_wrappers.h"
#include "wlr-output-management-unstable-v1.h"

#include "displ.h"
#include "listeners.h"
#include "log.h"
#include "process.h"

int _wl_display_prepare_read(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_prepare_read(display)) == -1) {
		if (errno != EAGAIN) {
			log_fatal(NULL);
			log_fatal_errno("wl_display_prepare_read failed at %s:%d, exiting", file, line);
			wd_exit_message(EXIT_FAILURE);
		}
	}

	return ret;
}

int _wl_display_dispatch_pending__read_events(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_dispatch_pending(display)) == -1) {
		log_fatal(NULL);
		log_fatal_errno("wl_display_dispatch_pending for read_events failed at %s:%d, exiting", file, line);
		wd_exit_message(EXIT_FAILURE);
	}

	return ret;
}

int _wl_display_dispatch_pending__prepare_read(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_dispatch_pending(display)) == -1) {
		log_fatal(NULL);
		log_fatal_errno("wl_display_dispatch_pending for prepare_read failed at %s:%d, exiting", file, line);
		wd_exit_message(EXIT_FAILURE);
	}

	return ret;
}

int _wl_display_flush(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_flush(display)) == -1) {
		log_fatal(NULL);
		log_fatal_errno("wl_display_flush failed at %s:%d, exiting", file, line);
		wd_exit_message(EXIT_FAILURE);
	}

	return ret;
}

int _wl_display_read_events(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_read_events(display)) == -1) {
		if (errno == EPIPE) {
			log_info(NULL);
			log_info("Wayland display terminated, exiting.");
		} else {
			log_fatal(NULL);
			log_fatal_errno("wl_display_read_events failed at %s:%d, exiting", file, line);
			wd_exit_message(EXIT_FAILURE);
		}
	}

	return ret;
}

struct zwlr_output_configuration_v1 *create_zwlr_output_config_listener(struct Displ *displ) {
	struct zwlr_output_configuration_v1 *zwlr_config = zwlr_output_manager_v1_create_configuration(displ->zwlr_output_manager, displ->zwlr_output_manager_serial);
	zwlr_output_configuration_v1_add_listener(zwlr_config, zwlr_output_configuration_listener(), displ);

	return zwlr_config;
}

struct zwlr_output_configuration_head_v1 * _zwlr_output_configuration_v1_enable_head(struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1, struct zwlr_output_head_v1 *head) {
	return zwlr_output_configuration_v1_enable_head(zwlr_output_configuration_v1, head);
}

void _zwlr_output_configuration_v1_disable_head(struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1, struct zwlr_output_head_v1 *head) {
	zwlr_output_configuration_v1_disable_head(zwlr_output_configuration_v1, head);
}

void _zwlr_output_configuration_v1_apply(struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	zwlr_output_configuration_v1_apply(zwlr_output_configuration_v1);
}

void _zwlr_output_configuration_head_v1_set_mode(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, struct zwlr_output_mode_v1 *mode) {
	zwlr_output_configuration_head_v1_set_mode(zwlr_output_configuration_head_v1, mode);
}

void _zwlr_output_configuration_head_v1_set_transform(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, int32_t transform) {
	zwlr_output_configuration_head_v1_set_transform(zwlr_output_configuration_head_v1, transform);
}

void _zwlr_output_configuration_head_v1_set_scale(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, int32_t scale) {
	zwlr_output_configuration_head_v1_set_scale(zwlr_output_configuration_head_v1, scale);
}

void _zwlr_output_configuration_head_v1_set_position(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, int32_t x, int32_t y) {
	zwlr_output_configuration_head_v1_set_position(zwlr_output_configuration_head_v1, x, y);
}

void _zwlr_output_configuration_head_v1_set_adaptive_sync(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, uint32_t state) {
	zwlr_output_configuration_head_v1_set_adaptive_sync(zwlr_output_configuration_head_v1, state);
}


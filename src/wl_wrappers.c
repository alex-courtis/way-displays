#include <errno.h>
#include <stdlib.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "wl_wrappers.h"

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


#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "wl_wrappers.h"

int _wl_display_prepare_read(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_prepare_read(display)) == -1) {
		if (errno != EAGAIN) {
			log_error("\nwl_display_prepare_read failed %d: '%s' at %s:%d, exiting\n", errno, strerror(errno), file, line);
			exit(EXIT_FAILURE);
		}
	}

	return ret;
}

int _wl_display_dispatch_pending(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_dispatch_pending(display)) == -1) {
		log_error("\nwl_display_dispatch_pending failed %d: '%s' at %s:%d, exiting\n", errno, strerror(errno), file, line);
		exit(EXIT_FAILURE);
	}

	return ret;
}

int _wl_display_flush(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_flush(display)) == -1) {
		log_error("\nwl_display_flush failed %d: '%s' at %s:%d, exiting\n", errno, strerror(errno), file, line);
		exit(EXIT_FAILURE);
	}

	return ret;
}

int _wl_display_read_events(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_read_events(display)) == -1) {
		if (errno == EPIPE) {
			log_info("\nWayland display terminated, exiting.\n");
			exit(EXIT_SUCCESS);
		} else {
			log_error("\nwl_display_read_events failed %d: '%s' at %s:%d, exiting\n", errno, strerror(errno), file, line);
			exit(EXIT_FAILURE);
		}
	}

	return ret;
}


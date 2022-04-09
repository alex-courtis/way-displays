#include <errno.h>
#include <stdlib.h>
#include <wayland-client-core.h>

#include "wl_wrappers.h"

#include "log.h"
#include "process.h"

int _wl_display_prepare_read(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_prepare_read(display)) == -1) {
		if (errno != EAGAIN) {
			log_efl_(ERROR, errno, file, line, "\nwl_display_prepare_read failed, exiting");
			exit_fail();
		}
	}

	return ret;
}

int _wl_display_dispatch_pending(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_dispatch_pending(display)) == -1) {
		log_efl_(ERROR, errno, file, line, "\nwl_display_dispatch_pending failed, exiting");
		exit_fail();
	}

	return ret;
}

int _wl_display_flush(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_flush(display)) == -1) {
		log_efl_(ERROR, errno, file, line, "\nwl_display_flush failed, exiting");
		exit_fail();
	}

	return ret;
}

int _wl_display_read_events(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_read_events(display)) == -1) {
		if (errno == EPIPE) {
			log_info("\nWayland display terminated, exiting.");
			exit(EXIT_SUCCESS);
		} else {
			log_efl_(ERROR, errno, file, line, "\nwl_display_read_events failed, exiting");
			exit_fail();
		}
	}

	return ret;
}


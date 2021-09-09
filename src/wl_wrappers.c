#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "wl_wrappers.h"

// TODO file and line for all of these error messages
int checked_wl_display_prepare_read(struct wl_display *display) {
	static int ret;

	if ((ret = wl_display_prepare_read(display)) == -1) {
		if (errno != EAGAIN) {
			fprintf(stderr, "ERROR: wl_display_prepare_read failed %d, exiting\n", errno);
			exit(EX_SOFTWARE);
		}
	}

	return ret;
}

int checked_wl_display_dispatch_pending(struct wl_display *display) {
	static int ret;

	if ((ret = wl_display_dispatch_pending(display)) == -1) {
		fprintf(stderr, "ERROR: wl_display_dispatch_pending failed %d, exiting\n", errno);
		exit(EX_SOFTWARE);
	}

	return ret;
}

int checked_wl_display_flush(struct wl_display *display) {
	static int ret;

	if ((ret = wl_display_flush(display)) == -1) {
		fprintf(stderr, "ERROR: wl_display_flush failed %d, exiting\n", errno);
		exit(EX_SOFTWARE);
	}

	return ret;
}

int checked_wl_display_read_events(struct wl_display *display) {
	static int ret;

	if ((ret = wl_display_read_events(display)) == -1) {
		fprintf(stderr, "ERROR: wl_display_read_events failed %d, exiting\n", errno);
		exit(EX_SOFTWARE);
	}

	return ret;
}

int checked_wl_display_cancel_read(struct wl_display *display) {
	static int ret;

	if ((ret = wl_display_read_events(display)) == -1) {
		fprintf(stderr, "ERROR: wl_display_cancel_read failed %d, exiting\n", errno);
		exit(EX_SOFTWARE);
	}

	return ret;
}

struct wl_display *checked_wl_display_connect(const char *name) {
	struct wl_display *display;

	if (!(display = wl_display_connect(name))) {
		// from swaybar/swaynag
		fprintf(stderr, "Unable to connect to the compositor. "
				"If your compositor is running, check or set the "
				"WAYLAND_DISPLAY environment variable.\n");
		exit(EX_SOFTWARE);
	}

	return display;
}


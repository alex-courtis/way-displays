#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "wl_wrappers.h"

int _wl_display_prepare_read(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_prepare_read(display)) == -1) {
		if (errno != EAGAIN) {
			fprintf(stderr, "\nERROR: wl_display_prepare_read failed %d: '%s' at %s:%d, exiting\n", errno, strerror(errno), file, line);
			exit(EX_SOFTWARE);
		}
	}

	return ret;
}

int _wl_display_dispatch_pending(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_dispatch_pending(display)) == -1) {
		fprintf(stderr, "\nERROR: wl_display_dispatch_pending failed %d: '%s' at %s:%d, exiting\n", errno, strerror(errno), file, line);
		exit(EX_SOFTWARE);
	}

	return ret;
}

int _wl_display_flush(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_flush(display)) == -1) {
		fprintf(stderr, "\nERROR: wl_display_flush failed %d: '%s' at %s:%d, exiting\n", errno, strerror(errno), file, line);
		exit(EX_SOFTWARE);
	}

	return ret;
}

int _wl_display_read_events(struct wl_display *display, char *file, int line) {
	static int ret;

	if ((ret = wl_display_read_events(display)) == -1) {
		if (errno == EPIPE) {
			printf("\nWayland display terminated, exiting.\n");
			exit(EXIT_SUCCESS);
		} else {
			fprintf(stderr, "\nERROR: wl_display_read_events failed %d: '%s' at %s:%d, exiting\n", errno, strerror(errno), file, line);
			exit(EX_SOFTWARE);
		}
	}

	return ret;
}


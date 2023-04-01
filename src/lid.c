#include <errno.h>
#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "lid.h"

#include "cfg.h"
#include "global.h"
#include "log.h"

static const char *LAPTOP_DISPLAY_PREFIX_DEFAULT = "eDP";

static int libinput_open_restricted(const char *path, int flags, void *data) {

	// user permissions are sufficient for input devices, no need for systemd
	int fd = open(path, flags);

	if (fd <= 0) {
		log_warn_errno("\nlibinput open %s failed", path, errno);
		return -errno;
	}

	return fd;
}

static void libinput_close_restricted(int fd, void *data) {

	if (close(fd) != 0) {
		log_warn_errno("\nlibinput close failed");
	}
}

static const struct libinput_interface libinput_impl = {
	.open_restricted = libinput_open_restricted,
	.close_restricted = libinput_close_restricted
};

struct libinput *create_libinput_discovery(void) {
	struct libinput *libinput = NULL;

	struct udev *udev = udev_new();
	if (!udev) {
		log_error("\nunable to create udev context, abandoning laptop lid detection");
		return NULL;
	}

	libinput = libinput_udev_create_context(&libinput_impl, NULL, udev);
	if (!libinput) {
		log_error("\nunable to create libinput discovery context, abandoning laptop lid detection");
		return NULL;
	}

	libinput_set_user_data(libinput, udev);

	const char *xdg_seat = getenv("XDG_SEAT");
	if (!xdg_seat) {
		xdg_seat = "seat0";
	}

	if (libinput_udev_assign_seat(libinput, xdg_seat) != 0) {
		log_error("\nfailed to assign seat to libinput, abandoning laptop lid detection");
		return NULL;
	}

	return libinput;
}

void destroy_libinput_discovery(struct libinput *libinput) {
	if (!libinput)
		return;

	udev_unref(libinput_get_user_data(libinput));

	libinput_suspend(libinput);

	libinput_unref(libinput);
}

char *discover_lid_device(struct libinput *libinput) {
	char *device_path = NULL;

	struct libinput_event *event;

	libinput_dispatch(libinput);
	while ((event = libinput_get_event(libinput))) {
		struct libinput_device *device = libinput_event_get_device(event);

		if (device && !device_path &&
				libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_SWITCH) &&
				(libinput_device_switch_has_switch(device, LIBINPUT_SWITCH_LID) == 1)) {
			device_path = calloc(PATH_MAX, sizeof(char));
			snprintf(device_path, PATH_MAX, "/dev/input/%s", libinput_device_get_sysname(device));
		}

		libinput_event_destroy(event);
		libinput_dispatch(libinput);
	}

	return device_path;
}

struct libinput *create_libinput_monitor(char *device_path) {

	struct libinput *libinput_context = libinput_path_create_context(&libinput_impl, NULL);
	if (!libinput_context) {
		log_error("\nunable to create libinput monitoring context, abandoning laptop lid detection");
		return NULL;
	}

	struct libinput_device *device = libinput_path_add_device(libinput_context, device_path);
	if (!device) {
		log_error("\nunable to add libinput path device %s, abandoning laptop lid detection", device_path);
		return NULL;
	}

	return libinput_context;
}

void destroy_libinput_monitor(struct libinput* libinput) {
	if (!libinput)
		return;

	libinput_suspend(libinput);

	libinput_unref(libinput);
}

void lid_destroy(void) {
	if (!lid)
		return;

	destroy_libinput_monitor(lid->libinput_monitor);

	free(lid->device_path);

	free(lid);
	lid = NULL;
}

void lid_update(void) {
	if (!lid || !lid->libinput_monitor)
		return;

	struct libinput_event *event;

	libinput_dispatch(lid->libinput_monitor);
	while ((event = libinput_get_event(lid->libinput_monitor))) {
		enum libinput_event_type event_type = libinput_event_get_type(event);

		if (event_type == LIBINPUT_EVENT_SWITCH_TOGGLE) {
			struct libinput_event_switch *event_switch = libinput_event_get_switch_event(event);
			lid->closed = libinput_event_switch_get_switch_state(event_switch) == LIBINPUT_SWITCH_STATE_ON;
		}

		libinput_event_destroy(event);
		libinput_dispatch(lid->libinput_monitor);
	}

	log_info("\nLid %s", lid->closed ? "closed" : "open");
}

void lid_init(void) {
	lid = NULL;
	struct libinput *libinput_discovery = NULL;
	struct libinput *libinput_monitor = NULL;
	char *device_path = NULL;

	// discover with a context of all inputs
	if ((libinput_discovery = create_libinput_discovery())) {

		device_path = discover_lid_device(libinput_discovery);

		destroy_libinput_discovery(libinput_discovery);

		if (!device_path) {
			return;
		}
	} else {
		log_warn("Unable to start libinput discovery for lid device");
		return;
	}

	// monitor in a context with just the lid
	if (!(libinput_monitor = create_libinput_monitor(device_path))) {
		log_warn("Unable to create libinput monitor for lid device %s", device_path);
		return;
	}

	log_info("\nMonitoring lid device: %s", device_path);

	lid = calloc(1, sizeof(struct Lid));
	lid->device_path = device_path;
	lid->libinput_fd = libinput_get_fd(libinput_monitor);
	lid->libinput_monitor = libinput_monitor;
}

bool lid_is_closed(char *name) {
	if (!name)
		return false;

	if (!lid)
		return false;

	const char *laptop_display_prefix;
	if (cfg->laptop_display_prefix) {
		laptop_display_prefix = cfg->laptop_display_prefix;
	} else {
		laptop_display_prefix = LAPTOP_DISPLAY_PREFIX_DEFAULT;
	}

	if (strncasecmp(laptop_display_prefix, name, strlen(laptop_display_prefix)) == 0) {
		return lid->closed;
	} else {
		return false;
	}
}


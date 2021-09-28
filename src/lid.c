#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <libinput.h>

#include "lid.h"

static int libinput_open_restricted(const char *path, int flags, void *data) {
	fprintf(stderr, "open_restricted %s\n", path);

	// user permissions are sufficient for input devices, no need for systemd
	int fd = open(path, flags);

	if (fd <= 0) {
		fprintf(stderr, "\nWARNING: open '%s' failed %d: '%s'\n", path, errno, strerror(errno));
	}

	return fd;
}

static void libinput_close_restricted(int fd, void *data) {
	fprintf(stderr, "close_restricted %d\n", fd);

	if (close(fd) != 0) {
		fprintf(stderr, "\nWARNING: close failed %d: '%s'\n", errno, strerror(errno));
	}
}

static const struct libinput_interface libinput_impl = {
	.open_restricted = libinput_open_restricted,
	.close_restricted = libinput_close_restricted
};

struct libinput *create_libinput_discovery() {

	struct udev *udev = udev_new();
	if (!udev) {
		fprintf(stderr, "\nERROR: unable to create udev context, abandoning laptop lid detection\n");
		return NULL;
	}

	struct libinput *libinput_context = libinput_udev_create_context(&libinput_impl, NULL, udev);
	if (!libinput_context) {
		fprintf(stderr, "\nERROR: unable to create libinput discovery context, abandoning laptop lid detection\n");
		return NULL;
	}

	const char *xdg_seat = getenv("XDG_SEAT");
	if (!xdg_seat) {
		xdg_seat = "seat0";
	}

	if (libinput_udev_assign_seat(libinput_context, xdg_seat) != 0) {
		fprintf(stderr, "\nERROR: failed to assign seat to libinput, abandoning laptop lid detection\n");
		return NULL;
	}

	return libinput_context;
}

char *discover_lid_device(struct libinput *libinput) {

	if (libinput_dispatch(libinput) != 0) {
		fprintf(stderr, "\nERROR: failed to dispatch libinput, abandoning laptop lid detection\n");
		return NULL;
	}

	char *device_path = NULL;
	struct libinput_event *event;

	while ((event = libinput_get_event(libinput))) {

		struct libinput_device *device = libinput_event_get_device(event);

		if (device && libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_SWITCH)) {
			device_path = calloc(PATH_MAX, sizeof(char));
			snprintf(device_path, PATH_MAX, "/dev/input/%s", libinput_device_get_sysname(device));
		}

		libinput_event_destroy(event);
	}

	return device_path;
}

struct libinput *create_libinput_monitor(char *device_path) {

	struct libinput *libinput_context = libinput_path_create_context(&libinput_impl, NULL);
	if (!libinput_context) {
		fprintf(stderr, "\nERROR: unable to create libinput monitoring context, abandoning laptop lid detection\n");
		return NULL;
	}

	struct libinput_device *device = libinput_path_add_device(libinput_context, device_path);
	if (!device) {
		fprintf(stderr, "\nERROR: unable to add libinput path device %s, abandoning laptop lid detection\n", device_path);
		return NULL;
	}

	return libinput_context;
}

void destroy_lid(struct Displ *displ) {
	struct SList *i;
	struct Head *head;
	if (!displ || !displ->lid)
		return;

	libinput_suspend(displ->lid->libinput_monitor);
	libinput_unref(displ->lid->libinput_monitor);

	free_lid(displ->lid);
	displ->lid = NULL;

	displ->npfds -= 1;
	displ->pfds = realloc(displ->pfds, sizeof(struct pollfd) * displ->npfds);

	if (displ->output_manager) {
		for (i = displ->output_manager->heads; i; i = i->nex) {
			head = i->val;
			if (!head)
				continue;

			head->lid_closed = false;
			head->dirty = true;
		}
	}
}

void update_lid(struct Displ *displ) {
	struct libinput_event *event;
	struct libinput_event_switch *event_switch;
	enum libinput_event_type event_type;
	enum libinput_switch_state switch_state;

	if (!displ || !displ->lid || !displ->lid->libinput_monitor)
		return;

	bool new_closed = displ->lid->closed;

	if (libinput_dispatch(displ->lid->libinput_monitor) < 0) {
		fprintf(stderr, "\nERROR: unable to dispatch libinput %d: '%s', abandoning laptop lid detection\n", errno, strerror(errno));
		destroy_lid(displ);
		return;
	}

	while ((event = libinput_get_event(displ->lid->libinput_monitor))) {
		event_type = libinput_event_get_type(event);

		if (event_type == LIBINPUT_EVENT_SWITCH_TOGGLE) {
			event_switch = libinput_event_get_switch_event(event);
			switch_state = libinput_event_switch_get_switch_state(event_switch);
			new_closed = switch_state == LIBINPUT_SWITCH_STATE_ON;
		}

		libinput_event_destroy(event);
	}

	displ->lid->dirty = new_closed != displ->lid->closed;
	displ->lid->closed = new_closed;
}

struct Lid *create_lid() {
	struct Lid *lid	= NULL;
	struct libinput *libinput_discovery = NULL;
	char *device_path = NULL;

	fprintf(stderr, "create_lid 0\n");

	// discover with a context of all inputs
	if ((libinput_discovery = create_libinput_discovery())) {
		fprintf(stderr, "create_lid 1\n");

		device_path = discover_lid_device(libinput_discovery);

		libinput_suspend(libinput_discovery);

		libinput_unref(libinput_discovery);

		if (!device_path) {
			fprintf(stderr, "create_lid 2 early end\n");
			return NULL;
		}
	} else {
		return NULL;
	}

	// provisional lid
	lid = calloc(1, sizeof(struct Lid));

	// monitor in a context with just the lid
	if ((lid->libinput_monitor = create_libinput_monitor(device_path)) == 0) {
		free_lid(lid);
		return NULL;
	}
	lid->libinput_fd = libinput_get_fd(lid->libinput_monitor);

	printf("\nMonitoring lid device: %s\n", device_path);

	return lid;
}

void update_heads_lid_closed(struct Displ *displ) {
	struct Head *head;
	struct SList *i;

	if (!displ || !displ->lid || !displ->output_manager || !displ->cfg || !displ->cfg->laptop_display_prefix)
		return;

	for (i = displ->output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		if (strncasecmp(displ->cfg->laptop_display_prefix, head->name, strlen(displ->cfg->laptop_display_prefix)) == 0) {
			if (head->lid_closed != displ->lid->closed) {
				head->lid_closed = displ->lid->closed;
				head->dirty = true;
			}
		}
	}
}


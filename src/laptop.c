#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libinput.h>

#include "laptop.h"

static int libinput_open_restricted(const char *path, int flags, void *data) {
	fprintf(stderr, "open_restricted %s\n", path);

	// this could be done with proper permissions by, say, libseat
	int fd = open(path, flags);

	if (fd <= 0) {
		// TODO give up on lid detection
		fprintf(stderr, "failed to open '%s' %d %s\n", path, errno, strerror(errno));
		exit(1);
	}

	return fd;
}

static void libinput_close_restricted(int fd, void *data) {
	fprintf(stderr, "close_restricted %d\n", fd);

	if (close(fd) != 0) {
		// TODO give up on lid detection
		fprintf(stderr, "failed to close %d %d %s\n", fd, errno, strerror(errno));
		exit(1);
	}
}

static const struct libinput_interface libinput_impl = {
	.open_restricted = libinput_open_restricted,
	.close_restricted = libinput_close_restricted
};

struct libinput *create_libinput_discovery_context() {

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
		fprintf(stderr, "\nERROR: $XDG_SEAT not set, abandoning laptop lid detection\n");
		return NULL;
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
		if (!device)
			continue;

		if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_SWITCH)) {
			device_path = calloc(PATH_MAX, sizeof(char));
			snprintf(device_path, PATH_MAX, "/dev/input/%s", libinput_device_get_sysname(device));
		}
	}

	return device_path;
}

struct libinput *create_libinput_monitor_context(char *device_path) {

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

void update_lid(struct Lid *lid) {
	if (!lid || !lid->libinput_monitor)
		return;

	bool new_closed = lid->closed;

	fprintf(stderr, "update_lid begin closed=%d dirty=%d\n", lid->closed, lid->dirty);

	// TODO wrap
	libinput_dispatch(lid->libinput_monitor);
	struct libinput_event *event;
	while ((event = libinput_get_event(lid->libinput_monitor))) {
		struct libinput_device *device = libinput_event_get_device(event);
		fprintf(stderr, "update_lid event %s\n", libinput_device_get_name(device));
		enum libinput_event_type event_type = libinput_event_get_type(event);
		if (event_type == LIBINPUT_EVENT_SWITCH_TOGGLE) {
			fprintf(stderr, "update_lid switch toggle\n");
			struct libinput_event_switch *event_switch = libinput_event_get_switch_event(event);
			enum libinput_switch_state switch_state = libinput_event_switch_get_switch_state(event_switch);
			new_closed = switch_state == LIBINPUT_SWITCH_STATE_ON;
		} else if (event_type == LIBINPUT_EVENT_DEVICE_ADDED) {
			fprintf(stderr, "update_lid added\n");
		} else {
			fprintf(stderr, "update_lid ???\n");
		}
	}

	lid->dirty = new_closed != lid->closed;
	lid->closed = new_closed;

	fprintf(stderr, "update_lid end closed=%d dirty=%d\n", lid->closed, lid->dirty);
}

struct Lid *create_lid() {
	struct Lid *lid	= NULL;
	fprintf(stderr, "create_lid 0\n");

	struct libinput *libinput_discovery = create_libinput_discovery_context();

	if (libinput_discovery) {
		char *device_path = discover_lid_device(libinput_discovery);

		if (!device_path) {
			return NULL;
		}

		lid = calloc(1, sizeof(struct Lid));

		// TODO there might be more to destroy
		libinput_suspend(libinput_discovery);

		if ((lid->libinput_monitor = create_libinput_monitor_context(device_path)) == 0) {
			return NULL;
		}
		lid->libinput_fd = libinput_get_fd(lid->libinput_monitor);

		update_lid(lid);
	}

	fprintf(stderr, "create_lid 99\n");
	return lid;
}

bool closed_laptop_display(const char *name, struct Cfg *cfg) {
	if (cfg && cfg->laptop_display_prefix && cfg->laptop_lid_path) {
		return
			strncasecmp(cfg->laptop_display_prefix, name, strlen(cfg->laptop_display_prefix)) == 0 &&
			laptop_lid_closed(cfg->laptop_lid_path);
	} else {
		return false;
	}
}

bool laptop_lid_closed(const char *root_path) {
	static char lidFileName[PATH_MAX];
	static char line[512];
	static bool closed;

	if (!root_path)
		return false;

	// find the lid state directory
	DIR *dir = opendir(root_path);
	if (dir) {
		struct dirent *dirent;
		while ((dirent = readdir(dir)) != NULL) {
			if (dirent->d_type == DT_DIR && strcmp(dirent->d_name, ".") != 0 && strcmp(dirent->d_name, "..") != 0) {

				// read the lid state file
				snprintf(lidFileName, PATH_MAX, "%s/%s/%s", root_path, dirent->d_name, "state");
				FILE *lidFile = fopen(lidFileName, "r");
				if (lidFile) {
					if (fgets(line, 512, lidFile)) {
						closed = strcasestr(line, "closed");
					}
					fclose(lidFile);
				}

				// drivers/acpi/button.c acpi_button_add_fs seems to indicate there will be only one file
				break;
			}
		}
		closedir(dir);
	}
	return closed;
}


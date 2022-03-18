#ifndef LID_H
#define LID_H

#include <stdbool.h>

struct Lid {
	bool closed;

	char *device_path;
	struct libinput *libinput_monitor;
	int libinput_fd;
};

void lid_init(void);

void lid_update(void);

bool lid_is_closed(char *name);

void lid_destroy(void);

#endif // LID_H


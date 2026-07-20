#ifndef LID_H
#define LID_H

#include <stdbool.h>

// global singleton
extern struct Lid *g_lid;

struct Lid {
	bool closed;

	char *device_path;
	struct libinput *libinput_monitor;
	int libinput_fd;
};

void g_lid_init(void);

void g_lid_update(void);

bool g_lid_is_closed(char *name);

void g_lid_destroy(void);

void lid_free(struct Lid *lid);

#endif // LID_H


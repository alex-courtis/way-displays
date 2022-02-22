#ifndef LID_H
#define LID_H

#include <stdbool.h>

#include "types.h"

struct Lid {
	bool closed;

	bool dirty;

	char *device_path;
	struct libinput *libinput_monitor;
	int libinput_fd;
};

struct Lid *create_lid();

void destroy_lid(struct Displ *displ);

bool update_lid(struct Displ *displ);

void update_heads_lid_closed(struct Displ *displ);

void free_lid(struct Lid *lid);

#endif // LID_H


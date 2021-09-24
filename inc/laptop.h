#ifndef LAPTOP_H
#define LAPTOP_H

#include "types.h"

struct Lid *create_lid();

void update_lid(struct Lid *lid);

void update_heads_lid_closed(struct Displ *displ);

bool lid_closed(char *name, struct Displ *displ);

#endif // LAPTOP_H


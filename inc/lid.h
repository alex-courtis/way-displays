#ifndef LAPTOP_H
#define LAPTOP_H

#include "types.h"

struct Lid *create_lid();

void update_lid(struct Displ *displ);

void update_heads_lid_closed(struct Displ *displ);

#endif // LAPTOP_H


#ifndef DISPL_H
#define DISPL_H

#include "types.h"

void connect_display(struct Displ *displ);

void destroy_display(struct Displ *displ);

bool consume_arrived_departed(struct OutputManager *output_manager);

#endif // DISPL_H

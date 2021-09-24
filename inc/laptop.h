#ifndef LAPTOP_H
#define LAPTOP_H

#include "types.h"

bool closed_laptop_display(const char *name, struct Cfg *cfg);

bool laptop_lid_closed(const char *root_path);

struct Lid *create_lid();

void update_lid(struct Lid *lid);

#endif // UTIL_H


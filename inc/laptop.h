#ifndef LAPTOP_H
#define LAPTOP_H

#include <stdbool.h>

#include "types.h"

bool closed_laptop_display(const char *name, struct Cfg *cfg);

bool laptop_lid_closed(const char *root_path);

#endif // UTIL_H


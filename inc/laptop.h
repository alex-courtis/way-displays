#ifndef LAPTOP_H
#define LAPTOP_H

#include <stdbool.h>

bool laptop_lid_closed();

bool laptop_lid_closed_path(const char *root_path);

#endif // UTIL_H


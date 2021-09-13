#ifndef INFO_H
#define INFO_H

#include "types.h"

enum event {
	ARRIVED,
	DEPARTED,
	DELTA,
};

void print_heads(enum event event, struct SList *heads);

#endif // INFO_H


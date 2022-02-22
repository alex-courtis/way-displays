#ifndef INFO_H
#define INFO_H

#include "cfg.h"
#include "list.h"

enum event {
	ARRIVED,
	DEPARTED,
	DELTA,
	NONE,
};

void print_cfg(struct Cfg *cfg);

void print_heads(enum event event, struct SList *heads);

#endif // INFO_H


#ifndef DELTA_H
#define DELTA_H

#include "head.h"
#include "pset.h"

// all changes between desired and current
char *delta_human(const struct Pset * const heads);

// mode changes
char *delta_human_mode(const struct Head * const head);

// vrr changes
char *delta_human_adaptive_sync(const struct Head * const head);

// reapply disabling message
char *delta_human_reapply(const struct Head * const head);

#endif // DELTA_H


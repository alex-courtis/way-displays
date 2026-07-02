#ifndef UTIL_INIT_H
#define UTIL_INIT_H

#include "head.h"

struct Head *head_init_name(const char *name);

struct Head *head_init_description(const char *description);

struct WlrMode *wlr_mode_init_empty(void);

struct WlrMode *wlr_mode_init_head(struct Head *head);

#endif // UTIL_INIT_H

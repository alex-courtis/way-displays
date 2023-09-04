#ifndef INFO_H
#define INFO_H

#include <stdbool.h>
#include <stddef.h>

#include "cfg.h"
#include "head.h"
#include "slist.h"
#include "log.h"
#include "mode.h"

enum InfoEvent {
	ARRIVED,
	DEPARTED,
	DELTA,
	NONE,
};

void print_cfg(enum LogThreshold t, struct Cfg *cfg, bool del);

void print_cfg_commands(enum LogThreshold t, struct Cfg *cfg);

void print_head(enum LogThreshold t, enum InfoEvent event, struct Head *head);

void print_heads(enum LogThreshold t, enum InfoEvent event, struct SList *heads);

void print_mode(enum LogThreshold t, struct Mode *mode);

void print_head_desired_mode_fallback(enum LogThreshold t, struct Head *head);

void print_user_mode(enum LogThreshold t, struct UserMode *user_mode, bool del);

void info_user_mode_string(struct UserMode *user_mode, char *buf, size_t nbuf);

void info_mode_string(struct Mode *mode, char *buf, size_t nbuf);

#endif // INFO_H


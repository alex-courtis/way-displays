#ifndef INFO_H
#define INFO_H

#include <stdbool.h>
#include <stddef.h>

#include "cfg.h"
#include "displ.h"
#include "head.h"
#include "slist.h"
#include "log.h"
#include "mode.h"

#define LEN_HUMAN 1024 * 64

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

// LEN_HUMAN, consumer frees
char *delta_human(const enum DisplState state, const struct SList * const heads);

// LEN_HUMAN, consumer frees
char *delta_human_mode(const enum DisplState state, const struct Head * const head);

// LEN_HUMAN, consumer frees
char *delta_human_adaptive_sync(const enum DisplState state, const struct Head * const head);

// log info and send on-change-command
void report_success(const char * const human);

// log error and send on-change-command
// void report_failure(const char * const human);

// log info and send on-change-command
void report_adaptive_sync_fail(struct Head *head);

#endif // INFO_H


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

#define CALLBACK_MSG_LEN 1024 * 64

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

// CALLBACK_MSG_LEN, consumer frees
char *delta_human(const enum DisplState state, const struct SList * const heads);

// CALLBACK_MSG_LEN, consumer frees
char *delta_human_mode(const enum DisplState state, const struct Head * const head);

// CALLBACK_MSG_LEN, consumer frees
char *delta_human_adaptive_sync(const enum DisplState state, const struct Head * const head);

// CALLBACK_MSG_LEN, consumer frees
char *delta_human_adaptive_sync_fail(const enum DisplState state, const struct Head * const head);

// maybe execute CALLBACK_CMD
// set CALLBACK_MSG to msg1..msg2
// set CALLBACK_STATUS to log name
void call_back(enum LogThreshold t, const char * const msg1, const char * const msg2);

// maybe execute CALLBACK_CMD with warning and custom human
void call_back_adaptive_sync_fail(enum LogThreshold t, const struct Head * const head);

#endif // INFO_H


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

void print_cfg(const enum LogThreshold t, const struct Cfg * const cfg, const bool del);

void print_cfg_commands(const enum LogThreshold t, const struct Cfg * const cfg);

void print_head(const enum LogThreshold t, const enum InfoEvent event, const struct Head * const head);

void print_heads(const enum LogThreshold t, const enum InfoEvent event, const struct SList * const heads);

void print_list(const enum LogThreshold t, const struct SList * const heads);

void print_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head);

void print_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct Mode * const mode);

void info_user_mode_string(const struct UserMode * const user_mode, char * const buf, const size_t nbuf);

void info_mode_string(const struct Mode * const mode, char * const buf, const size_t nbuf);

// consumer frees
char *delta_human(const enum DisplState state, const struct SList * const heads);

// consumer frees
char *delta_human_mode(const enum DisplState state, const struct Head * const head);

// consumer frees
char *delta_human_adaptive_sync(const enum DisplState state, const struct Head * const head);

// maybe execute CALLBACK_CMD
// set CALLBACK_MSG to msg1..msg2
// set CALLBACK_LEVEL to log name
void call_back(const enum LogThreshold t, const char * const msg1, const char * const msg2);

// maybe execute CALLBACK_CMD
void call_back_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head);

// maybe execute CALLBACK_CMD
void call_back_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct Mode * const mode);

#endif // INFO_H


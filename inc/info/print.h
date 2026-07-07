#ifndef PRINT_H
#define PRINT_H

#include <stdbool.h>

#include "cfg.h"
#include "displ.h"
#include "head.h"
#include "log.h"
#include "mode.h"
#include "slist.h"

enum InfoEvent {
	ARRIVED,
	DEPARTED,
	DELTA,
	NONE,
};

void print_cfg(const enum LogThreshold t, const struct Cfg * const cfg, const bool del);

void print_cfg_commands(const enum LogThreshold t, const struct Cfg * const cfg);

void print_head(const enum LogThreshold t, const enum InfoEvent event, const struct Head * const head);

void print_head_current(const enum LogThreshold t, const struct Head * const head);

void print_head_desired(const enum LogThreshold t, const struct Head * const head);

void print_heads(const enum LogThreshold t, const enum InfoEvent event, const struct SList * const heads);

void print_list(const enum LogThreshold t, const struct SList * const heads);

void print_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head);

void print_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct Mode * const mode);

void print_head_queue(const enum LogThreshold t, const char *msg, enum DisplState displ_state, struct SList * const heads);

#endif // PRINT_H


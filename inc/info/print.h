#ifndef PRINT_H
#define PRINT_H

#include <stdbool.h>

#include "cfg/cfg.h"
#include "enum.h"
#include "head.h"
#include "ppmap.h"
#include "pset.h"
#include "wlr-output-management-unstable-v1.h"

void print_cfg(const enum LogThreshold t, const struct Cfg * const cfg, const bool del);

void print_cfg_commands(const enum LogThreshold t, const struct Cfg * const cfg);

void print_head(const enum LogThreshold t, const enum InfoEvent event, const struct Head * const head);

void print_head_current(const enum LogThreshold t, const struct Head * const head);

void print_head_desired(const enum LogThreshold t, const struct Head * const head);

void print_head_map(const enum LogThreshold t, const enum InfoEvent event, const struct PPmap * const heads);

void print_head_set(const enum LogThreshold t, const enum InfoEvent event, const struct Pset * const heads);

void print_list(const enum LogThreshold t, const struct PPmap * const heads);

void print_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head);

void print_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct zwlr_output_mode_v1* const zmode);

void print_v1_deprecation(void);

#endif // PRINT_H


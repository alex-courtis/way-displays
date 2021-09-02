#ifndef TYPES_H
#define TYPES_H

// TODO remove
#include <stdio.h>

#include <stdbool.h>
#include <stdlib.h>
#include <wayland-util.h>

#include "list.h"

struct Mode {
	struct Head *head;

	struct zwlr_output_mode_v1 *zwlr_mode;

	int32_t width;
	int32_t height;
	int32_t refresh_mHz;
	bool preferred;
};

struct Head {
	struct OutputManager *output_manager;

	struct zwlr_output_head_v1 *zwlr_head;

	struct SList *modes;

	char *name;
	char *description;
	int32_t width_mm;
	int32_t height_mm;
	int enabled;
	struct Mode *current_mode;
	int32_t x;
	int32_t y;
	int32_t transform;
	wl_fixed_t scale;
	char *make;
	char *model;
	char *serial_number;

	struct {
		struct Mode *mode;
		wl_fixed_t scale;
		int enabled;
		int32_t x;
		int32_t y;
	} desired;
};

struct OutputManager {
	struct zwlr_output_manager_v1 *zwlr_output_manager;

	struct SList *heads;

	uint32_t serial;
	uint32_t serial_cfg_done;
	uint32_t name;
	char *interface;

	struct {
		struct SList *heads_ordered;
	} desired;
};

void free_mode(struct Mode *mode);
void free_head(struct Head *head);
void free_output_manager(struct OutputManager *output_manager);

void head_release_mode(struct Head *head, struct Mode *mode);
void output_manager_release_head(struct OutputManager *output_manager, struct Head *head);

#endif // TYPES_H


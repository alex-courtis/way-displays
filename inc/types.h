#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#include "list.h"

// TODO convenience functions to free / clear wl_lists of each of these

struct Mode {
	struct zwlr_output_mode_v1 *zwlr_mode;

	int32_t width;
	int32_t height;
	int32_t refresh_mHz;
	bool preferred;
};

struct Head {
	struct zwlr_output_head_v1 *zwlr_head;
	struct zwlr_output_mode_v1 *zwlr_current_mode;

	struct SList *modes;

	const char *name;
	const char *description;
	int32_t width_mm;
	int32_t height_mm;
	int enabled;
	int32_t x;
	int32_t y;
	int32_t transform;
	wl_fixed_t scale;
	const char *make;
	const char *model;
	const char *serial_number;

	struct {
		struct Mode *mode;
		wl_fixed_t scale;
		int enabled;
	} desired;
};

struct OutputManager {
	struct zwlr_output_manager_v1 *zwlr_output_manager;

	struct SList *heads;

	uint32_t serial;
	uint32_t name;
	const char *interface;

	struct {
		struct SList *heads_enabled;
		struct SList *heads_disabled;
		struct SList *order;
	} desired;
};

struct HeadOrder {

	const char *name_desc;
};

#endif // TYPES_H


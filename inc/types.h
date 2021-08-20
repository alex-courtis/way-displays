#ifndef TYPES_H
#define TYPES_H

struct Mode {
	struct wl_list link;

	struct zwlr_output_mode_v1 *zwlr_mode;

	int32_t width;
	int32_t height;
	int32_t refresh;
	int preferred;
};

struct Head {
	struct wl_list link;

	struct zwlr_output_head_v1 *zwlr_head;

	struct wl_list modes;

	const char *name;
	const char *description;
	int32_t width_mm;
	int32_t height_mm;
	// todo: try and make these into Modes
	struct zwlr_output_mode_v1 *mode;
	struct zwlr_output_mode_v1 *current_mode;
	int enabled;
	int32_t x;
	int32_t y;
	int32_t transform;
	wl_fixed_t scale;
	const char *make;
	const char *model;
	const char *serial_number;
};

struct OutputManager {
	struct zwlr_output_manager_v1 *zwlr_output_manager;

	struct wl_list heads;

	uint32_t serial;
	uint32_t name;
	const char *interface;
};

#endif // TYPES_H


#ifndef TYPES_H
#define TYPES_H

struct Mode {
	struct wl_list link;

	int32_t width;
	int32_t height;
	int32_t refresh;
	int preferred;
};

struct Head {
	struct wl_list link;

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
	struct wl_list modes;
};

#endif // TYPES_H


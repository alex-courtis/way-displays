#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdlib.h>
#include <wayland-client-core.h>

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

	struct zwlr_output_configuration_head_v1 *zwlr_config_head;

	struct SList *modes;

	bool dirty;

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
	bool lid_closed;
	bool size_specified;

	struct {
		struct Mode *mode;
		wl_fixed_t scale;
		int enabled;
		int32_t x;
		int32_t y;
	} desired;

	struct {
		bool mode;
		bool scale;
		bool enabled;
		bool position;
	} pending;
};

struct OutputManager {
	struct Displ *displ;

	struct zwlr_output_manager_v1 *zwlr_output_manager;

	struct SList *heads;

	bool dirty;

	uint32_t serial;
	char *interface;
	struct SList *heads_arrived;
	struct SList *heads_departed;

	struct {
		struct SList *heads;
	} desired;
};

struct Displ {
	struct wl_display *display;

	struct wl_registry *registry;

	struct OutputManager *output_manager;
	struct Cfg *cfg;
	struct Lid *lid;

	uint32_t name;
};

struct UserScale {
	char *name_desc;
	float scale;
};

struct Cfg {
	char *file_path;
	char *laptop_display_prefix;
	struct SList *order_name_desc;
	bool auto_scale;

	struct SList *user_scales;
};

struct Lid {
	bool closed;

	bool dirty;

	char *device_path;
	struct libinput *libinput_monitor;
	int libinput_fd;
};

void free_mode(struct Mode *mode);
void free_head(struct Head *head);
void free_output_manager(struct OutputManager *output_manager);
void free_displ(struct Displ *displ);
void free_user_scale(struct UserScale *user_scale);
void free_cfg(struct Cfg *cfg);
void free_lid(struct Lid *lid);

void head_free_mode(struct Head *head, struct Mode *mode);
void output_manager_free_head(struct OutputManager *output_manager, struct Head *head);
void output_manager_free_heads_departed(struct OutputManager *output_manager);

void output_manager_release_heads_arrived(struct OutputManager *output_manager);

bool is_dirty(struct Displ *displ);
void reset_dirty(struct Displ *displ);

bool is_pending_output_manager(struct OutputManager *output_manager);
bool is_pending_head(struct Head *head);

void reset_pending_desired(struct OutputManager *output_manager);

#endif // TYPES_H


#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

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
	struct Mode *preferred_mode;
	int32_t x;
	int32_t y;
	enum wl_output_transform transform;
	wl_fixed_t scale;
	char *make;
	char *model;
	char *serial_number;
	bool lid_closed;
	bool max_preferred_refresh;

	struct {
		struct Mode *mode;
		wl_fixed_t scale;
		int enabled;
		// layout coords
		int32_t x;
		int32_t y;
		int32_t width;
		int32_t height;
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

	int retries;

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

void free_mode(void *mode);
void free_head(void *head);
void free_output_manager(void *output_manager);
void free_displ(void *displ);

void head_free_mode(struct Head *head, struct Mode *mode);

bool is_dirty(struct Displ *displ);
void reset_dirty(struct Displ *displ);

bool is_pending_output_manager(struct OutputManager *output_manager);
bool is_pending_head(struct Head *head);

void reset_pending_desired(struct OutputManager *output_manager);

#endif // TYPES_H


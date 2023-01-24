#ifndef HEAD_H
#define HEAD_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "mode.h"
#include "wlr-output-management-unstable-v1.h"

extern struct SList *heads;
extern struct SList *heads_arrived;
extern struct SList *heads_departed;

struct HeadState {
	struct Mode *mode;
	wl_fixed_t scale;
	bool enabled;
	// layout coords
	int32_t x;
	int32_t y;
};

struct Head {

	struct zwlr_output_head_v1 *zwlr_head;

	struct zwlr_output_configuration_head_v1 *zwlr_config_head;

	struct SList *modes;

	char *name;
	char *description;
	int32_t width_mm;
	int32_t height_mm;
	struct Mode *preferred_mode;
	enum wl_output_transform transform;
	char *make;
	char *model;
	char *serial_number;
	enum zwlr_output_head_v1_adaptive_sync_state adaptive_sync;

	struct HeadState current;
	struct HeadState desired;

	struct SList *modes_failed;

	struct {
		int32_t width;
		int32_t height;
	} scaled;

	bool warned_no_preferred;
	bool warned_no_mode;
};

bool head_matches_name_desc(const void *name_desc, const void *head);

wl_fixed_t head_auto_scale(struct Head *head);

void head_scaled_dimensions(struct Head *head);

struct Mode *head_find_mode(struct Head *head);

bool head_current_not_desired(const void *head);

bool head_current_mode_not_desired(const void *head);

void head_release_mode(struct Head *head, struct Mode *mode);

void head_free(void *head);

void heads_release_head(struct Head *head);

void heads_destroy(void);

#endif // HEAD_H


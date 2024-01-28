#ifndef HEAD_H
#define HEAD_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "displ.h"
#include "mode.h"
#include "wlr-output-management-unstable-v1.h"

#define HEAD_DEFAULT_SCALING_BASE 256
// While the fractional-scale-v1 protocol deals with scales in multiples of 1/120,
// there are differences in behavior between compositors, see !138.
// We force scales to be multiples of 1/8 in this case, because gcd(256, 120) = 8.
#define HEAD_FRACTIONAL_SCALING_BASE 8

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
	enum wl_output_transform transform;
	enum zwlr_output_head_v1_adaptive_sync_state adaptive_sync;
};

struct Head {

	struct zwlr_output_head_v1 *zwlr_head;

	struct zwlr_output_configuration_head_v1 *zwlr_config_head;

	struct SList *modes;

	char *name;
	char *description;
	int32_t width_mm;
	int32_t height_mm;
	char *make;
	char *model;
	char *serial_number;

	struct HeadState current;
	struct HeadState desired;

	struct SList *modes_failed;
	bool adaptive_sync_failed;

	struct {
		int32_t width;
		int32_t height;
	} scaled;

	int32_t scaling_base;

	bool warned_no_preferred;
	bool warned_no_mode;
};

bool head_matches_name_desc_exact(const void *head, const void *name_desc);

bool head_matches_name_desc_regex(const void *head, const void *name_desc);

bool head_matches_name_desc_fuzzy(const void *h, const void *name_desc);

bool head_matches_name_desc_partial(const void *head, const void *name_desc);

bool head_matches_name_desc(const void *head, const void *name_desc);

bool head_name_desc_matches_head(const void *name_desc, const void *head);

wl_fixed_t head_get_fixed_scale(double scale, int32_t base);

int32_t head_get_scaled_length(int32_t length, wl_fixed_t fixed_scale, int32_t base);

wl_fixed_t head_auto_scale(struct Head *head, double min, double max);

void head_scaled_dimensions(struct Head *head);

struct Mode *head_find_mode(struct Head *head);

struct Mode *head_preferred_mode(struct Head *head);

bool head_current_not_desired(const void *head);

bool head_current_mode_not_desired(const void *head);

bool head_current_adaptive_sync_not_desired(const void *head);

void head_release_mode(struct Head *head, struct Mode *mode);

void head_free(void *head);

void heads_release_head(struct Head *head);

void heads_destroy(void);

#endif // HEAD_H


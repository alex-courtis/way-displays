#ifndef HEAD_H
#define HEAD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "mode.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"

// wl_fixed_t, used by the wlr-output-management protocol, uses scales in multiples of 1/256.
// Meanwhile, the fractional-scale-v1 protocol deals with scales in multiples of 1/120,
// and there are observed differences in behavior between compositors, see !138.
// We force scales to be multiples of 1/8, because gcd(256, 120) = 8.
#define HEAD_DEFAULT_SCALING_BASE 8
#define HEAD_WLFIXED_SCALING_BASE 256

extern struct SList *heads;
extern struct SList *heads_arrived;
extern struct SList *heads_departed;

enum ManualOverride {
	NoOverride = 0,
	OverrideTrue,
	OverrideFalse,
};

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

	enum ManualOverride overrided_enabled;

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

// description, name, "???"
const char *head_human(const struct Head * const head);

// collection equality functions
bool head_matches_name_desc_exact(const void * const head, const void * const name_desc);

bool head_matches_name_desc_regex(const void * const head, const void * const name_desc);

bool head_matches_name_desc_partial(const void * const h, const void * const name_desc);

bool head_matches_name_desc_fuzzy(const void * const head, const void * const name_desc);

bool head_name_desc_matches_head(const void * const name_desc, const void * const head);

bool head_disabled_matches_head(const void * const d, const void * const h);

// scale calculations
wl_fixed_t head_get_fixed_scale(const struct Head * const head, const double scale, const int32_t base);

wl_fixed_t head_auto_scale(const struct Head * const head, const double min, const double max);

// sets scaled.height/width
void head_set_scaled_dimensions(struct Head * const head);

// applies extra toggles that should change head state directly
void head_apply_toggles(struct Head * const head, struct Cfg *cfg);

// finds a mode and logs/calls back on
//  no mode:           error
//  invalid user mode: warning
//  no preferred:      info
// maybe sets warned_no_preferred
struct Mode *head_find_mode(struct Head * const head);

struct Mode *head_preferred_mode(const struct Head * const head);

bool head_current_not_desired(const void * const head);

size_t head_num_current_not_desired(struct SList * const heads);

bool head_current_mode_not_desired(const void * const head);

bool head_current_adaptive_sync_not_desired(const void * const head);

void head_release_mode(struct Head * const head, const struct Mode * const mode);

void head_free(const void * const head);

void heads_release_head(const struct Head * const head);

void heads_destroy(void);

#endif // HEAD_H


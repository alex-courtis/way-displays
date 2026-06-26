#ifndef HEAD_H
#define HEAD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "cfg/disabled.h"
#include "mode.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"

// global singletons
extern struct SList *g_heads;
extern struct SList *g_heads_arrived;
extern struct SList *g_heads_departed;

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
	bool reapply_required;

	struct SList *modes_failed;
	bool adaptive_sync_failed;

	struct {
		int32_t width;
		int32_t height;
	} scaled;

	bool warned_no_preferred;
	bool warned_no_mode;
};

// description, name, "???"
const char *head_human(const struct Head * const head);

bool head_matches_name_desc_exact(const struct Head * const head, const char * const name_desc);

bool head_matches_name_desc_regex(const struct Head * const head, const char * const name_desc);

bool head_matches_name_desc_fuzzy(const struct Head * const head, const char * const name_desc);

bool head_matches_name_desc(const struct Head * const head, const char * const name_desc);

bool head_name_desc_matches_head(const char * const name_desc, const struct Head * const head);

bool head_name_desc_i_matches_head(const char * const name_desc, const size_t i, const struct Head * const head);

bool head_disabled_matches_head(const struct Disabled * const disabled, const struct Head * const head);

// calculate fixed scale correctly quantized for fractional scaling, obeying scale_round_to and scale_round_strategy
wl_fixed_t head_get_fixed_scale(const double scale);

wl_fixed_t head_auto_scale(const struct Head * const head, const double min, const double max);

// sets scaled.height/width
void head_set_scaled_dimensions(struct Head * const head);

// applies extra toggles that should change head state directly
void head_apply_toggles(struct Head * const head, const struct Cfg *cfg);

// finds a mode and logs/calls back on
//  no mode:           error
//  invalid user mode: warning
//  no preferred:      info
// maybe sets warned_no_preferred
struct Mode *head_find_mode(struct Head * const head);

struct Mode *head_preferred_mode(const struct Head * const head);

bool head_current_not_desired(const struct Head * const head);

size_t head_num_current_not_desired(struct SList * const heads);

bool head_reapply_required(const struct Head * const head);

bool head_current_mode_not_desired(const struct Head * const head);

bool head_current_adaptive_sync_not_desired(const struct Head * const head);

// clear current and failed modes, flag for reapply
void heads_reapply(struct SList *heads);

// set description, stripping any leading "(null) "
void head_set_description(struct Head * const head, const char *description);

void head_release_mode(struct Head * const head, const struct Mode * const mode);

void head_free(struct Head *head);

void heads_release_head(const struct Head * const head);

void heads_destroy(void);

#endif // HEAD_H


#ifndef HEAD_H
#define HEAD_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
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
	const struct Mode *mode;
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

	const struct PSet* modes;          // pointers equal, not mode_equal
	const struct PSet *modes_failed;   // moved from modes

	const struct Mode *mode_preferred; // pointer into modes

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

	bool adaptive_sync_failed;

	struct {
		int32_t width;
		int32_t height;
	} scaled;

	bool warned_no_preferred;
	bool warned_no_mode;
};

/*
 * lifecycle
 */

struct Head *head_init(void);

// init a head, adding it to g_heads and g_heads_arrived
struct Head *head_introduce(struct zwlr_output_head_v1 *zwlr_head);

void head_free(struct Head *head);

// free a head, creating a dummy in g_heads_departed
void head_release(struct Head * const head);

// remove a mode from the head, including current/desired, freeing it
void head_release_mode(struct Mode *mode);

// free all g_heads
void heads_destroy(void);

/*
 * mutation
 */

// applies extra toggles that should change head state directly
void head_apply_toggles(struct Head * const head, const struct Cfg *cfg);

// set description, stripping any leading "(null) "
void head_set_description(struct Head * const head, const char *description);

// add a new entry to modes and return it, NULL on any NULL input
struct Mode *head_add_mode(struct Head * const head, struct zwlr_output_mode_v1 *zwlr_mode);

// set current.mode, does nothing on NULL inputs or zwlr_mode not present
void head_set_current_mode(struct Head * const head, const struct zwlr_output_mode_v1 *zwlr_mode);

// set preferred mode, NOP and warning if preferred mode already set
void head_set_mode_preferred(const struct Mode * const mode);

// clear current and failed modes, flag for reapply
void heads_reapply(struct SList *heads);

/*
 * string rendering
 */

// description, name, "???"
const char *head_human(const struct Head * const head);

/*
 * predicates
 */

// exact name or description
bool head_matches_name_desc_exact(const struct Head * const head, const char * const name_desc);

// regex match on name or description
bool head_matches_name_desc_regex(const struct Head * const head, const char * const name_desc);

// partial case insensitive name or description, regexes excluded
bool head_matches_name_desc_fuzzy(const struct Head * const head, const char * const name_desc);

// exact, regex or fuzzy
bool head_matches_name_desc(const struct Head * const head, const char * const name_desc);

// exact, regex or fuzzy
bool head_name_desc_matches_head(const char * const name_desc, const struct Head * const head);

/*
 * tests
 */

// current and desired differ in any way
bool head_current_not_desired(const struct Head * const head);

// current mode is not desired
bool head_current_mode_not_desired(const struct Head * const head);

// current adaptive sync is not desired
bool head_current_adaptive_sync_not_desired(const struct Head * const head);

// full reapply next layout
bool head_reapply_required(const struct Head * const head);

/*
 * utility
 */

// calculate fixed scale correctly quantized for fractional scaling, obeying scale_round_to and scale_round_strategy
wl_fixed_t head_get_fixed_scale(const double scale);

// auto scale at the desired mode, 1 when no desired or mode_dpi unavailable
wl_fixed_t head_auto_scale(const struct Head * const head, const double min, const double max);

/*
 * search
 */

// finds a mode and logs/calls back on
//  no mode:           error
//  invalid user mode: warning
//  no preferred:      info
// maybe sets warned_no_preferred
const struct Mode *head_find_mode(struct Head * const head);

// highest resolution at its highest refresh
const struct Mode *head_max_mode(const struct Head * const head);

#endif // HEAD_H


#ifndef HEAD_H
#define HEAD_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg/cfg.h"
#include "enum.h"
#include "ppmap.h"
#include "wlr-output-management-unstable-v1.h"

struct HeadState {
	const struct zwlr_output_mode_v1 *zmode;
	wl_fixed_t scale;
	bool enabled;
	// layout coords
	int32_t x;
	int32_t y;
	enum wl_output_transform transform;
	enum zwlr_output_head_v1_adaptive_sync_state adaptive_sync;
};

struct Head {
	struct zwlr_output_configuration_head_v1 *zconfig;

	const struct PPmap *modes;        // mode_ppmap_init - Modes by zwlr_output_mode_v1
	const struct PPmap *modes_failed; // mode_ppmap_init - moved out of modes

	const struct zwlr_output_mode_v1 *zmode_pref; // key to modes/modes_failed

	char *name;
	char *description;
	int32_t width_mm;
	int32_t height_mm;
	char *make;
	char *model;
	char *serial_number;

	struct {
		int32_t width;
		int32_t height;
	} scaled;

	struct HeadState cur;
	struct HeadState des;

	enum ManualOverride overrided_enabled;

	bool reapply_required;
	bool adaptive_sync_failed;
	bool warned_no_preferred;
	bool warned_no_mode;
};

/*
 * lifecycle
 */

struct Head *head_init(void);

// dummy head for departure printing
struct Head *head_dummy_init(const struct Head * const head);

const struct Pset *head_pset_init(void);

const struct PPmap *head_ppmap_init(void);

void head_free(struct Head *head);

// remove a mode from the head, including current/desired, freeing it
void head_release_mode(struct Head * const head, const struct zwlr_output_mode_v1 *zmode);

/*
 * mutation
 */

// applies extra toggles that should change head state directly
void head_apply_toggles(struct Head * const head, const struct Cfg *cfg);

// set description, stripping any leading "(null) "
void head_set_description(struct Head * const head, const char *description);

// set preferred mode, NOP and warning if preferred mode already set
void head_set_mode_pref(struct Head * const head, const struct zwlr_output_mode_v1* const zmode);

// clear current and failed modes, flag for reapply
void heads_reapply(const struct PPmap *heads);

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

// DPI / AUTO_SCALE_DPI, 1 when no DPI available
double head_scale(const struct Head * const head, const struct zwlr_output_mode_v1 * const zmode);

/*
 * search
 */

// finds a mode and logs/calls back on
//  no mode:           error
//  invalid user mode: warning
//  no preferred:      info
// maybe sets warned_no_preferred
const struct zwlr_output_mode_v1 *head_find_mode(struct Head * const head);

#endif // HEAD_H


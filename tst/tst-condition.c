#include "asserts.h"
#include "data.h"
#include "tst.h"
#include "util-col.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cfg/disabled.h"
#include "displ.h"
#include "enum.h"
#include "head.h"
#include "lid.h"
#include "pset.h"
#include "spmap.h"
#include "sset.h"

#include "cfg/condition.h"

struct State {
	struct CfgCondition *condition;
};

static int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));
	s->condition = cfg_condition_init();

	g_displ = displ_init();

	ppmap_put_many(g_displ->heads,
			H1, head_n("DP-1"),
			H2, head_n("DP-2"),
			H3, head_n("DP-3"),
			NULL);

	g_lid = NULL;

	*state = s;
	return 0;
}

static int after_each(void **state) {
	struct State *s = *state;

	cfg_condition_free(s->condition);

	displ_free(g_displ);

	free(g_lid);
	g_lid = NULL;

	free(s);

	return 0;
}

static void cfg_condition_failed__plugged(void **state) {
	const struct State *s = *state;

	sset_add(s->condition->plugged, "DP-1");
	assert_false(cfg_condition_failed(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-2");
	assert_false(cfg_condition_failed(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-3");
	assert_false(cfg_condition_failed(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-4");
	assert_true(cfg_condition_failed(s->condition, NULL));
}

static void cfg_condition_failed__unplugged(void **state) {
	const struct State *s = *state;

	sset_add(s->condition->unplugged, "DP-4");
	assert_false(cfg_condition_failed(s->condition, NULL));

	sset_add(s->condition->unplugged, "DP-1");
	assert_true(cfg_condition_failed(s->condition, NULL));
}

static void cfg_condition_failed__lid_closed(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_CLOSED;

	assert_true(cfg_condition_failed(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	g_lid->closed = true;

	assert_false(cfg_condition_failed(s->condition, NULL));

	g_lid->closed = false;

	assert_true(cfg_condition_failed(s->condition, NULL));
}

static void cfg_condition_failed__fail_lid_closed(void **state) {
	struct State *s = *state;

	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	s->condition->lid = LID_CLOSED;

	bool fail_lid_closed = false;

	assert_false(cfg_condition_failed(s->condition, &fail_lid_closed));

	fail_lid_closed = true;

	assert_true(cfg_condition_failed(s->condition, &fail_lid_closed));
}

static void cfg_condition_failed__lid_open(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_OPEN;

	assert_true(cfg_condition_failed(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	g_lid->closed = false;

	assert_false(cfg_condition_failed(s->condition, NULL));

	g_lid->closed = true;

	assert_true(cfg_condition_failed(s->condition, NULL));
}

static void cfg_condition_failed__lid_not_present(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_NOT_PRESENT;

	assert_false(cfg_condition_failed(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	assert_true(cfg_condition_failed(s->condition, NULL));
}

static void cfg_condition_failed__complex(void **state) {
	struct State *s = *state;

	sset_add(s->condition->plugged, "DP-1");
	assert_false(cfg_condition_failed(s->condition, NULL));

	sset_add(s->condition->unplugged, "DP-4");
	assert_false(cfg_condition_failed(s->condition, NULL));

	s->condition->lid = LID_CLOSED;
	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	assert_false(cfg_condition_failed(s->condition, NULL));
}

static void cfg_disabled_applies_to_head__name_desc_conditions(void **state) {
	const struct CfgCondition *condition_met = NULL;

	struct Head *head = head_n("DP-1");

	const struct CfgDisabled *disabled = cfg_disabled_init();

	const struct SPmap *disableds = cfg_disabled_spmap_init();
	spmap_put(disableds, "!DP-[1-5]", disabled);

	// unconditional met
	assert_true(cfg_disabled_applies_to_head(&condition_met, disableds, head, false));
	assert_nul(condition_met);

	struct CfgCondition *condition_unplugged = cfg_condition_init();
	sset_add(condition_unplugged->unplugged, "DP-99");
	pset_add(disabled->conditions, condition_unplugged);

	// unplugged met
	assert_true(cfg_disabled_applies_to_head(&condition_met, disableds, head, false));
	assert_ptr_equal(condition_met, condition_unplugged);

	struct CfgCondition *condition_lid = cfg_condition_init();
	condition_lid->lid = LID_NOT_PRESENT;
	pset_add(disabled->conditions, condition_lid);

	// unplugged still met
	assert_true(cfg_disabled_applies_to_head(&condition_met, disableds, head, false));
	assert_ptr_equal(condition_met, condition_unplugged);

	sset_add(condition_unplugged->unplugged, "DP-1");

	// lid now met
	assert_true(cfg_disabled_applies_to_head(&condition_met, disableds, head, false));
	assert_ptr_equal(condition_met, condition_lid);

	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	// none met
	assert_false(cfg_disabled_applies_to_head(&condition_met, disableds, head, false));
	assert_nul(condition_met);

	spmap_free_vals(disableds);

	head_free(head);
}

static void cfg_disabled_applies_to_head__name_desc_only(void **state) {
	const struct CfgDisabled *disabled = cfg_disabled_init();

	struct Head *head_disabled = head_n("DP-1");

	const struct SPmap *disableds = cfg_disabled_spmap_init();
	spmap_put(disableds, "DP-1", disabled);

	const struct CfgCondition *condition = NULL;

	assert_true(cfg_disabled_applies_to_head(&condition, disableds, head_disabled, false));

	assert_nul(condition);

	struct Head *head_enabled = head_n("DP-2");

	assert_false(cfg_disabled_applies_to_head(&condition, disableds, head_enabled, false));

	assert_nul(condition);

	spmap_free_vals(disableds);

	head_free(head_enabled);
	head_free(head_disabled);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(cfg_condition_failed__plugged),
		TEST_BA(cfg_condition_failed__unplugged),
		TEST_BA(cfg_condition_failed__lid_closed),
		TEST_BA(cfg_condition_failed__fail_lid_closed),
		TEST_BA(cfg_condition_failed__lid_open),
		TEST_BA(cfg_condition_failed__lid_not_present),
		TEST_BA(cfg_condition_failed__complex),

		TEST_BA(cfg_disabled_applies_to_head__name_desc_conditions),
		TEST_BA(cfg_disabled_applies_to_head__name_desc_only),
	};

	return RUN(tests);
}


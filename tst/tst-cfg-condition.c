#include "data.h"
#include "tst.h"
#include "util-col.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "displ.h"
#include "enum.h"
#include "lid.h"
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

static void cfg_condition_met__plugged(void **state) {
	const struct State *s = *state;

	sset_add(s->condition->plugged, "DP-1");
	assert_true(cfg_condition_met(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-2");
	assert_true(cfg_condition_met(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-3");
	assert_true(cfg_condition_met(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-4");
	assert_false(cfg_condition_met(s->condition, NULL));
}

static void cfg_condition_met__unplugged(void **state) {
	const struct State *s = *state;

	sset_add(s->condition->unplugged, "DP-4");
	assert_true(cfg_condition_met(s->condition, NULL));

	sset_add(s->condition->unplugged, "DP-1");
	assert_false(cfg_condition_met(s->condition, NULL));
}

static void cfg_condition_met__lid_closed(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_CLOSED;

	assert_false(cfg_condition_met(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	g_lid->closed = true;

	assert_true(cfg_condition_met(s->condition, NULL));

	g_lid->closed = false;

	assert_false(cfg_condition_met(s->condition, NULL));
}

static void cfg_condition_met__fail_lid_closed(void **state) {
	struct State *s = *state;

	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	s->condition->lid = LID_CLOSED;

	bool fail_lid_closed = false;

	assert_true(cfg_condition_met(s->condition, &fail_lid_closed));

	fail_lid_closed = true;

	assert_false(cfg_condition_met(s->condition, &fail_lid_closed));
}

static void cfg_condition_met__lid_open(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_OPEN;

	assert_false(cfg_condition_met(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	g_lid->closed = false;

	assert_true(cfg_condition_met(s->condition, NULL));

	g_lid->closed = true;

	assert_false(cfg_condition_met(s->condition, NULL));
}

static void cfg_condition_met__lid_not_present(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_NOT_PRESENT;

	assert_true(cfg_condition_met(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	assert_false(cfg_condition_met(s->condition, NULL));
}

static void cfg_condition_met__complex(void **state) {
	struct State *s = *state;

	sset_add(s->condition->plugged, "DP-1");
	assert_true(cfg_condition_met(s->condition, NULL));

	sset_add(s->condition->unplugged, "DP-4");
	assert_true(cfg_condition_met(s->condition, NULL));

	s->condition->lid = LID_CLOSED;
	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	assert_true(cfg_condition_met(s->condition, NULL));
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(cfg_condition_met__plugged),
		TEST_BA(cfg_condition_met__unplugged),
		TEST_BA(cfg_condition_met__lid_closed),
		TEST_BA(cfg_condition_met__fail_lid_closed),
		TEST_BA(cfg_condition_met__lid_open),
		TEST_BA(cfg_condition_met__lid_not_present),
		TEST_BA(cfg_condition_met__complex),
	};

	return RUN(tests);
}


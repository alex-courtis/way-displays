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

static void condition__plugged(void **state) {
	const struct State *s = *state;

	sset_add(s->condition->plugged, "DP-1");
	assert_false(cfg_condition_true(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-2");
	assert_false(cfg_condition_true(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-3");
	assert_false(cfg_condition_true(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-4");
	assert_true(cfg_condition_true(s->condition, NULL));
}

static void condition__unplugged(void **state) {
	const struct State *s = *state;

	sset_add(s->condition->unplugged, "DP-4");
	assert_false(cfg_condition_true(s->condition, NULL));

	sset_add(s->condition->unplugged, "DP-1");
	assert_true(cfg_condition_true(s->condition, NULL));
}

static void condition__lid_closed(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_CLOSED;

	assert_true(cfg_condition_true(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	g_lid->closed = true;

	assert_false(cfg_condition_true(s->condition, NULL));

	g_lid->closed = false;

	assert_true(cfg_condition_true(s->condition, NULL));
}

static void condition__lid_open(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_OPEN;

	assert_true(cfg_condition_true(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	g_lid->closed = false;

	assert_false(cfg_condition_true(s->condition, NULL));

	g_lid->closed = true;

	assert_true(cfg_condition_true(s->condition, NULL));
}

static void condition__lid_not_present(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_NOT_PRESENT;

	assert_false(cfg_condition_true(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	assert_true(cfg_condition_true(s->condition, NULL));
}

static void condition__complex(void **state) {
	struct State *s = *state;

	sset_add(s->condition->plugged, "DP-1");
	assert_false(cfg_condition_true(s->condition, NULL));

	sset_add(s->condition->unplugged, "DP-4");
	assert_false(cfg_condition_true(s->condition, NULL));

	s->condition->lid = LID_CLOSED;
	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	assert_false(cfg_condition_true(s->condition, NULL));
}

static void cfg_disabled_matches_head__name_desc_conditions(void **state) {
	const struct State *s = *state;

	struct Head *head = head_n("DP-1");

	struct CfgDisabled *disabled = disabled_nd("DP-1");

	const struct CfgCondition *condition_plugged = cfg_condition_clone(s->condition);
	sset_add(condition_plugged->plugged, "DP-1");
	pset_add(disabled->conditions, condition_plugged);

	struct CfgCondition *condition_lid = cfg_condition_clone(condition_plugged);
	condition_lid->lid = LID_NOT_PRESENT;
	pset_add(disabled->conditions, condition_lid);

	const struct CfgCondition *condition_unplugged = cfg_condition_clone(condition_lid);
	sset_add(condition_unplugged->unplugged, "DP-99");
	pset_add(disabled->conditions, condition_unplugged);

	assert_true(cfg_disabled_matches_head(disabled, head));

	condition_lid->lid = LID_OPEN;

	assert_false(cfg_disabled_matches_head(disabled, head));

	cfg_disabled_free(disabled);

	head_free(head);
}

static void cfg_disabled_matches_head__name_desc_only(void **state) {
	struct CfgDisabled *disabled = disabled_nd("DP-1");

	struct Head *head_disabled = head_n("DP-1");

	assert_true(cfg_disabled_matches_head(disabled, head_disabled));

	struct Head *head_enabled = head_n("DP-2");

	assert_false(cfg_disabled_matches_head(disabled, head_enabled));

	cfg_disabled_free(disabled);

	head_free(head_enabled);
	head_free(head_disabled);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(condition__plugged),
		TEST_BA(condition__unplugged),
		TEST_BA(condition__lid_closed),
		TEST_BA(condition__lid_open),
		TEST_BA(condition__lid_not_present),
		TEST_BA(condition__complex),

		TEST_BA(cfg_disabled_matches_head__name_desc_conditions),
		TEST_BA(cfg_disabled_matches_head__name_desc_only),
	};

	return RUN(tests);
}


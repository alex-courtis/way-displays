#include "assert-log.h"
#include "tst.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cfg/disabled.h"
#include "fn.h"
#include "head.h"
#include "lid.h"
#include "pset.h"
#include "slist.h"
#include "sset.h"

#include "cfg/condition.h"

struct State {
	struct Condition *condition;
};

static int before_each(void **state) {
	assert_logs_empty_before();

	struct State *s = calloc(1, sizeof(struct State));
	s->condition = condition_init();

	struct Head *h1 = head_init_name("DP-1");
	struct Head *h2 = head_init_name("DP-2");
	struct Head *h3 = head_init_name("DP-3");

	slist_append(&g_heads, h1);
	slist_append(&g_heads, h2);
	slist_append(&g_heads, h3);

	g_lid = NULL;

	*state = s;
	return 0;
}

static int after_each(void **state) {
	struct State *s = *state;

	condition_free(s->condition);
	slist_free_vals(&g_heads, (fn_free)head_free);

	free(g_lid);
	g_lid = NULL;

	free(s);

	return 0;
}

static void condition__plugged(void **state) {
	const struct State *s = *state;

	sset_add(s->condition->plugged, "DP-1");
	assert_false(condition_true(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-2");
	assert_false(condition_true(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-3");
	assert_false(condition_true(s->condition, NULL));

	sset_add(s->condition->plugged, "DP-4");
	assert_true(condition_true(s->condition, NULL));
}

static void condition__unplugged(void **state) {
	const struct State *s = *state;

	sset_add(s->condition->unplugged, "DP-4");
	assert_false(condition_true(s->condition, NULL));

	sset_add(s->condition->unplugged, "DP-1");
	assert_true(condition_true(s->condition, NULL));
}

static void condition__lid_closed(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_CLOSED;

	assert_true(condition_true(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	g_lid->closed = true;

	assert_false(condition_true(s->condition, NULL));

	g_lid->closed = false;

	assert_true(condition_true(s->condition, NULL));
}

static void condition__lid_open(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_OPEN;

	assert_true(condition_true(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	g_lid->closed = false;

	assert_false(condition_true(s->condition, NULL));

	g_lid->closed = true;

	assert_true(condition_true(s->condition, NULL));
}

static void condition__lid_not_present(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_NOT_PRESENT;

	assert_false(condition_true(s->condition, NULL));

	g_lid = calloc(1, sizeof(struct Lid));

	assert_true(condition_true(s->condition, NULL));
}

static void condition__complex(void **state) {
	struct State *s = *state;

	sset_add(s->condition->plugged, "DP-1");
	assert_false(condition_true(s->condition, NULL));

	sset_add(s->condition->unplugged, "DP-4");
	assert_false(condition_true(s->condition, NULL));

	s->condition->lid = LID_CLOSED;
	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	assert_false(condition_true(s->condition, NULL));
}

// TODO move to tst-disabled
static void disabled_matches_head__name_desc_conditions(void **state) {
	const struct State *s = *state;

	struct Head *head = head_init_name("DP-1");

	struct Disabled *disabled = disabled_init_name_desc("DP-1");

	const struct Condition *condition_plugged = condition_clone(s->condition);
	sset_add(condition_plugged->plugged, "DP-1");
	pset_add(disabled->conditions, condition_plugged);

	struct Condition *condition_lid = condition_clone(condition_plugged);
	condition_lid->lid = LID_NOT_PRESENT;
	pset_add(disabled->conditions, condition_lid);

	const struct Condition *condition_unplugged = condition_clone(condition_lid);
	sset_add(condition_unplugged->unplugged, "DP-99");
	pset_add(disabled->conditions, condition_unplugged);

	assert_true(disabled_matches_head(disabled, head));

	condition_lid->lid = LID_OPEN;

	assert_false(disabled_matches_head(disabled, head));

	disabled_free(disabled);

	head_free(head);
}

static void disabled_matches_head__name_desc_only(void **state) {
	struct Disabled *disabled = disabled_init_name_desc("DP-1");

	struct Head *head_disabled = head_init_name("DP-1");

	assert_true(disabled_matches_head(disabled, head_disabled));

	struct Head *head_enabled = head_init_name("DP-2");

	assert_false(disabled_matches_head(disabled, head_enabled));

	disabled_free(disabled);

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
		TEST_BA(disabled_matches_head__name_desc_conditions),
		TEST_BA(disabled_matches_head__name_desc_only),
	};

	return RUN(tests);
}


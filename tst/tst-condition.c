#include "tst.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "head.h"
#include "lid.h"
#include "slist.h"
#include "sset.h"

#include "cfg/condition.h"

struct State {
	struct Condition *condition;
};

static int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));
	s->condition = condition_init();

	struct Head *h1 = calloc(1, sizeof(struct Head));
	struct Head *h2 = calloc(1, sizeof(struct Head));
	struct Head *h3 = calloc(1, sizeof(struct Head));

	h1->name = strdup("DP-1");
	h2->name = strdup("DP-2");
	h3->name = strdup("DP-3");

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
	assert_true(condition_evaluate(s->condition));

	sset_add(s->condition->plugged, "DP-2");
	assert_true(condition_evaluate(s->condition));

	sset_add(s->condition->plugged, "DP-3");
	assert_true(condition_evaluate(s->condition));

	sset_add(s->condition->plugged, "DP-4");
	assert_false(condition_evaluate(s->condition));
}

static void condition__unplugged(void **state) {
	const struct State *s = *state;

	sset_add(s->condition->unplugged, "DP-4");
	assert_true(condition_evaluate(s->condition));

	sset_add(s->condition->unplugged, "DP-1");
	assert_false(condition_evaluate(s->condition));
}

static void condition__lid_closed(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_CLOSED;

	assert_false(condition_evaluate(s->condition));

	g_lid = calloc(1, sizeof(struct Lid));

	g_lid->closed = true;

	assert_true(condition_evaluate(s->condition));

	g_lid->closed = false;

	assert_false(condition_evaluate(s->condition));
}

static void condition__lid_open(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_OPEN;

	assert_false(condition_evaluate(s->condition));

	g_lid = calloc(1, sizeof(struct Lid));

	g_lid->closed = false;

	assert_true(condition_evaluate(s->condition));

	g_lid->closed = true;

	assert_false(condition_evaluate(s->condition));
}

static void condition__lid_not_present(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_NOT_PRESENT;

	assert_true(condition_evaluate(s->condition));

	g_lid = calloc(1, sizeof(struct Lid));

	assert_false(condition_evaluate(s->condition));
}

static void condition__complex(void **state) {
	struct State *s = *state;

	sset_add(s->condition->plugged, "DP-1");
	assert_true(condition_evaluate(s->condition));

	sset_add(s->condition->unplugged, "DP-4");
	assert_true(condition_evaluate(s->condition));

	s->condition->lid = LID_CLOSED;
	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	assert_true(condition_evaluate(s->condition));
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(condition__plugged),
		TEST_BA(condition__unplugged),
		TEST_BA(condition__lid_closed),
		TEST_BA(condition__lid_open),
		TEST_BA(condition__lid_not_present),
		TEST_BA(condition__complex),
	};

	return RUN(tests);
}


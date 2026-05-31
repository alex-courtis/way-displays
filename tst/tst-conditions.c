#include "tst.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "conditions.h"
#include "head.h"
#include "lid.h"
#include "slist.h"

struct State {
	struct Condition *condition;
};

static int before_all(void **state) {
	return 0;
}

static int after_all(void **state) {
	return 0;
}

static int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));
	s->condition = calloc(1, sizeof(struct Condition));

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
	slist_free_vals(&g_heads, head_free);

	free(g_lid);
	g_lid = NULL;

	free(s);

	return 0;
}


static void conditions__plugged(void **state) {
	struct State *s = *state;

	slist_append(&s->condition->plugged, strdup("DP-1"));
	assert_true(condition_evaluate(s->condition));

	slist_append(&s->condition->plugged, strdup("DP-2"));
	assert_true(condition_evaluate(s->condition));

	slist_append(&s->condition->plugged, strdup("DP-3"));
	assert_true(condition_evaluate(s->condition));

	slist_append(&s->condition->plugged, strdup("DP-4"));
	assert_false(condition_evaluate(s->condition));
}

static void conditions__unplugged(void **state) {
	struct State *s = *state;

	slist_append(&s->condition->unplugged, strdup("DP-4"));
	assert_true(condition_evaluate(s->condition));

	slist_append(&s->condition->unplugged, strdup("DP-1"));
	assert_false(condition_evaluate(s->condition));
}

static void conditions__lid_closed(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_CLOSED;

	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	assert_true(condition_evaluate(s->condition));
}

static void conditions__lid_open(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_OPEN;

	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = false;

	assert_true(condition_evaluate(s->condition));
}

static void conditions__lid_not_present(void **state) {
	struct State *s = *state;

	s->condition->lid = LID_NOT_PRESENT;

	assert_true(condition_evaluate(s->condition));
}

static void conditions__complex(void **state) {
	struct State *s = *state;

	slist_append(&s->condition->plugged, strdup("DP-1"));
	assert_true(condition_evaluate(s->condition));

	slist_append(&s->condition->unplugged, strdup("DP-4"));
	assert_true(condition_evaluate(s->condition));

	s->condition->lid = LID_CLOSED;
	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	assert_true(condition_evaluate(s->condition));
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(conditions__plugged),
		TEST(conditions__unplugged),
		TEST(conditions__lid_closed),
		TEST(conditions__lid_open),
		TEST(conditions__lid_not_present),
		TEST(conditions__complex),
	};

	return RUN(tests);
}


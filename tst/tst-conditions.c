#include "tst.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "conditions.h"
#include "head.h"
#include "slist.h"

struct State {
	struct Condition *condition;
	struct SList *conditions;
};

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));
	s->condition = calloc(1, sizeof(struct Condition));

	struct Head *h1 = calloc(1, sizeof(struct Head));
	struct Head *h2 = calloc(1, sizeof(struct Head));
	struct Head *h3 = calloc(1, sizeof(struct Head));

	h1->name = strdup("DP-1");
	h2->name = strdup("DP-2");
	h3->name = strdup("DP-3");

	slist_append(&heads, h1);
	slist_append(&heads, h2);
	slist_append(&heads, h3);

	*state = s;
	return 0;
}

int after_each(void **state) {
	struct State *s = *state;

	condition_free(s->condition);
	slist_free_vals(&s->conditions, condition_free);
	slist_free_vals(&heads, head_free);

	free(s);

	return 0;
}


void conditions__plugged(void **state) {
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

void conditions__unplugged(void **state) {
	struct State *s = *state;

	slist_append(&s->condition->unplugged, strdup("DP-4"));
	assert_true(condition_evaluate(s->condition));

	slist_append(&s->condition->unplugged, strdup("DP-1"));
	assert_false(condition_evaluate(s->condition));
}

void conditions__complex(void **state) {
	struct State *s = *state;

	slist_append(&s->condition->plugged, strdup("DP-1"));
	assert_true(condition_evaluate(s->condition));

	slist_append(&s->condition->unplugged, strdup("DP-4"));
	assert_true(condition_evaluate(s->condition));
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(conditions__plugged),
		TEST(conditions__unplugged),
		TEST(conditions__complex),
	};

	return RUN(tests);
}


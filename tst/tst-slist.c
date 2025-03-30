#include "tst.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"

struct State {
	struct SList *list1;
	struct SList *list2;
	struct SList *expected;
};

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	s->list1 = NULL;
	s->list2 = NULL;
	s->expected = NULL;

	*state = s;
	return 0;
}

int after_each(void **state) {
	struct State *s = *state;

	slist_free_vals(&s->list1, NULL);
	slist_free_vals(&s->list2, NULL);
	slist_free_vals(&s->expected, NULL);

	free(s);
	return 0;
}


static bool string_list_equal(struct SList *a, struct SList *b) {
	struct SList *i, *j;
	size_t a_count = 0, b_count = 0;

	for (i = a; i; i = i->nex) a_count++;
	for (j = b; j; j = j->nex) b_count++;

	if (a_count != b_count) return false;

	for (i = a; i; i = i->nex) {
		bool found = false;
		for (j = b; j; j = j->nex) {
			if (fn_comp_equals_strcmp(i->val, j->val)) {
				found = true;
				break;
			}
		}
		if (!found) return false;
	}

	return true;
}

void slist_xor_free__empty_lists(void **state) {
	struct State *s = *state;

	slist_xor_free(&s->list1, s->list2, fn_comp_equals_strcmp, NULL, fn_clone_strdup);

	assert_int_equal(slist_length(s->list1), 0);
}

void slist_xor_free__first_list_empty(void **state) {
	struct State *s = *state;

	slist_append(&s->list2, strdup("item1"));
	slist_append(&s->list2, strdup("item2"));

	slist_append(&s->expected, strdup("item1"));
	slist_append(&s->expected, strdup("item2"));

	slist_xor_free(&s->list1, s->list2, fn_comp_equals_strcmp, NULL, fn_clone_strdup);

	assert_true(string_list_equal(s->list1, s->expected));
}

void slist_xor_free__second_list_empty(void **state) {
	struct State *s = *state;

	slist_append(&s->list1, strdup("item1"));
	slist_append(&s->list1, strdup("item2"));

	slist_append(&s->expected, strdup("item1"));
	slist_append(&s->expected, strdup("item2"));

	slist_xor_free(&s->list1, s->list2, fn_comp_equals_strcmp, NULL, fn_clone_strdup);

	assert_true(string_list_equal(s->list1, s->expected));
}

void slist_xor_free__toggle_items(void **state) {
	struct State *s = *state;

	slist_append(&s->list1, strdup("item1"));
	slist_append(&s->list1, strdup("item2"));
	slist_append(&s->list1, strdup("item3"));

	slist_append(&s->list2, strdup("item2"));
	slist_append(&s->list2, strdup("item4"));

	slist_append(&s->expected, strdup("item1"));
	slist_append(&s->expected, strdup("item3"));
	slist_append(&s->expected, strdup("item4"));

	slist_xor_free(&s->list1, s->list2, fn_comp_equals_strcmp, NULL, fn_clone_strdup);

	assert_true(string_list_equal(s->list1, s->expected));
}

void slist_xor_free__duplicate_items(void **state) {
	struct State *s = *state;

	slist_append(&s->list1, strdup("item1"));
	slist_append(&s->list1, strdup("item1")); // duplicate
	slist_append(&s->list1, strdup("item2"));

	slist_append(&s->list2, strdup("item1"));

	slist_append(&s->expected, strdup("item2"));

	slist_xor_free(&s->list1, s->list2, fn_comp_equals_strcmp, NULL, fn_clone_strdup);

	assert_true(string_list_equal(s->list1, s->expected));
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(slist_xor_free__empty_lists),
		TEST(slist_xor_free__first_list_empty),
		TEST(slist_xor_free__second_list_empty),
		TEST(slist_xor_free__toggle_items),
		TEST(slist_xor_free__duplicate_items),
	};

	return RUN(tests);
}

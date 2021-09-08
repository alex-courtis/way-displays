#include <string.h>

#include "calc.h"

static int ltr_heads_setup(void **state) {
	struct SList *heads = NULL;
	struct Head *head = NULL;
	struct Mode *mode = NULL;

	mode = calloc(1, sizeof(struct Mode));
	mode->width = 10;
	head = calloc(1, sizeof(struct Head));
	head->desired.mode = mode;
	head->name = strdup("1");
	head->desired.enabled = true;
	head->desired.scale = wl_fixed_from_int(2);
	slist_append(&heads, head);

	mode = calloc(1, sizeof(struct Mode));
	mode->width = 10;
	head = calloc(1, sizeof(struct Head));
	head->desired.mode = mode;
	head->name = strdup("2");
	head->desired.enabled = false;
	head->desired.scale = wl_fixed_from_int(1);
	slist_append(&heads, head);

	head = calloc(1, sizeof(struct Head));
	head->name = strdup("3");
	head->desired.enabled = false;
	slist_append(&heads, head);

	mode = calloc(1, sizeof(struct Mode));
	mode->width = 11;
	head = calloc(1, sizeof(struct Head));
	head->name = strdup("4");
	head->desired.mode = mode;
	head->desired.enabled = true;
	head->desired.scale = wl_fixed_from_int(4);
	slist_append(&heads, head);

	mode = calloc(1, sizeof(struct Mode));
	mode->width = 1;
	head = calloc(1, sizeof(struct Head));
	head->name = strdup("5");
	head->desired.mode = mode;
	head->desired.enabled = true;
	head->desired.scale = wl_fixed_from_int(1);
	slist_append(&heads, head);

	*state = heads;

	return 0;
}

static int ltr_heads_teardown(void **state) {
	struct SList *heads = *state;
	struct Head *head = NULL;

	for (struct SList *i = heads; i; i = i->nex) {
		head = i->val;
		free(head->name);
		free(head->desired.mode);
		free(head);
	}
	slist_free(&heads);

	return 0;
}

static void ltr_heads_valid(void **state) {
	struct SList *heads = slist_shallow_clone(*state);
	struct Head *head = NULL;

	ltr_heads(heads);

	head = heads->val;
	assert_string_equal(head->name, "1");
	assert_int_equal(head->desired.x, 0);
	assert_int_equal(head->desired.y, 0);
	slist_remove(&heads, head);

	head = heads->val;
	assert_string_equal(head->name, "2");
	assert_int_equal(head->desired.x, 0);
	assert_int_equal(head->desired.y, 0);
	slist_remove(&heads, head);

	head = heads->val;
	assert_string_equal(head->name, "3");
	assert_int_equal(head->desired.x, 0);
	assert_int_equal(head->desired.y, 0);
	slist_remove(&heads, head);

	head = heads->val;
	assert_string_equal(head->name, "4");
	assert_int_equal(head->desired.x, 5);
	assert_int_equal(head->desired.y, 0);
	slist_remove(&heads, head);

	head = heads->val;
	assert_string_equal(head->name, "5");
	// head 4 rounds it up to 3
	assert_int_equal(head->desired.x, 5 + 3);
	assert_int_equal(head->desired.y, 0);
	slist_remove(&heads, head);

	assert_null(heads);
}

#define calc_ltr_heads_tests \
	cmocka_unit_test_setup_teardown(ltr_heads_valid, ltr_heads_setup, ltr_heads_teardown)


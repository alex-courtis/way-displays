#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "head.h"
#include "list.h"
#include "mode.h"
#include "server.h"

// forward declarations
struct SList *order_heads(struct SList *order_name_desc, struct SList *heads);
void position_heads(struct SList *heads);


struct State {
	struct Mode *mode;
	struct SList *heads;
};


int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	cfg = cfg_default();

	struct State *s = calloc(1, sizeof(struct State));

	s->mode = calloc(1, sizeof(struct Mode));
	for (int i = 0; i < 10; i++) {
		struct Head *head = calloc(1, sizeof(struct Head));
		head->desired.enabled = true;
		head->desired.mode = s->mode;
		slist_append(&s->heads, head);
	}

	*state = s;
	return 0;
}

int after_each(void **state) {
	cfg_destroy();

	struct State *s = *state;

	slist_free_vals(&s->heads, NULL);
	free(s->mode);

	free(s);
	return 0;
}

void order_heads__exact_partial_regex(void **state) {
	struct SList *order_name_desc = NULL;
	struct SList *heads = NULL;
	struct SList *expected = NULL;

	// ORDER
	slist_append(&order_name_desc, strdup("exact0"));
	slist_append(&order_name_desc, strdup("exact1"));
	slist_append(&order_name_desc, strdup("!.*regex.*"));
	slist_append(&order_name_desc, strdup("exact1")); // should not repeat
	slist_append(&order_name_desc, strdup("partial"));

	// heads
	struct Head not_specified_1 = { .description = "not specified 1", };
	struct Head exact0_partial =  { .description = "not an exact0 exact match" };
	struct Head partial =         { .description = "a partial match" };
	struct Head regex_match_1 =   { .description = "a regex match" };
	struct Head exact1 =          { .description = "exact1" };
	struct Head exact0 =          { .description = "exact0" };
	struct Head regex_match_2 =   { .description = "another regex match" };
	struct Head not_specified_2 = { .description = "not specified 2" };
	slist_append(&heads, &not_specified_1);
	slist_append(&heads, &exact0_partial);
	slist_append(&heads, &partial);
	slist_append(&heads, &regex_match_1);
	slist_append(&heads, &exact1);
	slist_append(&heads, &exact0);
	slist_append(&heads, &regex_match_2);
	slist_append(&heads, &not_specified_2);

	// expected
	slist_append(&expected, &exact0);
	slist_append(&expected, &exact0_partial);
	slist_append(&expected, &exact1);
	slist_append(&expected, &regex_match_1);
	slist_append(&expected, &regex_match_2);
	slist_append(&expected, &partial);
	slist_append(&expected, &not_specified_1);
	slist_append(&expected, &not_specified_2);

	struct SList *heads_ordered = order_heads(order_name_desc, heads);

	assert_heads_equal(heads_ordered, expected);

	slist_free_vals(&order_name_desc, NULL);
	slist_free(&heads);
	slist_free(&heads_ordered);
}

void order_heads__exact_regex_catchall(void **state) {
	struct SList *order_name_desc = NULL;
	struct SList *heads = NULL;
	struct SList *expected = NULL;

	// ORDER
	slist_append(&order_name_desc, strdup("exact0"));
	slist_append(&order_name_desc, strdup("!.*regex.*"));
	slist_append(&order_name_desc, strdup("!.*$"));
	slist_append(&order_name_desc, strdup("exact9"));

	// heads
	struct Head exact9 =          { .description = "exact9" };
	struct Head not_specified_1 = { .description = "not specified 1", };
	struct Head regex_match_1 =   { .description = "a regex match" };
	struct Head exact0 =          { .description = "exact0" };
	struct Head regex_match_2 =   { .description = "another regex match" };
	struct Head not_specified_2 = { .description = "not specified 2" };
	slist_append(&heads, &not_specified_1);
	slist_append(&heads, &regex_match_1);
	slist_append(&heads, &exact0);
	slist_append(&heads, &regex_match_2);
	slist_append(&heads, &not_specified_2);
	slist_append(&heads, &exact9);

	// expected
	slist_append(&expected, &exact0);
	slist_append(&expected, &regex_match_1);
	slist_append(&expected, &regex_match_2);
	slist_append(&expected, &not_specified_1);
	slist_append(&expected, &not_specified_2);
	slist_append(&expected, &exact9);

	struct SList *heads_ordered = order_heads(order_name_desc, heads);

	assert_heads_equal(heads_ordered, expected);

	slist_free_vals(&order_name_desc, NULL);
	slist_free(&heads);
	slist_free(&heads_ordered);
}

void order_heads__no_order(void **state) {
	struct SList *heads = NULL;
	struct Head head = { .name = "head", };

	slist_append(&heads, &head);

	// null/empty order
	struct SList *heads_ordered = order_heads(NULL, heads);
	assert_heads_equal(heads_ordered, heads);

	slist_free(&heads);
}

void position_heads__col_left(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = COL;
	cfg->align = LEFT;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 0, 5);
}

void position_heads__col_mid(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = COL;
	cfg->align = MIDDLE;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 2, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 3, 5);
}

void position_heads__col_right(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = COL;
	cfg->align = RIGHT;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 3;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 3, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 2); assert_head_position(head, 5, 5);
}

void position_heads__row_top(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = ROW;
	cfg->align = TOP;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 0);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 0);
}

void position_heads__row_mid(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = ROW;
	cfg->align = MIDDLE;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 2);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 2);
}

void position_heads__row_bottom(void **state) {
	struct State *s = *state;
	struct Head *head;

	cfg->arrange = ROW;
	cfg->align = BOTTOM;

	head = slist_at(s->heads, 0); head->scaled.width = 4; head->scaled.height = 2;
	head = slist_at(s->heads, 1); head->scaled.width = 7; head->scaled.height = 5;
	head = slist_at(s->heads, 2); head->scaled.width = 2; head->scaled.height = 1;

	position_heads(s->heads);

	head = slist_at(s->heads, 0); assert_head_position(head, 0, 3);
	head = slist_at(s->heads, 1); assert_head_position(head, 4, 0);
	head = slist_at(s->heads, 2); assert_head_position(head, 11, 4);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(order_heads__exact_partial_regex),
		TEST(order_heads__exact_regex_catchall),
		TEST(order_heads__no_order),

		TEST(position_heads__col_left),
		TEST(position_heads__col_mid),
		TEST(position_heads__col_right),
		TEST(position_heads__row_top),
		TEST(position_heads__row_mid),
		TEST(position_heads__row_bottom),
	};

	return RUN(tests);
}


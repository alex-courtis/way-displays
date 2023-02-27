#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <string.h>

#include "head.h"
#include "list.h"

// forward declarations
struct SList *order_heads(struct SList *order_name_desc, struct SList *heads);


int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	return 0;
}

int after_each(void **state) {
	return 0;
}

void order_heads__order(void **state) {
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

void order_heads__no_order(void **state) {

	struct SList *heads = NULL;
	struct Head head = { .name = "head", };

	slist_append(&heads, &head);

	// null/empty order
	struct SList *heads_ordered = order_heads(NULL, heads);
	assert_heads_equal(heads_ordered, heads);

	slist_free(&heads);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(order_heads__order),
		TEST(order_heads__no_order),
	};

	return RUN(tests);
}


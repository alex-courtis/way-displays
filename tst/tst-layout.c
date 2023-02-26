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
	slist_append(&order_name_desc, strdup("partial"));

	// heads
	struct Head head0 = { .name = "head0", .description = strdup("not specified") };
	struct Head head1 = { .name = "head1", .description = strdup("not an exact0 exact match") };
	struct Head head2 = { .name = "head2", .description = strdup("a partial match") };
	struct Head head3 = { .name = "head3", .description = strdup("exact1") };
	struct Head head4 = { .name = "head4", .description = strdup("exact0") };
	struct Head head5 = { .name = "head5", .description = strdup("a regex match") };
	struct Head head6 = { .name = "head6", .description = strdup("not specified") };
	slist_append(&heads, &head0);
	slist_append(&heads, &head1);
	slist_append(&heads, &head2);
	slist_append(&heads, &head3);
	slist_append(&heads, &head4);
	slist_append(&heads, &head5);
	slist_append(&heads, &head6);

	// expected
	slist_append(&expected, &head4); // exact0
	slist_append(&expected, &head1); // exact0 (partial)
	slist_append(&expected, &head3); // exact1
	slist_append(&expected, &head5); // !.*regex.*
	slist_append(&expected, &head2); // partial
	slist_append(&expected, &head0); //
	slist_append(&expected, &head6); //

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


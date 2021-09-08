#include <string.h>

#include "calc.h"

static void order_heads_valid(void **state) {
	struct Head *head;

	struct SList *order_name_desc = NULL;
	slist_append(&order_name_desc, strdup("e"));
	slist_append(&order_name_desc, strdup("d"));
	slist_append(&order_name_desc, NULL);
	slist_append(&order_name_desc, strdup("cdesc"));

	struct SList *heads = NULL;
	head = calloc(1, sizeof(struct Head));
	head->name = strdup("a");
	head->desired.enabled = false;
	slist_append(&heads, head);

	slist_append(&heads, NULL);

	head = calloc(1, sizeof(struct Head));
	head->name = strdup("b");
	head->desired.enabled = true;
	slist_append(&heads, head);

	head = calloc(1, sizeof(struct Head));
	head->name = strdup("c");
	head->description = strdup("cdesc");
	head->desired.enabled = true;
	slist_append(&heads, head);

	head = calloc(1, sizeof(struct Head));
	head->name = strdup("d");
	head->desired.enabled = false;
	slist_append(&heads, head);

	head = calloc(1, sizeof(struct Head));
	head->name = strdup("e");
	head->desired.enabled = true;
	slist_append(&heads, head);


	struct SList *heads_ordered = NULL;
	heads_ordered = order_heads(order_name_desc, heads);

	head = heads_ordered->val;
	assert_string_equal(head->name, "e");
	slist_remove(&heads_ordered, head);

	head = heads_ordered->val;
	assert_string_equal(head->name, "d");
	slist_remove(&heads_ordered, head);

	head = heads_ordered->val;
	assert_string_equal(head->name, "c");
	assert_string_equal(head->description, "cdesc");
	slist_remove(&heads_ordered, head);

	head = heads_ordered->val;
	assert_string_equal(head->name, "a");
	slist_remove(&heads_ordered, head);

	head = heads_ordered->val;
	assert_string_equal(head->name, "b");
	slist_remove(&heads_ordered, head);

	assert_null(heads_ordered);


	struct SList *i = order_name_desc;
	char *name_desc;
	while (i) {
		name_desc = i->val;
		i = i->nex;
		slist_remove(&order_name_desc, name_desc);
		free(name_desc);
	}
	slist_free(&order_name_desc);

	i = heads;
	while (i) {
		head = i->val;
		i = i->nex;
		slist_remove(&heads, head);
		if (head) {
			free(head->name);
			free(head);
		}
	}
	slist_free(&heads);
}

#define calc_order_heads_tests \
	cmocka_unit_test(order_heads_valid)


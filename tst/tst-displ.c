#include "tst.h"

#include "asserts.h"
#include "data.h"
#include "util-init.h"

#include <cmocka.h>

#include "head.h"
#include "ppmap.h"
#include "pset.h"

#include "displ.h"

static int before_each(void **state) {
	return 0;
}

static int after_each(void **state) {
	return 0;
}

static void displ_finished_head__not_present(void **state) {
	struct Displ *displ = displ_init();

	displ_finished_head(displ, H0);

	assert_int_equal(ppmap_size(displ->heads), 0);
	assert_int_equal(pset_size(displ->heads_arrived), 0);
	assert_int_equal(pset_size(displ->heads_departed), 0);

	displ_free(displ);
}

static void displ_finished_head__present(void **state) {

	struct Displ *displ = displ_init();

	const struct Head *head = head_n("head_finished");
	ppmap_put(displ->heads, H0, head);

	displ_finished_head(displ, H0);

	assert_int_equal(ppmap_size(displ->heads), 0);
	assert_int_equal(pset_size(displ->heads_arrived), 0);
	assert_int_equal(pset_size(displ->heads_departed), 1);

	const struct PsetIt *it = pset_it(displ->heads_departed);
	const struct Head *head_departed = it->val;
	pset_it_free(it);

	assert_non_nul(head_departed);

	assert_str_equal(head_departed->name, "head_finished");
	assert_str_equal(head_departed->description, "???");

	displ_free(displ);
}


int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(displ_finished_head__not_present),
		TEST_BA(displ_finished_head__present),
	};

	return RUN(tests);
}


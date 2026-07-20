#include "tst.h"

#include "asserts.h"
#include "data.h"
#include "util-init.h"

#include <cmocka.h>

#include "head.h"
#include "ppmap.h"
#include "pset.h"

#include "displ.h"

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

	ppmap_put(displ->heads, H0, head_n("head_finished"));

	displ_finished_head(displ, H0);

	assert_int_equal(ppmap_size(displ->heads), 0);
	assert_int_equal(pset_size(displ->heads_arrived), 0);
	assert_int_equal(pset_size(displ->heads_departed), 1);

	const struct Head *head_departed = pset_at(displ->heads_departed, 0);

	assert_non_nul(head_departed);

	assert_str_equal(head_departed->name, "head_finished");
	assert_str_equal(head_departed->description, "???");

	displ_free(displ);
}


int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(displ_finished_head__not_present),
		TEST(displ_finished_head__present),
	};

	return RUN(tests);
}


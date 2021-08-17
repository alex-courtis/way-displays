#include <stdio.h>
#include <setjmp.h>
#include <cmocka.h>

#include "add.h"

int __real_second(int second);

int
__wrap_first(int first) {
	return 1;
}

int
__wrap_second(int second) {
	return __real_second(second);
}

static void
test_add(void **state) {
	assert_int_equal(add(3, 5), 6);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_add),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}


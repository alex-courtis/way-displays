#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "test-add.c"
#include "test-laptop.c"
#include "test-util.c"

static int setup(void **state) {
	return 0;
}

static int teardown(void **state) {
	return 0;
}

int main(void) {
	const struct CMUnitTest tests[] = {
		add_tests,
		laptop_tests,
		util_tests,
	};
	return cmocka_run_group_tests(tests, setup, teardown);
}


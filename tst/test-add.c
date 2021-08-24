#include "add.h"

int __real_second(int second);

int __wrap_first(int first) {
	return 1;
}

int __wrap_second(int second) {
	return __real_second(second);
}

static void add_success(void **state) {
	assert_int_equal(add(3, 5), 6);
}

#define add_tests \
	cmocka_unit_test(add_success)


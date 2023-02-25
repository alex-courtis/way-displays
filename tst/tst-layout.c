#include "tst.h"

#include <cmocka.h>

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

void blargh(void **state) {
	assert_int_equal(1, 1);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(blargh),
	};

	return RUN(tests);
}


#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "mode.h"

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
	assert_logs_empty();
	return 0;
}


void mhz_to_hz_str__(void **state) {
	assert_string_equal_nn(mhz_to_hz_str(0), "0");
	assert_string_equal_nn(mhz_to_hz_str(99000), "99");
	assert_string_equal_nn(mhz_to_hz_str(12300), "12.3");
	assert_string_equal_nn(mhz_to_hz_str(12345), "12.345");
}

void hz_str_to_mhz__(void **state) {
	assert_int_equal(hz_str_to_mhz(NULL), 0);
	assert_int_equal(hz_str_to_mhz(""), 0);
	assert_int_equal(hz_str_to_mhz("abcde"), 0);
	assert_int_equal(hz_str_to_mhz("12"), 12000);
	assert_int_equal(hz_str_to_mhz("12.34"), 12340);
	assert_int_equal(hz_str_to_mhz("12.3456"), 12346);
}

void mhz_to_hz_rounded__(void **state) {
	assert_int_equal(mhz_to_hz_rounded(0), 0);
	assert_int_equal(mhz_to_hz_rounded(123567), 124);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(mhz_to_hz_str__),
		TEST(hz_str_to_mhz__),
		TEST(mhz_to_hz_rounded__),
	};

	return RUN(tests);
}


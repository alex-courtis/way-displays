#include <cmocka.h>

#include "mock-fn.h"

bool mock_equal(const void* const a, const void* const b) {
	check_expected_ptr(a);
	check_expected_ptr(b);

	return mock_type(bool);
}

bool mock_equal_size_t(const size_t a, const void* const b) {
	check_expected_int(a);
	check_expected_ptr(b);

	return mock_type(bool);
}

bool mock_less_than(const void* const a, const void* const b) {
	check_expected_ptr(a);
	check_expected_ptr(b);

	return mock_type(bool);
}

bool mock_test(const void* const val) {
	check_expected_ptr(val);

	return mock_type(bool);
}

void *mock_clone(const void* const val) {
	check_expected_ptr(val);

	return mock_ptr_type_checked(void*);
}

void *mock_alloc(const void* const val) {
	check_expected_ptr(val);

	return mock_ptr_type_checked(void*);
}

void mock_free(const void* const val) {
	check_expected_ptr(val);
}

char* mock_str(const void* const val) {
	check_expected_ptr(val);

	return mock_ptr_type_checked(char*);
}


#include <cmocka.h>

#include "mock-fn.h"

bool mock_equal(const void* const a, const void* const b) {
	check_expected_ptr(a);
	check_expected_ptr(b);

	return mock_type(bool);
}

bool mock_less_than(const void* const a, const void* const b) {
	check_expected_ptr(a);
	check_expected_ptr(b);

	return mock_type(bool);
}

bool mock_match_ptr(const void* const val, const void* const data) {
	check_expected_ptr(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_str(const char* const val, const void* const data) {
	check_expected_ptr(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_size_t(const size_t val, const void* const data) {
	check_expected_int(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_ptr_ptr(const void* const key, const void* const val, const void* const data) {
	check_expected_ptr(key);
	check_expected_ptr(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_str_ptr(const char* const key, const void* const val, const void* const data) {
	check_expected_ptr(key);
	check_expected_ptr(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_str_str(const char* const key, const char* const val, const void* const data) {
	check_expected_ptr(key);
	check_expected_ptr(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_str_size_t(const char *key, const size_t val, const void* const data) {
	check_expected_ptr(key);
	check_expected_int(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_size_t_ptr(const size_t key, const void* const val, const void* const data) {
	check_expected_int(key);
	check_expected_ptr(val);
	check_expected_ptr(data);

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


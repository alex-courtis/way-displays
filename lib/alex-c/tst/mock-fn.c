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

bool mock_match_val(const void* const val, const void* const data) {
	check_expected_ptr(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_key_val(const void* const key, const void* const val, const void* const data) {
	check_expected_ptr(key);
	check_expected_ptr(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_smap(const char *key, const void* const val, const void* const data) {
	check_expected_ptr(key);
	check_expected_ptr(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_smaps(const char *key, const char* const val, const void* const data) {
	check_expected_ptr(key);
	check_expected_ptr(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_sset(const char* const val, const void* const data) {
	check_expected_ptr(val);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_match_imap(const size_t key, const void* const val, const void* const data) {
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


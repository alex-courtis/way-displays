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

bool mock_2pred(const void* const ptr, const void* const data) {
	check_expected_ptr(ptr);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_2pred_str(const char* const str, const void* const data) {
	check_expected_ptr(str);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_2pred_szt(const size_t i, const void* const data) {
	check_expected_int(i);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_3pred(const void* const ptr1, const void* const ptr2, const void* const data) {
	check_expected_ptr(ptr1);
	check_expected_ptr(ptr2);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_3pred_str_ptr(const char* const str, const void* const ptr, const void* const data) {
	check_expected_ptr(str);
	check_expected_ptr(ptr);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_3pred_str_str(const char* const str1, const char* const str2, const void* const data) {
	check_expected_ptr(str1);
	check_expected_ptr(str2);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_3pred_str_szt(const char *str, const size_t i, const void* const data) {
	check_expected_ptr(str);
	check_expected_int(i);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_3pred_szt_ptr(const size_t i, const void* const ptr, const void* const data) {
	check_expected_int(i);
	check_expected_ptr(ptr);
	check_expected_ptr(data);

	return mock_type(bool);
}

bool mock_pred(const void* const ptr) {
	check_expected_ptr(ptr);

	return mock_type(bool);
}

void *mock_clone(const void* const ptr) {
	check_expected_ptr(ptr);

	return mock_ptr_type_checked(void*);
}

void *mock_alloc(const void* const ptr) {
	check_expected_ptr(ptr);

	return mock_ptr_type_checked(void*);
}

void mock_free(const void* const ptr) {
	check_expected_ptr(ptr);
}

char* mock_str(const void* const ptr) {
	check_expected_ptr(ptr);

	return mock_ptr_type_checked(char*);
}


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

bool mock_pred_p(const void* const p) {
	check_expected_ptr(p);

	return mock_type(bool);
}

bool mock_pred_s(const char* const s) {
	check_expected_ptr(s);

	return mock_type(bool);
}

bool mock_pred_i(const size_t i) {
	check_expected_int(i);

	return mock_type(bool);
}

bool mock_pred_p_p(const void* const p1, const void* const p2) {
	check_expected_ptr(p1);
	check_expected_ptr(p2);

	return mock_type(bool);
}

bool mock_pred_s_p(const char* const s, const void* const p) {
	check_expected_ptr(s);
	check_expected_ptr(p);

	return mock_type(bool);
}

bool mock_pred_s_s(const char* const s1, const void* const s2) {
	check_expected_ptr(s1);
	check_expected_ptr(s2);

	return mock_type(bool);
}

bool mock_pred_s_i(const char* const s,  const size_t i) {
	check_expected_ptr(s);
	check_expected_int(i);

	return mock_type(bool);
}

bool mock_pred_i_p(const size_t i, const void* const p) {
	check_expected_int(i);
	check_expected_ptr(p);

	return mock_type(bool);
}

bool mock_pred_p_p_p(const void* const p1, const void* const p2, const void* const p3) {
	check_expected_ptr(p1);
	check_expected_ptr(p2);
	check_expected_ptr(p3);

	return mock_type(bool);
}

bool mock_pred_s_p_p(const char* const s, const void* const p1, const void* const p2) {
	check_expected_ptr(s);
	check_expected_ptr(p1);
	check_expected_ptr(p2);

	return mock_type(bool);
}

bool mock_pred_s_s_p(const char* const s1, const char* const s2, const void* const p) {
	check_expected_ptr(s1);
	check_expected_ptr(s2);
	check_expected_ptr(p);

	return mock_type(bool);
}

bool mock_pred_s_i_p(const char *s, const size_t i, const void* const p) {
	check_expected_ptr(s);
	check_expected_int(i);
	check_expected_ptr(p);

	return mock_type(bool);
}

bool mock_pred_i_p_p(const size_t i, const void* const p1, const void* const p2) {
	check_expected_int(i);
	check_expected_ptr(p1);
	check_expected_ptr(p2);

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

void mock_free(void *ptr) {
	check_expected_ptr(ptr);
}

char* mock_str(const void* const ptr) {
	check_expected_ptr(ptr);

	return mock_ptr_type_checked(char*);
}


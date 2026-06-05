#ifndef ASSERTS_H
#define ASSERTS_H

#include <cmocka.h>
#include <string.h>

void _assert_nul(const void *a, const char * const ae, const char * const file, const int line) {
	if (a) {
		cmocka_print_error("%s is not NULL\n", ae);
		_fail(file, line);
	}
}
#define assert_nul(a) _assert_nul(a, #a, __FILE__, __LINE__)

void _assert_non_nul(const void *a, const char * const ae, const char * const file, const int line) {
	if (!a) {
		cmocka_print_error("%s is NULL\n", ae);
		_fail(file, line);
	}
}
#define assert_non_nul(a) _assert_non_nul(a, #a, __FILE__, __LINE__)

void _assert_str_equal(const char * const a, const char * const ae, const char * const b, const char * const be, const char * const file, const int line) {
	if (!a && !b)
		return;
	_assert_non_nul(a, ae, file, line);
	_assert_non_nul(b, be, file, line);
	_assert_string_equal(a, b, file, line);
}
#define assert_str_equal(a, b) _assert_str_equal(a, #a, b, #b, __FILE__, __LINE__)

void _assert_str_equal_n(const char * const a, const char * const ae, const char * const b, const char * const be, const size_t n, const char * const file, const int line) {
	if (!a && !b)
		return;
	_assert_non_nul(a, ae, file, line);
	_assert_non_nul(b, be, file, line);
	if (strncmp(a, b, n) != 0) {
		cmocka_print_error("\"%.*s\" != \"%.*s\"\n", (int)n, a, (int)n, b);
		_fail(file, line);
	}
}

#define assert_str_equal_n(a, b, n) _assert_str_equal_n(a, #a, b, #b, n, __FILE__, __LINE__)

#endif // ASSERTS_H

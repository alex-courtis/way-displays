#ifndef ASSERTS_H
#define ASSERTS_H

#include <cmocka.h>
#include <string.h>

#include "fn.h"
#include "pset.h"
#include "slist.h"
#include "sset.h"

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

void _assert_slist_equal(struct SList *a, struct SList *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (!slist_equal(a, b, equal)) {
		cmocka_print_error("\n%s != \n%s", slist_str(a, str), slist_str(b, str));
		_fail(file, line);
	}
}
#define assert_slist_equal(a, b, equal, str) _assert_slist_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_slist_not_equal(struct SList *a, struct SList *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (slist_equal(a, b, equal)) {
		cmocka_print_error("\n%s == \n%s", slist_str(a, str), slist_str(b, str));
		_fail(file, line);
	}
}
#define assert_slist_not_equal(a, b, equal, str) _assert_slist_not_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_sset_equal(const struct SSet *a, const struct SSet *b, const char * const file, const int line) {
	if (!sset_equal(a, b)) {
		cmocka_print_error("\n%s != \n%s", sset_str(a), sset_str(b));
		_fail(file, line);
	}
}
#define assert_sset_equal(a, b) _assert_sset_equal(a, b, __FILE__, __LINE__)

void _assert_sset_not_equal(const struct SSet *a, const struct SSet *b, const char * const file, const int line) {
	if (sset_equal(a, b)) {
		cmocka_print_error("\n%s == \n%s", sset_str(a), sset_str(b));
		_fail(file, line);
	}
}
#define assert_sset_not_equal(a, b) _assert_sset_not_equal(a, b, __FILE__, __LINE__)

void _assert_pset_equal(const struct PSet *a, const struct PSet *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (!pset_equal(a, b, equal)) {
		cmocka_print_error("\n%s != \n%s", pset_str(a, str), pset_str(b, str));
		_fail(file, line);
	}
}
#define assert_pset_equal(a, b, equal, str) _assert_pset_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_pset_not_equal(const struct PSet *a, const struct PSet *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (pset_equal(a, b, equal)) {
		cmocka_print_error("\n%s == \n%s", pset_str(a, str), pset_str(b, str));
		_fail(file, line);
	}
}
#define assert_pset_not_equal(a, b, equal, str) _assert_pset_not_equal(a, b, equal, str, __FILE__, __LINE__)

#endif // ASSERTS_H

#ifndef ASSERT_IMAP_H
#define ASSERT_IMAP_H

#include <cmocka.h>

#include "util-file.h"

#include "imap.h"

void _assert_imap_equal(const struct IMap *a, const struct IMap *b, const char * const file, const int line) {
	if (!imap_equal(a, b)) {
		write_file("actual.imap", imap_str(a));
		write_file("expected.imap", imap_str(b));
		cmocka_print_error("\n%s != \n%s", imap_str(a), imap_str(b));
		_fail(file, line);
	}
}
#define assert_imap_equal(a, b) _assert_imap_equal(a, b, __FILE__, __LINE__)

void _assert_imap_not_equal(const struct IMap *a, const struct IMap *b, const char * const file, const int line) {
	if (imap_equal(a, b)) {
		write_file("actual.imap", imap_str(a));
		write_file("expected.imap", imap_str(b));
		cmocka_print_error("\n%s == \n%s", imap_str(a), imap_str(b));
		_fail(file, line);
	}
}
#define assert_imap_not_equal(a, b) _assert_imap_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_IMAP_H

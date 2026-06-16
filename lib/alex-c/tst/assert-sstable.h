#ifndef ASSERT_SSTABLE_H
#define ASSERT_SSTABLE_H

#include <cmocka.h>

#include "util-file.h"

#include "sstable.h"

void _assert_sstable_equal(const struct SSTable *a, const struct SSTable *b, const char * const file, const int line) {
	if (!sstable_equal(a, b)) {
		write_file("actual.sstable", sstable_str(a));
		write_file("expected.sstable", sstable_str(b));
		cmocka_print_error("\n%s != \n%s", sstable_str(a), sstable_str(b));
		_fail(file, line);
	}
}
#define assert_sstable_equal(a, b) _assert_sstable_equal(a, b, __FILE__, __LINE__)

void _assert_sstable_not_equal(const struct SSTable *a, const struct SSTable *b, const char * const file, const int line) {
	if (sstable_equal(a, b)) {
		write_file("actual.sstable", sstable_str(a));
		write_file("expected.sstable", sstable_str(b));
		cmocka_print_error("\n%s == \n%s", sstable_str(a), sstable_str(b));
		_fail(file, line);
	}
}
#define assert_sstable_not_equal(a, b) _assert_sstable_not_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_SSTABLE_H

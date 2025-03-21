#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"

#include "fs.h"

void clean_dirs(void) {
	rmdir("tst/mkdir_p/foo/bar");
	rmdir("tst/mkdir_p/foo");
	rmdir("tst/mkdir_p");

	struct stat sb;
	assert_int_equal(stat("tst/mkdir_p", &sb), -1);
	assert_int_equal(errno, ENOENT);
}

int before_all(void **state) {
	clean_dirs();
	return 0;
}

int after_all(void **state) {
	clean_dirs();
	return 0;
}

int before_each(void **state) {
	clean_dirs();
	return 0;
}

int after_each(void **state) {
	clean_dirs();
	return 0;
}

void mkdir_p__no_perm(void **state) {
	assert_true(mkdir_p("tst/mkdir_p", 0755));
	assert_true(mkdir_p("tst/mkdir_p/foo", 0555));

	assert_false(mkdir_p("tst/mkdir_p/foo/bar", 0755));

	assert_log(ERROR, "\nCannot create directory tst/mkdir_p/foo/bar\n");

	struct stat sb;
	assert_int_equal(stat("tst/mkdir_p/foo/bar", &sb), -1);
	assert_int_equal(errno, ENOENT);
}

void mkdir_p__ok(void **state) {
	assert_true(mkdir_p("tst/mkdir_p/foo", 0755));

	struct stat sb;
	assert_int_equal(stat("tst/mkdir_p/foo", &sb), 0);
}

void mkdir_p__exists(void **state) {
	mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR;
	mode |=       S_IRGRP | S_IXGRP;
	mode |=       S_IROTH | S_IXOTH;

	assert_true(mkdir_p("tst/mkdir_p/foo", mode));

	assert_true(mkdir_p("tst/mkdir_p/foo", mode));

	struct stat sb;
	assert_int_equal(stat("tst/mkdir_p/foo", &sb), 0);
}


int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(mkdir_p__no_perm),
		TEST(mkdir_p__ok),
		TEST(mkdir_p__exists),
	};

	return RUN(tests);
}


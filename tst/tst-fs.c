#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"

#include "fs.h"

void clean_files(void) {
	rmdir("tst/tmp/mkdir_p/foo/bar");
	rmdir("tst/tmp/mkdir_p/foo");
	rmdir("tst/tmp/mkdir_p");
}


struct State {
	struct Cfg *from;
	struct Cfg *to;
	struct Cfg *expected;
};

int before_all(void **state) {
	mkdir("tst/tmp", 0755);
	return 0;
}

int after_all(void **state) {
	rmdir("tst/tmp");
	return 0;
}

int before_each(void **state) {
	clean_files();

	return 0;
}

int after_each(void **state) {
	clean_files();

	return 0;
}


void mkdir_p__no_perm(void **state) {
	assert_false(mkdir_p("/foo/bar", 0755));

	assert_log(ERROR, "\nCannot create directory /foo\n");

	struct stat sb;
	assert_int_equal(stat("/foo/bar", &sb), -1);
	assert_int_equal(errno, ENOENT);
}

void mkdir_p__ok(void **state) {
	assert_true(mkdir_p("tst/tmp/mkdir_p/foo/bar", 0755));

	struct stat sb;
	assert_int_equal(stat("tst/tmp/mkdir_p/foo/bar", &sb), 0);
}

void mkdir_p__exists(void **state) {
	mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR;
	mode |=       S_IRGRP | S_IXGRP;
	mode |=       S_IROTH | S_IXOTH;

	assert_true(mkdir_p("tst/tmp/mkdir_p/foo/bar", mode));

	assert_true(mkdir_p("tst/tmp/mkdir_p/foo/bar", mode));

	struct stat sb;
	assert_int_equal(stat("tst/tmp/mkdir_p/foo/bar", &sb), 0);
}


int main(void) {
	const struct CMUnitTest tests[] = {
		// TEST(mkdir_p__no_perm),
		TEST(mkdir_p__ok),
		TEST(mkdir_p__exists),
	};

	return RUN(tests);
}


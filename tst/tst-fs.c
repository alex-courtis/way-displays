#include "tst.h"

#include "asserts.h"
#include "assert-log.h"
#include "util-file.h"

#include <cmocka.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "str.h"

#include "fs.h"

static void clean_files_dirs(void) {
	chmod("tst/tmp/fs_mkdir_p/notwritable", 0755);
	rmdir("tst/tmp/fs_mkdir_p/notwritable");
	rmdir("tst/tmp/fs_mkdir_p/writable/bar");
	rmdir("tst/tmp/fs_mkdir_p/writable");
	rmdir("tst/tmp/fs_mkdir_p/foo/bar");
	rmdir("tst/tmp/fs_mkdir_p/foo");
	rmdir("tst/tmp/fs_mkdir_p");

	chmod("tst/tmp/fs_canonical_path/noperms", 0755);
	remove("tst/tmp/fs_canonical_path/file.yaml");
	remove("tst/tmp/fs_canonical_path/inexistent.yaml");
	remove("tst/tmp/fs_canonical_path/noperms/file.yaml");
	remove("tst/tmp/fs_canonical_path/resolved.yaml");
	remove("tst/tmp/fs_canonical_path/sub/link.yaml");
	rmdir("tst/tmp/fs_canonical_path/noperms");
	rmdir("tst/tmp/fs_canonical_path/sub");
	rmdir("tst/tmp/fs_canonical_path");

	chmod("tst/tmp/fs_file_write/noperms", 0755);
	remove("tst/tmp/fs_file_write/new");
	remove("tst/tmp/fs_file_write/existing");
	remove("tst/tmp/fs_file_write/empty");
	remove("tst/tmp/fs_file_write/close_fail");
	rmdir("tst/tmp/fs_file_write/noperms");
	rmdir("tst/tmp/fs_file_write");

	rmdir("tst/tmp");

	struct stat sb;
	assert_int_equal(stat("tst/tmp", &sb), -1);
	assert_int_equal(errno, ENOENT);
}

static void create_dirs(void) {
	mkdir("tst/tmp", 0755);
	mkdir("tst/tmp/fs_canonical_path", 0755);
	mkdir("tst/tmp/fs_canonical_path/sub", 0755);
	mkdir("tst/tmp/fs_canonical_path/noperms", 0755);
	mkdir("tst/tmp/fs_file_write", 0755);
}

static int before_each(void **state) {
	clean_files_dirs();

	create_dirs();

	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	clean_files_dirs();

	return 0;
}

static void fs_mkdir_p__null(void **state) {
	assert_false(fs_mkdir_p(NULL, 0755));
}

static void fs_mkdir_p__no_perm(void **state) {
	assert_true(fs_mkdir_p("tst/tmp/fs_mkdir_p", 0755));
	assert_true(fs_mkdir_p("tst/tmp/fs_mkdir_p/writable", 0555));

	assert_false(fs_mkdir_p("tst/tmp/fs_mkdir_p/writable/bar", 0755));

	struct stat sb;
	assert_int_equal(stat("tst/tmp/fs_mkdir_p/writable/bar", &sb), -1);
	assert_int_equal(errno, ENOENT);
}

static void fs_mkdir_p__bad_perms(void **state) {
	assert_true(fs_mkdir_p("tst/tmp/fs_mkdir_p", 0755));
	assert_false(fs_mkdir_p("tst/tmp/fs_mkdir_p/notwritable/foo/bar", 0555));

	struct stat sb;
	assert_int_equal(stat("tst/tmp/fs_mkdir_p/notwritable/foo", &sb), -1);
	assert_int_equal(errno, ENOENT);
}

static void fs_mkdir_p__ok(void **state) {
	assert_true(fs_mkdir_p("tst/tmp/fs_mkdir_p/foo", 0755));

	struct stat sb;
	assert_int_equal(stat("tst/tmp/fs_mkdir_p/foo", &sb), 0);
}

static void fs_mkdir_p__exists(void **state) {
	mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR;
	mode |=       S_IRGRP | S_IXGRP;
	mode |=       S_IROTH | S_IXOTH;

	assert_true(fs_mkdir_p("tst/tmp/fs_mkdir_p/foo", mode));

	assert_true(fs_mkdir_p("tst/tmp/fs_mkdir_p/foo", mode));

	struct stat sb;
	assert_int_equal(stat("tst/tmp/fs_mkdir_p/foo", &sb), 0);
}

static void fs_canonical_path__null(void **state) {
	assert_nul(fs_canonical_path(NULL));
}

static void fs_canonical_path__not_found(void **state) {
	char cwd[PATH_MAX];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	char *file_path = sprintf_alloc("%s/tst/tmp/fs_canonical_path/inexistent.yaml", cwd);

	assert_nul(fs_canonical_path(file_path));

	free(file_path);
}

static void fs_canonical_path__direct(void **state) {
	char cwd[PATH_MAX];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	char *file_path = sprintf_alloc("%s/tst/tmp/fs_canonical_path/file.yaml", cwd);

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	if (f)
		fclose(f);

	char *actual = fs_canonical_path(file_path);

	assert_str_equal(actual, file_path);

	free(actual);
	free(file_path);
}

static void fs_canonical_path__linked(void **state) {
	char cwd[PATH_MAX];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	char *file_path = sprintf_alloc("%s/tst/tmp/fs_canonical_path/file.yaml", cwd);
	char *link_path = sprintf_alloc("%s/tst/tmp/fs_canonical_path/sub/link.yaml", cwd);

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	if (f)
		fclose(f);

	assert_int_equal(symlink(file_path, link_path), 0);

	char *actual = fs_canonical_path(link_path);

	assert_str_equal(actual, file_path);

	free(actual);
	free(file_path);
	free(link_path);
}

static void fs_canonical_path__link_broken(void **state) {
	char cwd[PATH_MAX];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	char *file_path = sprintf_alloc("%s/tst/tmp/fs_canonical_path/file.yaml", cwd);
	char *link_path = sprintf_alloc("%s/tst/tmp/fs_canonical_path/sub/link.yaml", cwd);

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	if (f)
		fclose(f);

	assert_int_equal(symlink(file_path, link_path), 0);

	remove(file_path);

	assert_nul(fs_canonical_path(link_path));

	free(file_path);
	free(link_path);
}

static void fs_canonical_path__no_dir_perms(void **state) {
	char cwd[PATH_MAX];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	char *dir_path =  sprintf_alloc("%s/tst/tmp/fs_canonical_path/noperms", cwd);
	char *file_path = sprintf_alloc("%s/tst/tmp/fs_canonical_path/noperms/file.yaml", cwd);

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	if (f)
		fclose(f);

	assert_int_equal(chmod(dir_path, 0444), 0);

	assert_nul(fs_canonical_path(file_path));

	free(file_path);
	free(dir_path);
}

static void fs_canonical_path__no_file_perms(void **state) {
	char cwd[PATH_MAX];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	char *dir_path =  sprintf_alloc("%s/tst/tmp/fs_canonical_path/noperms", cwd);
	char *file_path = sprintf_alloc("%s/tst/tmp/fs_canonical_path/noperms/file.yaml", cwd);

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	if (f)
		fclose(f);

	assert_int_equal(chmod(file_path, 0000), 0);

	assert_nul(fs_canonical_path(file_path));

	free(file_path);
	free(dir_path);
}

static void fs_file_write__nulls(void **state) {
	assert_false(fs_file_write(NULL, "foo", "w"));
	assert_false(fs_file_write("foo", "foo", NULL));
}

static void fs_file_write__empty(void **state) {
	char *path = "tst/tmp/fs_file_write/empty";

	assert_true(fs_file_write(path, NULL, "w"));

	struct stat sb;
	assert_int_equal(stat(path, &sb), 0);

	char *read = read_file(path);

	assert_str_equal(read, "");

	free(read);
}

static void fs_file_write__new(void **state) {
	char *path = "tst/tmp/fs_file_write/new";

	assert_true(fs_file_write(path, "foo\nbar\n", "w"));

	struct stat sb;
	assert_int_equal(stat(path, &sb), 0);

	char *read = read_file(path);

	assert_str_equal(read, "foo\nbar\n");

	free(read);
}

static void fs_file_write__append(void **state) {
	char *path = "tst/tmp/fs_file_write/existing";

	assert_true(fs_file_write(path, "1", "w"));
	assert_true(fs_file_write(path, "2", "a"));

	struct stat sb;
	assert_int_equal(stat(path, &sb), 0);

	char *read = read_file(path);

	assert_str_equal(read, "12");

	free(read);
}

static void fs_file_write__bad_path(void **state) {
	char *path = "tst/tmp/fs_file_write/bad/path";

	assert_false(fs_file_write(path, "1", "w"));

	assert_int_equal(errno, ENOENT);

	struct stat sb;
	assert_int_equal(stat(path, &sb), -1);
}

static void fs_file_write__no_perms(void **state) {
	mkdir("tst/tmp/fs_file_write/noperms", 0555);

	char *path = "tst/tmp/fs_file_write/noperms/foo";

	assert_false(fs_file_write(path, "1", "w"));

	assert_int_equal(errno, EACCES);

	struct stat sb;
	assert_int_equal(stat(path, &sb), -1);
}

static void fs_file_write__bad_mode(void **state) {
	mkdir("tst/tmp/fs_file_write/noperms", 0555);

	char *path = "tst/tmp/fs_file_write/noperms/foo";

	assert_false(fs_file_write(path, "1", "z"));

	assert_int_equal(errno, EINVAL);

	struct stat sb;
	assert_int_equal(stat(path, &sb), -1);
}

static void fs_file_write__close_fail(void **state) {
	char *path = "tst/tmp/fs_file_write/close_fail";

	will_return_int(__wrap_fclose, 1);

	assert_false(fs_file_write(path, "foo\nbar\n", "w"));

	// it will be created anyway, we just returned false
	struct stat sb;
	assert_int_equal(stat(path, &sb), 0);

	char *read = read_file(path);

	assert_str_equal(read, "foo\nbar\n");

	free(read);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(fs_mkdir_p__null),
		TEST_BA(fs_mkdir_p__no_perm),
		TEST_BA(fs_mkdir_p__bad_perms),
		TEST_BA(fs_mkdir_p__ok),
		TEST_BA(fs_mkdir_p__exists),

		TEST_BA(fs_canonical_path__null),
		TEST_BA(fs_canonical_path__not_found),
		TEST_BA(fs_canonical_path__direct),
		TEST_BA(fs_canonical_path__linked),
		TEST_BA(fs_canonical_path__link_broken),
		TEST_BA(fs_canonical_path__no_dir_perms),
		TEST_BA(fs_canonical_path__no_file_perms),

		TEST_BA(fs_file_write__nulls),
		TEST_BA(fs_file_write__empty),
		TEST_BA(fs_file_write__new),
		TEST_BA(fs_file_write__append),
		TEST_BA(fs_file_write__bad_path),
		TEST_BA(fs_file_write__no_perms),
		TEST_BA(fs_file_write__bad_mode),
		TEST_BA(fs_file_write__close_fail),
	};

	return RUN(tests);
}


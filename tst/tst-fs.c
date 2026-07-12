#include "tst.h"

#include "asserts.h"
#include "assert-log.h"

#include <cmocka.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cfg/file.h"
#include "log.h"
#include "pslist.h"
#include "str.h"

#include "fs.h"

static void clean_dirs(void) {
	rmdir("tst/mkdir_p/foo/bar");
	rmdir("tst/mkdir_p/foo");
	rmdir("tst/mkdir_p");

	chmod("tst/resolve/noperms", 0755);
	rmdir("tst/resolve/noperms");
	rmdir("tst/resolve/sub");
	rmdir("tst/resolve");

	struct stat sb;
	assert_int_equal(stat("tst/mkdir_p", &sb), -1);
	assert_int_equal(errno, ENOENT);
}

static void clean_files(void) {
	chmod("tst/resolve/noperms", 0755);

	remove("tst/resolve/resolved.yaml");
	remove("tst/resolve/inexistent.yaml");
	remove("tst/resolve/sub/link.yaml");
	remove("tst/resolve/noperms/file.yaml");
}

static void create_dirs(void) {
	mkdir("tst/resolve", 0755);
	mkdir("tst/resolve/sub", 0755);
	mkdir("tst/resolve/noperms", 0755);
}

static int before_each(void **state) {
	clean_dirs();
	clean_files();
	create_dirs();

	pslist_free_vals(&g_cfg_file_paths, NULL);

	g_cfg_file_init();

	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	clean_files();
	clean_dirs();

	g_cfg_file_destroy();

	pslist_free_vals(&g_cfg_file_paths, NULL);

	return 0;
}

static void mkdir_p__no_perm(void **state) {
	assert_true(mkdir_p("tst/mkdir_p", 0755));
	assert_true(mkdir_p("tst/mkdir_p/foo", 0555));

	assert_false(mkdir_p("tst/mkdir_p/foo/bar", 0755));

	assert_log(ERROR, "\nCannot create directory tst/mkdir_p/foo/bar\n");

	struct stat sb;
	assert_int_equal(stat("tst/mkdir_p/foo/bar", &sb), -1);
	assert_int_equal(errno, ENOENT);
}

static void mkdir_p__ok(void **state) {
	assert_true(mkdir_p("tst/mkdir_p/foo", 0755));

	struct stat sb;
	assert_int_equal(stat("tst/mkdir_p/foo", &sb), 0);
}

static void mkdir_p__exists(void **state) {
	mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR;
	mode |=       S_IRGRP | S_IXGRP;
	mode |=       S_IROTH | S_IXOTH;

	assert_true(mkdir_p("tst/mkdir_p/foo", mode));

	assert_true(mkdir_p("tst/mkdir_p/foo", mode));

	struct stat sb;
	assert_int_equal(stat("tst/mkdir_p/foo", &sb), 0);
}

static void g_cfg_file_resolve1__not_found(void **state) {
	char cwd[PATH_MAX];
	char file_path[PATH_MAX + 20];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	snprintf(file_path, sizeof(file_path), "%s/inexistent.yaml", cwd);

	pslist_append(&g_cfg_file_paths, strdup(file_path));

	assert_false(g_cfg_file_resolve1());

	assert_nul(g_cfg_file->file_path);
	assert_nul(g_cfg_file->dir_path);
	assert_nul(g_cfg_file->file_name);
	assert_nul(g_cfg_file->resolved_path);
}

static void g_cfg_file_resolve1__direct(void **state) {
	char cwd[PATH_MAX];
	char dir_path[PATH_MAX + 20];
	char file_path[PATH_MAX + 40];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	snprintf(dir_path, sizeof(dir_path), "%s/tst/resolve", cwd);
	snprintf(file_path, sizeof(file_path), "%s/resolved.yaml", dir_path);
	pslist_append(&g_cfg_file_paths, strdup(file_path));

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	if (f) {
		fclose(f);
	}

	assert_true(g_cfg_file_resolve1());

	assert_str_equal(g_cfg_file->file_path, file_path);
	assert_str_equal(g_cfg_file->dir_path, dir_path);
	assert_str_equal(g_cfg_file->file_name, "resolved.yaml");
	assert_str_equal(g_cfg_file->resolved_path, file_path);
	assert_ptr_equal(g_cfg_file->resolved_path, pslist_at(g_cfg_file_paths, 0));
}

static void g_cfg_file_resolve1__linked(void **state) {
	char cwd[PATH_MAX];
	char dir_path[PATH_MAX + 20];
	char file_path[PATH_MAX + 40];
	char linked_path[PATH_MAX + 50];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	snprintf(dir_path, sizeof(dir_path), "%s/tst/resolve", cwd);
	snprintf(file_path, sizeof(file_path), "%s/resolved.yaml", dir_path);
	snprintf(linked_path, sizeof(linked_path), "%s/tst/resolve/sub/link.yaml", cwd);
	pslist_append(&g_cfg_file_paths, strdup(linked_path));

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	if (f) {
		fclose(f);
	}
	assert_int_equal(symlink(file_path, linked_path), 0);

	assert_true(g_cfg_file_resolve1());

	assert_str_equal(g_cfg_file->file_path, file_path);
	assert_str_equal(g_cfg_file->dir_path, dir_path);
	assert_str_equal(g_cfg_file->file_name, "resolved.yaml");
	assert_str_equal(g_cfg_file->resolved_path, linked_path);
	assert_ptr_equal(g_cfg_file->resolved_path, pslist_at(g_cfg_file_paths, 0));
}

static void g_cfg_file_resolve__not_found(void **state) {
	char cwd[PATH_MAX];
	char file_path[PATH_MAX + 20];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	snprintf(file_path, sizeof(file_path), "%s/inexistent.yaml", cwd);

	pslist_append(&g_cfg_file_paths, strdup(file_path));

	assert_false(g_cfg_file_resolve());

	assert_nul(g_cfg_file->file_path);
	assert_nul(g_cfg_file->dir_path);
	assert_nul(g_cfg_file->file_name);
	assert_nul(g_cfg_file->resolved_path);
}

static void g_cfg_file_resolve__direct(void **state) {
	char cwd[PATH_MAX];
	char dir_path[PATH_MAX + 20];
	char file_path[PATH_MAX + 40];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	snprintf(dir_path, sizeof(dir_path), "%s/tst/resolve", cwd);
	snprintf(file_path, sizeof(file_path), "%s/resolved.yaml", dir_path);
	pslist_append(&g_cfg_file_paths, strdup(file_path));

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	if (f) {
		fclose(f);
	}

	assert_true(g_cfg_file_resolve());

	assert_str_equal(g_cfg_file->file_path, file_path);
	assert_str_equal(g_cfg_file->dir_path, dir_path);
	assert_str_equal(g_cfg_file->file_name, "resolved.yaml");
	assert_str_equal(g_cfg_file->resolved_path, file_path);
	assert_ptr_equal(g_cfg_file->resolved_path, pslist_at(g_cfg_file_paths, 0));
}

static void g_cfg_file_resolve__linked(void **state) {
	char cwd[PATH_MAX];
	char dir_path[PATH_MAX + 20];
	char file_path[PATH_MAX + 40];
	char linked_path[PATH_MAX + 50];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	snprintf(dir_path, sizeof(dir_path), "%s/tst/resolve", cwd);
	snprintf(file_path, sizeof(file_path), "%s/resolved.yaml", dir_path);
	snprintf(linked_path, sizeof(linked_path), "%s/tst/resolve/sub/link.yaml", cwd);
	pslist_append(&g_cfg_file_paths, strdup(linked_path));

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	if (f) {
		fclose(f);
	}
	assert_int_equal(symlink(file_path, linked_path), 0);

	assert_true(g_cfg_file_resolve());

	assert_str_equal(g_cfg_file->file_path, file_path);
	assert_str_equal(g_cfg_file->dir_path, dir_path);
	assert_str_equal(g_cfg_file->file_name, "resolved.yaml");
	assert_str_equal(g_cfg_file->resolved_path, linked_path);
	assert_ptr_equal(g_cfg_file->resolved_path, pslist_at(g_cfg_file_paths, 0));
}

static void canonical_path__not_found(void **state) {
	char cwd[PATH_MAX];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	char *file_path = sprintf_alloc("%s/tst/resolve/inexistent.yaml", cwd);

	assert_nul(canonical_path(file_path));

	free(file_path);
}

static void canonical_path__direct(void **state) {
	char cwd[PATH_MAX];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	char *file_path = sprintf_alloc("%s/tst/resolve/file.yaml", cwd);

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	fclose(f);

	char *actual = canonical_path(file_path);

	assert_str_equal(actual, file_path);

	free(actual);
	free(file_path);
}

static void canonical_path__linked(void **state) {
	char cwd[PATH_MAX];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	char *file_path = sprintf_alloc("%s/tst/resolve/file.yaml", cwd);
	char *link_path = sprintf_alloc("%s/tst/resolve/sub/link.yaml", cwd);

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	fclose(f);

	assert_int_equal(symlink(file_path, link_path), 0);

	char *actual = canonical_path(link_path);

	assert_str_equal(actual, file_path);

	free(actual);
	free(file_path);
	free(link_path);
}

static void canonical_path__link_broken(void **state) {
	char cwd[PATH_MAX];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	char *file_path = sprintf_alloc("%s/tst/resolve/file.yaml", cwd);
	char *link_path = sprintf_alloc("%s/tst/resolve/sub/link.yaml", cwd);

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	fclose(f);

	assert_int_equal(symlink(file_path, link_path), 0);

	remove(file_path);

	assert_nul(canonical_path(link_path));

	free(file_path);
	free(link_path);
}

static void canonical_path__no_dir_perms(void **state) {
	char cwd[PATH_MAX];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	char *dir_path =  sprintf_alloc("%s/tst/resolve/noperms", cwd);
	char *file_path = sprintf_alloc("%s/tst/resolve/noperms/file.yaml", cwd);

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	fclose(f);

	assert_int_equal(chmod(dir_path, 0444), 0);

	assert_nul(canonical_path(file_path));

	free(file_path);
	free(dir_path);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(mkdir_p__no_perm),
		TEST_BA(mkdir_p__ok),
		TEST_BA(mkdir_p__exists),

		TEST_BA(g_cfg_file_resolve__not_found),
		TEST_BA(g_cfg_file_resolve__direct),
		TEST_BA(g_cfg_file_resolve__linked),

		TEST_BA(g_cfg_file_resolve1__not_found),
		TEST_BA(g_cfg_file_resolve1__direct),
		TEST_BA(g_cfg_file_resolve1__linked),

		TEST_BA(canonical_path__not_found),
		TEST_BA(canonical_path__direct),
		TEST_BA(canonical_path__linked),
		TEST_BA(canonical_path__link_broken),
		TEST_BA(canonical_path__no_dir_perms),
	};

	return RUN(tests);
}


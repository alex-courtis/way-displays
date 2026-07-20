#include "tst.h"

#include "assert-log.h"
#include "asserts.h"
#include "expects.h"

#include <cmocka.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg/cfg.h"
#include "enum.h"
#include "sset.h"

#include "cfg/file.h"

void candidates_init(const char *user_path);

extern const struct Sset *g_candidates;

// this is local to this test, as some asserts/expects will call fs_file_write
// cppcheck-suppress staticFunction
bool __wrap_fs_file_write(const char *path, const char *contents, const char *mode) {
	check_expected_ptr(path);
	check_expected_ptr(contents);
	check_expected_ptr(mode);
	return mock_type(bool);
}

static int before_each(void **state) {
	g_candidates = sset_init();

	memset(&g_cfg_file, 0, sizeof(struct CfgFile));

	return 0;
}

static int after_each(void **state) {
	sset_free(g_candidates);
	g_candidates = NULL;

	memset(&g_cfg_file, 0, sizeof(struct CfgFile));

	g_cfg_destroy();

	return 0;
}

static void g_cfg_file_write__bad_yaml(void **state) {
	strncpy(g_cfg_file.file_path, "something", PATH_MAX);

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, NULL, char*);

	g_cfg_file_write();

	assert_logs_empty();
}

static void g_cfg_file_write__none(void **state) {
	sset_add(g_candidates, "/path/zero/0");

	char *expected = strdup("XXXX");

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, expected, char*);

	expect_function_call(__wrap_fd_wd_cfg_dir_destroy);

	expect_str(__wrap_fs_mkdir_p, path, "/path/zero");
	expect_int_value(__wrap_fs_mkdir_p, mode, 0755);
	will_return_int(__wrap_fs_mkdir_p, true);

	expect_str(__wrap_fs_file_write, path, "/path/zero/0");
	expect_str(__wrap_fs_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_fs_file_write, mode, "w");
	will_return_int(__wrap_fs_file_write, true);

	expect_str(__wrap_fs_file_write, path, "/path/zero/0");
	expect_str(__wrap_fs_file_write, contents, expected);
	expect_str(__wrap_fs_file_write, mode, "a");
	will_return_int(__wrap_fs_file_write, true);

	expect_function_call(__wrap_fd_wd_cfg_dir_create);

	g_cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: /path/zero/0\n");

	assert_str_equal(g_cfg_file.file_path, "/path/zero/0");
	assert_str_equal(g_cfg_file.dir_path, "/path/zero");
	assert_str_equal(g_cfg_file.file_name, "0");
	assert_str_equal(g_cfg_file.file_path_resolved, "/path/zero/0");
	assert_false(g_cfg_file.written);

	assert_logs_empty();
}

static void g_cfg_file_write__cannot_write_use_alternative(void **state) {
	sset_add(g_candidates, "/path/zero/0");
	sset_add(g_candidates, "/path/one/1");
	sset_add(g_candidates, "/path/two/2");
	sset_add(g_candidates, "/path/three/3");
	sset_add(g_candidates, "/path/four/4");

	strncpy(g_cfg_file.file_path, "/path/two/2", PATH_MAX - 1);
	strncpy(g_cfg_file.dir_path, "nothing", PATH_MAX - 1);
	strncpy(g_cfg_file.file_name, "missing", PATH_MAX - 1);
	strncpy(g_cfg_file.file_path_resolved, "/path/two/2", PATH_MAX - 1);

	char *expected = strdup("XXXXxxxX");

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, strdup(expected), char*);

	expect_function_call(__wrap_fd_wd_cfg_dir_destroy);

	// 2 fails header write
	expect_str(__wrap_fs_file_write, path, "/path/two/2");
	expect_str(__wrap_fs_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_fs_file_write, mode, "w");
	will_return_int(__wrap_fs_file_write, false);

	// 0 creates directory, fails header write
	expect_str(__wrap_fs_mkdir_p, path, "/path/zero");
	expect_int_value(__wrap_fs_mkdir_p, mode, 0755);
	will_return_int(__wrap_fs_mkdir_p, true);

	expect_str(__wrap_fs_file_write, path, "/path/zero/0");
	expect_str(__wrap_fs_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_fs_file_write, mode, "w");
	will_return_int(__wrap_fs_file_write, false);

	// 1 fails to create dir
	expect_str(__wrap_fs_mkdir_p, path, "/path/one");
	expect_int_value(__wrap_fs_mkdir_p, mode, 0755);
	will_return_int(__wrap_fs_mkdir_p, false);

	// 2 already tried

	// 3 creates dir and writes OK
	expect_str(__wrap_fs_mkdir_p, path, "/path/three");
	expect_int_value(__wrap_fs_mkdir_p, mode, 0755);
	will_return_int(__wrap_fs_mkdir_p, true);

	expect_str(__wrap_fs_file_write, path, "/path/three/3");
	expect_str(__wrap_fs_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_fs_file_write, mode, "w");
	will_return_int(__wrap_fs_file_write, true);

	expect_str(__wrap_fs_file_write, path, "/path/three/3");
	expect_str(__wrap_fs_file_write, contents, expected);
	expect_str(__wrap_fs_file_write, mode, "a");
	will_return_int(__wrap_fs_file_write, true);

	expect_function_call(__wrap_fd_wd_cfg_dir_create);

	g_cfg_file_write();

	assert_log(ERROR,
			"\nUnable to write to /path/two/2\n"
			"\nUnable to write to /path/zero/0\n"
			"\nCannot create directory /path/one\n"
			);
	assert_log(INFO, "\nWrote configuration file: /path/three/3\n");

	assert_str_equal(g_cfg_file.file_path, "/path/three/3");
	assert_str_equal(g_cfg_file.dir_path, "/path/three");
	assert_str_equal(g_cfg_file.file_name, "3");
	assert_str_equal(g_cfg_file.file_path_resolved, "/path/three/3");
	assert_false(g_cfg_file.written);

	free(expected);

	assert_logs_empty();
}

static void g_cfg_file_write__cannot_write_no_alternative(void **state) {
	sset_add(g_candidates, "/path/zero/0");
	sset_add(g_candidates, "/path/one/1");

	strncpy(g_cfg_file.file_path, "/path/zero/0", PATH_MAX - 1);
	strncpy(g_cfg_file.dir_path, "/path/zero", PATH_MAX - 1);
	strncpy(g_cfg_file.file_name, "one", PATH_MAX - 1);
	strncpy(g_cfg_file.file_path_resolved, "/path/zero/0", PATH_MAX - 1);

	char *expected = strdup("XXXX");

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, strdup(expected), char*);

	expect_function_call(__wrap_fd_wd_cfg_dir_destroy);

	// 0 fails file write existing
	expect_str(__wrap_fs_file_write, path, "/path/zero/0");
	expect_str(__wrap_fs_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_fs_file_write, mode, "w");
	will_return_int(__wrap_fs_file_write, false);

	// 1 creates dir but fails second write
	expect_str(__wrap_fs_mkdir_p, path, "/path/one");
	expect_int_value(__wrap_fs_mkdir_p, mode, 0755);
	will_return_int(__wrap_fs_mkdir_p, true);

	expect_str(__wrap_fs_file_write, path, "/path/one/1");
	expect_str(__wrap_fs_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_fs_file_write, mode, "w");
	will_return_int(__wrap_fs_file_write, true);

	expect_str(__wrap_fs_file_write, path, "/path/one/1");
	expect_str(__wrap_fs_file_write, contents, expected);
	expect_str(__wrap_fs_file_write, mode, "a");
	will_return_int(__wrap_fs_file_write, false);

	g_cfg_file_write();

	assert_str_equal(g_cfg_file.file_path, "");
	assert_str_equal(g_cfg_file.dir_path, "");
	assert_str_equal(g_cfg_file.file_name, "");
	assert_str_equal(g_cfg_file.file_path_resolved, "");
	assert_false(g_cfg_file.written);

	assert_log(ERROR,
			"\nUnable to write to /path/zero/0\n"
			"\nUnable to write to /path/one/1\n"
			);

	free(expected);

	assert_logs_empty();
}

static void g_cfg_file_write__existing(void **state) {
	strncpy(g_cfg_file.file_path, "tst/tmp/write-existing-cfg.yaml", PATH_MAX - 1);

	char *expected = strdup("XXXX");

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, strdup(expected), char*);

	expect_str(__wrap_fs_file_write, path, g_cfg_file.file_path);
	expect_str(__wrap_fs_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_fs_file_write, mode, "w");
	will_return_int(__wrap_fs_file_write, true);

	expect_str(__wrap_fs_file_write, path, g_cfg_file.file_path);
	expect_str(__wrap_fs_file_write, contents, expected);
	expect_str(__wrap_fs_file_write, mode, "a");
	will_return_int(__wrap_fs_file_write, true);

	g_cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: tst/tmp/write-existing-cfg.yaml\n");

	assert_true(g_cfg_file.written);

	free(expected);

	assert_logs_empty();
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(g_cfg_file_write__bad_yaml),
		TEST_BA(g_cfg_file_write__none),
		TEST_BA(g_cfg_file_write__cannot_write_use_alternative),
		TEST_BA(g_cfg_file_write__cannot_write_no_alternative),
		TEST_BA(g_cfg_file_write__existing),
	};

	return RUN(tests);
}


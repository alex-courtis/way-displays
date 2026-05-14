#include "tst.h"
#include "asserts.h"
#include "expects.h"

#include <cmocka.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "global.h"
#include "slist.h"
#include "log.h"

#include "server.h"
#include "cfg.h"

char *_dir_path = NULL;
char *_file_name = NULL;
char *_file_path = NULL;

bool __wrap_resolve_cfg_file(struct Cfg *cfg) {
	check_expected_ptr(cfg);

	cfg->dir_path = _dir_path ? strdup(_dir_path) : NULL;
	cfg->file_name = _file_name ? strdup(_file_name) : NULL;
	cfg->file_path = _file_path ? strdup(_file_path) : NULL;

	return mock_type(bool);
}

bool __wrap_unmarshal_cfg_from_file(struct Cfg *cfg) {
	check_expected_ptr(cfg);

	cfg->auto_scale_max = 888;
	cfg->log_threshold = FATAL;

	return mock_type(bool);
}

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	cfg_destroy();
	logs_clear();

	return 0;
}

int after_each(void **state) {
	cfg_destroy();

	free(_dir_path);
	_dir_path = NULL;
	free(_file_name);
	_file_name = NULL;
	free(_file_path);
	_file_path = NULL;

	return 0;
}

void load_cfg__no_file(void **state) {
	struct Cfg *cfg_orig = cfg;

	expect_any(__wrap_resolve_cfg_file, cfg);
	will_return_int(__wrap_resolve_cfg_file, false);

	load_cfg();

	assert_non_nul(cfg);
	assert_ptr_not_equal(cfg, cfg_orig);

	struct Cfg *expected_cfg = cfg_default();

	assert_cfg_equal(cfg, expected_cfg);
	assert_nul(cfg->file_name);
	assert_nul(cfg->file_path);
	assert_nul(cfg->dir_path);

	char *expected_log = read_file("tst/server/load-no-file.log");
	assert_log(INFO, expected_log);
	assert_logs_empty();

	free(expected_log);
	cfg_free(expected_cfg);
}

void load_cfg__invalid_file(void **state) {
	struct Cfg *cfg_orig = cfg;

	expect_any(__wrap_resolve_cfg_file, cfg);
	will_return_int(__wrap_resolve_cfg_file, true);

	expect_any(__wrap_unmarshal_cfg_from_file, cfg);
	will_return_int(__wrap_unmarshal_cfg_from_file, false);

	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	load_cfg();

	assert_non_nul(cfg);
	assert_ptr_not_equal(cfg, cfg_orig);

	struct Cfg *expected_cfg = cfg_default();

	assert_cfg_equal(cfg, expected_cfg);
	assert_str_equal(cfg->file_path, "file_path");
	assert_str_equal(cfg->file_name, "file_name");
	assert_str_equal(cfg->dir_path, "dir_path");

	char *expected_log = read_file("tst/server/load-invalid-file.log");
	assert_log(INFO, expected_log);
	assert_logs_empty();

	free(expected_log);
	cfg_free(expected_cfg);
}

void load_cfg__valid_file(void **state) {
	struct Cfg *cfg_orig = cfg;

	expect_any(__wrap_resolve_cfg_file, cfg);
	will_return_int(__wrap_resolve_cfg_file, true);

	expect_any(__wrap_unmarshal_cfg_from_file, cfg);
	will_return_int(__wrap_unmarshal_cfg_from_file, true);

	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	load_cfg();

	assert_non_nul(cfg);

	assert_ptr_not_equal(cfg, cfg_orig);

	struct Cfg *expected_cfg = cfg_default();
	expected_cfg->auto_scale_max = 888;
	expected_cfg->log_threshold = FATAL;

	assert_cfg_equal(cfg, expected_cfg);
	assert_str_equal(cfg->file_path, "file_path");
	assert_str_equal(cfg->file_name, "file_name");
	assert_str_equal(cfg->dir_path, "dir_path");

	char *expected_log = read_file("tst/server/load-valid-file.log");
	assert_log(INFO, expected_log);
	assert_logs_empty();

	free(expected_log);
	cfg_free(expected_cfg);
}

void reload_cfg__no_file(void **state) {
	cfg = cfg_default();
	cfg->auto_scale_max = 111;
	cfg->dir_path = strdup("foo");

	struct Cfg *cfg_orig = cfg;

	// no mock calls expected

	reload_cfg();

	assert_ptr_equal(cfg, cfg_orig);

	struct Cfg *expected_cfg = cfg_default();
	expected_cfg->auto_scale_max = 111;

	assert_cfg_equal(cfg, expected_cfg);
	assert_str_equal(cfg->dir_path, "foo");

	assert_logs_empty();

	cfg_free(expected_cfg);
}

void reload_cfg__invalid_file(void **state) {
	cfg = cfg_default();
	cfg->auto_scale_max = 111;

	cfg->file_path = strdup("file_path");
	cfg->file_name = strdup("file_name");
	cfg->dir_path = strdup("dir_path");

	struct Cfg *cfg_orig = cfg;

	expect_any(__wrap_unmarshal_cfg_from_file, cfg);
	will_return_int(__wrap_unmarshal_cfg_from_file, false);

	reload_cfg();

	assert_ptr_equal(cfg, cfg_orig);

	struct Cfg *expected_cfg = cfg_default();
	expected_cfg->auto_scale_max = 111;

	assert_cfg_equal(cfg, expected_cfg);
	assert_str_equal(cfg->file_path, "file_path");
	assert_str_equal(cfg->file_name, "file_name");
	assert_str_equal(cfg->dir_path, "dir_path");

	char *expected_log = read_file("tst/server/reload-invalid-file.log");
	assert_log(INFO, expected_log);
	assert_logs_empty();

	free(expected_log);
	cfg_free(expected_cfg);
}

void reload_cfg__valid_file(void **state) {
	cfg = cfg_default();
	cfg->auto_scale_max = 222;

	cfg->file_path = strdup("file_path");
	cfg->file_name = strdup("file_name");
	cfg->dir_path = strdup("dir_path");

	struct Cfg *cfg_orig = cfg;

	expect_any(__wrap_unmarshal_cfg_from_file, cfg);
	will_return_int(__wrap_unmarshal_cfg_from_file, true);

	expect_int_value(__wrap_log_set_threshold, threshold, FATAL);
	expect_int_value(__wrap_log_set_threshold, cli, false);

	reload_cfg();

	assert_ptr_not_equal(cfg, cfg_orig);

	struct Cfg *expected_cfg = cfg_default();
	expected_cfg->auto_scale_max = 888;
	expected_cfg->log_threshold = FATAL;

	assert_cfg_equal(cfg, expected_cfg);
	assert_str_equal(cfg->file_path, "file_path");
	assert_str_equal(cfg->file_name, "file_name");
	assert_str_equal(cfg->dir_path, "dir_path");

	char *expected_log = read_file("tst/server/reload-valid-file.log");
	assert_log(INFO, expected_log);
	assert_logs_empty();

	free(expected_log);
	cfg_free(expected_cfg);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(load_cfg__no_file),
		TEST(load_cfg__valid_file),
		TEST(load_cfg__invalid_file),

		TEST(reload_cfg__no_file),
		TEST(reload_cfg__invalid_file),
		TEST(reload_cfg__valid_file),
	};

	return RUN(tests);
}


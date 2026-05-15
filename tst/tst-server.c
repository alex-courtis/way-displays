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

bool __wrap_cfg_resolve_file(struct Cfg *cfg) {
	check_expected_ptr(cfg);

	cfg->dir_path = _dir_path ? strdup(_dir_path) : NULL;
	cfg->file_name = _file_name ? strdup(_file_name) : NULL;
	cfg->file_path = _file_path ? strdup(_file_path) : NULL;

	return mock_type(bool);
}

struct Cfg *__wrap_unmarshal_cfg_from_file(const char *path) {
	check_expected_ptr(path);

	return mock_ptr_type_checked(struct Cfg*);
}

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	logs_clear();

	return 0;
}

int after_each(void **state) {
	free(_dir_path);
	_dir_path = NULL;
	free(_file_name);
	_file_name = NULL;
	free(_file_path);
	_file_path = NULL;

	return 0;
}

void load_cfg__no_file(void **state) {
	expect_any(__wrap_cfg_resolve_file, cfg);
	will_return_int(__wrap_cfg_resolve_file, false);

	struct Cfg *cfg_actual = load_cfg();

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(cfg_actual, cfg_expected);
	assert_nul(cfg_actual->file_name);
	assert_nul(cfg_actual->file_path);
	assert_nul(cfg_actual->dir_path);

	char *log_expected = read_file("tst/server/load-no-file.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	assert_nul(cfg);

	free(log_expected);
	cfg_free(cfg_expected);
	cfg_free(cfg_actual);
}

void load_cfg__invalid_file(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	expect_any(__wrap_cfg_resolve_file, cfg);
	will_return_int(__wrap_cfg_resolve_file, true);

	expect_str(__wrap_unmarshal_cfg_from_file, path, "file_path");
	will_return_ptr_type(__wrap_unmarshal_cfg_from_file, NULL, struct Cfg*);

	struct Cfg *cfg_actual = load_cfg();

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(cfg_actual, cfg_expected);
	assert_str_equal(cfg_actual->file_path, "file_path");
	assert_str_equal(cfg_actual->file_name, "file_name");
	assert_str_equal(cfg_actual->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/load-invalid-file.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	assert_nul(cfg);

	free(log_expected);
	cfg_free(cfg_expected);
	cfg_free(cfg_actual);
}

void load_cfg__valid_file(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	struct Cfg *cfg_read = cfg_default();
	cfg_read->auto_scale_max = 888;
	cfg_read->log_threshold = FATAL;

	expect_any(__wrap_cfg_resolve_file, cfg);
	will_return_int(__wrap_cfg_resolve_file, true);

	expect_str(__wrap_unmarshal_cfg_from_file, path, "file_path");
	will_return_ptr_type(__wrap_unmarshal_cfg_from_file, cfg_read, struct Cfg*);

	struct Cfg *cfg_actual = load_cfg();

	assert_ptr_equal(cfg_actual, cfg_read);

	struct Cfg *cfg_expected = cfg_default();
	cfg_expected->auto_scale_max = 888;
	cfg_expected->log_threshold = FATAL;

	assert_cfg_equal(cfg_actual, cfg_expected);
	assert_str_equal(cfg_actual->file_path, "file_path");
	assert_str_equal(cfg_actual->file_name, "file_name");
	assert_str_equal(cfg_actual->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/load-valid-file.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	assert_nul(cfg);

	free(log_expected);
	cfg_free(cfg_expected);
	cfg_free(cfg_actual);
}

void load_cfg__missing_defaults(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	struct Cfg *cfg_read = cfg_init();
	slist_append(&cfg_read->order_name_desc, strdup("first head"));
	cfg_read->align = BOTTOM;
	cfg_read->auto_scale = OFF;

	expect_any(__wrap_cfg_resolve_file, cfg);
	will_return_int(__wrap_cfg_resolve_file, true);

	expect_str(__wrap_unmarshal_cfg_from_file, path, "file_path");
	will_return_ptr_type(__wrap_unmarshal_cfg_from_file, cfg_read, struct Cfg*);

	struct Cfg *cfg_actual = load_cfg();

	assert_ptr_equal(cfg_actual, cfg_read);

	struct Cfg *cfg_expected = cfg_default();
	slist_append(&cfg_expected->order_name_desc, strdup("first head"));
	cfg_expected->align = BOTTOM;
	cfg_expected->auto_scale = OFF;

	assert_cfg_equal(cfg_actual, cfg_expected);
	assert_str_equal(cfg_actual->file_path, "file_path");
	assert_str_equal(cfg_actual->file_name, "file_name");
	assert_str_equal(cfg_actual->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/load-missing-defaults.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	assert_nul(cfg);

	free(log_expected);
	cfg_free(cfg_expected);
	cfg_free(cfg_actual);
}

void reload_cfg__no_file(void **state) {
	// no mock calls expected

	reload_cfg();

	assert_logs_empty();
}

void reload_cfg__invalid_file(void **state) {
	struct Cfg *cfg_orig = cfg_default();
	cfg = cfg_orig;
	cfg->auto_scale_max = 111;

	cfg->file_path = strdup("file_path");
	cfg->file_name = strdup("file_name");
	cfg->dir_path = strdup("dir_path");

	expect_str(__wrap_unmarshal_cfg_from_file, path, "file_path");
	will_return_ptr_type(__wrap_unmarshal_cfg_from_file, NULL, struct Cfg*);

	reload_cfg();

	assert_ptr_equal(cfg, cfg_orig);

	struct Cfg *cfg_expected = cfg_default();
	cfg_expected->auto_scale_max = 111;

	assert_cfg_equal(cfg, cfg_expected);
	assert_str_equal(cfg->file_path, "file_path");
	assert_str_equal(cfg->file_name, "file_name");
	assert_str_equal(cfg->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/reload-invalid-file.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	free(log_expected);
	cfg_free(cfg_expected);
	cfg_free(cfg_orig);
}

void reload_cfg__valid_file(void **state) {
	struct Cfg *cfg_orig = cfg_default();
	cfg = cfg_orig;
	cfg->auto_scale_max = 222;
	cfg->log_threshold = INFO;

	struct Cfg *cfg_read = cfg_default();
	cfg_read->auto_scale_max = 888;
	cfg_read->log_threshold = FATAL;

	cfg->file_path = strdup("file_path");
	cfg->file_name = strdup("file_name");
	cfg->dir_path = strdup("dir_path");

	expect_str(__wrap_unmarshal_cfg_from_file, path, "file_path");
	will_return_ptr_type(__wrap_unmarshal_cfg_from_file, cfg_read, struct Cfg*);

	expect_int_value(__wrap_log_set_threshold, threshold, FATAL);
	expect_int_value(__wrap_log_set_threshold, cli, false);

	reload_cfg();

	assert_ptr_not_equal(cfg, cfg_orig);
	assert_ptr_equal(cfg, cfg_read);

	struct Cfg *cfg_expected = cfg_default();
	cfg_expected->auto_scale_max = 888;
	cfg_expected->log_threshold = FATAL;

	assert_cfg_equal(cfg, cfg_expected);
	assert_str_equal(cfg->file_path, "file_path");
	assert_str_equal(cfg->file_name, "file_name");
	assert_str_equal(cfg->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/reload-valid-file.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	free(log_expected);
	cfg_free(cfg_expected);
	cfg_free(cfg);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(load_cfg__no_file),
		TEST(load_cfg__valid_file),
		TEST(load_cfg__invalid_file),
		TEST(load_cfg__missing_defaults),

		TEST(reload_cfg__no_file),
		TEST(reload_cfg__invalid_file),
		TEST(reload_cfg__valid_file),
	};

	return RUN(tests);
}


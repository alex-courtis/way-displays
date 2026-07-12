#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "asserts.h"
#include "expects.h"
#include "util-file.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "cfg/file.h"
#include "log.h"
#include "sset.h"

// TODO move to cfg-file

char *_dir_path = NULL;
char *_file_name = NULL;
char *_file_path = NULL;

// cppcheck-suppress staticFunction
bool __wrap_cfg_file_resolve(void) {
	g_cfg_file->dir_path = _dir_path ? strdup(_dir_path) : NULL;
	g_cfg_file->file_name = _file_name ? strdup(_file_name) : NULL;
	g_cfg_file->file_path = _file_path ? strdup(_file_path) : NULL;

	return mock_type(bool);
}

static int before_each(void **state) {
	cfg_destroy();

	cfg_file_init_global();

	return 0;
}

static int after_each(void **state) {
	// assert_logs_empty();

	cfg_destroy();

	cfg_file_destroy_global();

	free(_dir_path);
	_dir_path = NULL;
	free(_file_name);
	_file_name = NULL;
	free(_file_path);
	_file_path = NULL;

	return 0;
}

static void cfg_file_read__no_file(void **state) {
	will_return_int(__wrap_cfg_file_resolve, false);

	cfg_file_read();

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_nul(g_cfg_file->file_name);
	assert_nul(g_cfg_file->file_path);
	assert_nul(g_cfg_file->dir_path);

	char *log_expected = read_file("tst/server/load-no-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);
}

static void cfg_file_read__valid_file(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	struct Cfg *cfg_read = cfg_default();
	cfg_read->auto_scale_max = 888;
	cfg_read->log_threshold = FATAL;
	cfg_read->scale_round_to = 4;

	will_return_int(__wrap_cfg_file_resolve, true);

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, cfg_read, struct Cfg*);

	cfg_file_read();

	assert_ptr_equal(g_cfg, cfg_read);

	struct Cfg *cfg_expected = cfg_default();
	cfg_expected->auto_scale_max = 888;
	cfg_expected->log_threshold = FATAL;
	cfg_expected->scale_round_to = 4;

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_str_equal(g_cfg_file->file_path, "file_path");
	assert_str_equal(g_cfg_file->file_name, "file_name");
	assert_str_equal(g_cfg_file->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/load-valid-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);
}

static void cfg_file_read__invalid_file(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	will_return_int(__wrap_cfg_file_resolve, true);

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, NULL, struct Cfg*);

	cfg_file_read();

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_str_equal(g_cfg_file->file_path, "file_path");
	assert_str_equal(g_cfg_file->file_name, "file_name");
	assert_str_equal(g_cfg_file->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/load-invalid-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);
}

static void cfg_file_read__missing_defaults(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	struct Cfg *cfg_read = cfg_init();
	sset_add(cfg_read->order_name_desc, "first head");
	cfg_read->align = BOTTOM;
	cfg_read->auto_scale = OFF;
	cfg_read->scale_round_to = 2;

	will_return_int(__wrap_cfg_file_resolve, true);

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, cfg_read, struct Cfg*);

	cfg_file_read();

	assert_ptr_equal(g_cfg, cfg_read);

	struct Cfg *cfg_expected = cfg_default();
	sset_add(cfg_expected->order_name_desc, "first head");
	cfg_expected->align = BOTTOM;
	cfg_expected->auto_scale = OFF;
	cfg_expected->scale_round_to = 2;

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_str_equal(g_cfg_file->file_path, "file_path");
	assert_str_equal(g_cfg_file->file_name, "file_name");
	assert_str_equal(g_cfg_file->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/load-missing-defaults.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);
}

static void cfg_file_reload__no_file(void **state) {
	struct Cfg *cfg_orig = cfg_default();
	g_cfg = cfg_orig;

	// no mock calls expected

	cfg_file_reload();

	assert_ptr_equal(g_cfg, cfg_orig);
}

static void cfg_file_reload__invalid_file(void **state) {
	struct Cfg *cfg_orig = cfg_default();
	g_cfg = cfg_orig;
	g_cfg->auto_scale_max = 111;

	g_cfg_file->file_path = strdup("file_path");
	g_cfg_file->file_name = strdup("file_name");
	g_cfg_file->dir_path = strdup("dir_path");

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, NULL, struct Cfg*);

	cfg_file_reload();

	assert_ptr_equal(g_cfg, cfg_orig);

	struct Cfg *cfg_expected = cfg_default();
	cfg_expected->auto_scale_max = 111;

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_str_equal(g_cfg_file->file_path, "file_path");
	assert_str_equal(g_cfg_file->file_name, "file_name");
	assert_str_equal(g_cfg_file->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/reload-invalid-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);
}

static void cfg_file_reload__valid_file(void **state) {
	struct Cfg *cfg_orig = cfg_default();
	g_cfg = cfg_orig;
	g_cfg->auto_scale_max = 222;
	g_cfg->log_threshold = INFO;

	struct Cfg *cfg_read = cfg_default();
	cfg_read->auto_scale_max = 888;
	cfg_read->log_threshold = FATAL;

	g_cfg_file->file_path = strdup("file_path");
	g_cfg_file->file_name = strdup("file_name");
	g_cfg_file->dir_path = strdup("dir_path");

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, cfg_read, struct Cfg*);

	expect_int_value(__wrap_log_set_threshold, threshold, FATAL);
	expect_int_value(__wrap_log_set_threshold, cli, false);

	cfg_file_reload();

	assert_ptr_not_equal(g_cfg, cfg_orig);
	assert_ptr_equal(g_cfg, cfg_read);

	struct Cfg *cfg_expected = cfg_default();
	cfg_expected->auto_scale_max = 888;
	cfg_expected->log_threshold = FATAL;

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_str_equal(g_cfg_file->file_path, "file_path");
	assert_str_equal(g_cfg_file->file_name, "file_name");
	assert_str_equal(g_cfg_file->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/reload-valid-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(cfg_file_read__no_file),
		TEST_BA(cfg_file_read__valid_file),
		TEST_BA(cfg_file_read__invalid_file),
		TEST_BA(cfg_file_read__missing_defaults),

		TEST_BA(cfg_file_reload__no_file),
		TEST_BA(cfg_file_reload__invalid_file),
		TEST_BA(cfg_file_reload__valid_file),
	};

	return RUN(tests);
}


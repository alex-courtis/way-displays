#include "tst.h"
#include "asserts.h"
#include "expects.h"
#include "util.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "slist.h"
#include "log.h"

#include "server.h"
#include "cfg.h"
#include "yaml/unmarshal.h"


void load_cfg(void);
void reload_cfg(void);


char *_dir_path = NULL;
char *_file_name = NULL;
char *_file_path = NULL;

// cppcheck-suppress staticFunction
bool __wrap_cfg_resolve_file_path(struct Cfg *cfg) {
	check_expected_ptr(cfg);

	cfg->dir_path = _dir_path ? strdup(_dir_path) : NULL;
	cfg->file_name = _file_name ? strdup(_file_name) : NULL;
	cfg->file_path = _file_path ? strdup(_file_path) : NULL;

	return mock_type(bool);
}

// cppcheck-suppress staticFunction
void *__wrap_yaml_unmarshal_file(const char *path, yaml_root_to_type_fn fn) {
	check_expected_ptr(path);

	return mock_ptr_type_checked(struct Cfg*);
}

static int before_all(void **state) {
	return 0;
}

static int after_all(void **state) {
	return 0;
}

static int before_each(void **state) {
	cfg_destroy();

	logs_clear();

	return 0;
}

static int after_each(void **state) {
	cfg_destroy();

	free(_dir_path);
	_dir_path = NULL;
	free(_file_name);
	_file_name = NULL;
	free(_file_path);
	_file_path = NULL;

	return 0;
}

static void load_cfg__no_file(void **state) {
	expect_any(__wrap_cfg_resolve_file_path, cfg);
	will_return_int(__wrap_cfg_resolve_file_path, false);

	load_cfg();

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_nul(g_cfg->file_name);
	assert_nul(g_cfg->file_path);
	assert_nul(g_cfg->dir_path);

	char *log_expected = read_file("tst/server/load-no-file.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	free(log_expected);
	cfg_free(cfg_expected);
}

static void load_cfg__valid_file(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	struct Cfg *cfg_read = cfg_default();
	cfg_read->auto_scale_max = 888;
	cfg_read->log_threshold = FATAL;
	cfg_read->scale_round_to = 4;

	expect_any(__wrap_cfg_resolve_file_path, cfg);
	will_return_int(__wrap_cfg_resolve_file_path, true);

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, cfg_read, struct Cfg*);

	load_cfg();

	assert_ptr_equal(g_cfg, cfg_read);

	struct Cfg *cfg_expected = cfg_default();
	cfg_expected->auto_scale_max = 888;
	cfg_expected->log_threshold = FATAL;
	cfg_expected->scale_round_to = 4;

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_str_equal(g_cfg->file_path, "file_path");
	assert_str_equal(g_cfg->file_name, "file_name");
	assert_str_equal(g_cfg->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/load-valid-file.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	free(log_expected);
	cfg_free(cfg_expected);
}

static void load_cfg__invalid_file(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	expect_any(__wrap_cfg_resolve_file_path, cfg);
	will_return_int(__wrap_cfg_resolve_file_path, true);

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, NULL, struct Cfg*);

	load_cfg();

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_str_equal(g_cfg->file_path, "file_path");
	assert_str_equal(g_cfg->file_name, "file_name");
	assert_str_equal(g_cfg->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/load-invalid-file.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	free(log_expected);
	cfg_free(cfg_expected);
}

static void load_cfg__missing_defaults(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	struct Cfg *cfg_read = cfg_init();
	slist_append(&cfg_read->order_name_desc, strdup("first head"));
	cfg_read->align = BOTTOM;
	cfg_read->auto_scale = OFF;
	cfg_read->scale_round_to = 2;

	expect_any(__wrap_cfg_resolve_file_path, cfg);
	will_return_int(__wrap_cfg_resolve_file_path, true);

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, cfg_read, struct Cfg*);

	load_cfg();

	assert_ptr_equal(g_cfg, cfg_read);

	struct Cfg *cfg_expected = cfg_default();
	slist_append(&cfg_expected->order_name_desc, strdup("first head"));
	cfg_expected->align = BOTTOM;
	cfg_expected->auto_scale = OFF;
	cfg_expected->scale_round_to = 2;

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_str_equal(g_cfg->file_path, "file_path");
	assert_str_equal(g_cfg->file_name, "file_name");
	assert_str_equal(g_cfg->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/load-missing-defaults.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	free(log_expected);
	cfg_free(cfg_expected);
}

static void reload_cfg__no_file(void **state) {
	struct Cfg *cfg_orig = cfg_default();
	g_cfg = cfg_orig;

	// no mock calls expected

	reload_cfg();

	assert_ptr_equal(g_cfg, cfg_orig);

	assert_logs_empty();
}

static void reload_cfg__invalid_file(void **state) {
	struct Cfg *cfg_orig = cfg_default();
	g_cfg = cfg_orig;
	g_cfg->auto_scale_max = 111;

	g_cfg->file_path = strdup("file_path");
	g_cfg->file_name = strdup("file_name");
	g_cfg->dir_path = strdup("dir_path");

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, NULL, struct Cfg*);

	reload_cfg();

	assert_ptr_equal(g_cfg, cfg_orig);

	struct Cfg *cfg_expected = cfg_default();
	cfg_expected->auto_scale_max = 111;

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_str_equal(g_cfg->file_path, "file_path");
	assert_str_equal(g_cfg->file_name, "file_name");
	assert_str_equal(g_cfg->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/reload-invalid-file.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	free(log_expected);
	cfg_free(cfg_expected);
}

static void reload_cfg__valid_file(void **state) {
	struct Cfg *cfg_orig = cfg_default();
	g_cfg = cfg_orig;
	g_cfg->auto_scale_max = 222;
	g_cfg->log_threshold = INFO;

	struct Cfg *cfg_read = cfg_default();
	cfg_read->auto_scale_max = 888;
	cfg_read->log_threshold = FATAL;

	g_cfg->file_path = strdup("file_path");
	g_cfg->file_name = strdup("file_name");
	g_cfg->dir_path = strdup("dir_path");

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, cfg_read, struct Cfg*);

	expect_int_value(__wrap_log_set_threshold, threshold, FATAL);
	expect_int_value(__wrap_log_set_threshold, cli, false);

	reload_cfg();

	assert_ptr_not_equal(g_cfg, cfg_orig);
	assert_ptr_equal(g_cfg, cfg_read);

	struct Cfg *cfg_expected = cfg_default();
	cfg_expected->auto_scale_max = 888;
	cfg_expected->log_threshold = FATAL;

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_str_equal(g_cfg->file_path, "file_path");
	assert_str_equal(g_cfg->file_name, "file_name");
	assert_str_equal(g_cfg->dir_path, "dir_path");

	char *log_expected = read_file("tst/server/reload-valid-file.log");
	assert_log(INFO, log_expected);
	assert_logs_empty();

	free(log_expected);
	cfg_free(cfg_expected);
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


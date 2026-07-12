#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "asserts.h"
#include "expects.h"
#include "util-file.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cfg.h"
#include "log.h"
#include "pslist.h"
#include "sset.h"

#include "cfg/file.h"

extern struct Pslist *g_cfg_file_paths;

char *env_xdg_config_home = NULL;
char *env_home = NULL;

char *_dir_path = NULL;
char *_file_name = NULL;
char *_file_path = NULL;

// cppcheck-suppress staticFunction
bool __wrap_g_cfg_file_resolve(void) {
	g_cfg_file->dir_path = _dir_path ? strdup(_dir_path) : NULL;
	g_cfg_file->file_name = _file_name ? strdup(_file_name) : NULL;
	g_cfg_file->file_path = _file_path ? strdup(_file_path) : NULL;

	return mock_type(bool);
}

static void clean_files(void) {
	remove("tst/tmp/write-existing-cfg.yaml");
}

static int before_all(void **state) {
	env_xdg_config_home = getenv("XDG_CONFIG_HOME");
	if (env_xdg_config_home) {
		env_xdg_config_home = strdup(env_xdg_config_home);
	}

	mkdir("tst/tmp", 0755);

	env_home = getenv("HOME");
	if (env_home) {
		env_home = strdup(env_home);
	}

	return 0;
}

static int after_all(void **state) {
	free(env_xdg_config_home);
	free(env_home);

	rmdir("tst/tmp");

	return 0;
}

static int before_each(void **state) {
	pslist_free_vals(&g_cfg_file_paths, NULL);

	clean_files();

	g_cfg_file_init();

	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	if (env_xdg_config_home) {
		setenv("XDG_CONFIG_HOME", env_xdg_config_home, 1);
	} else {
		unsetenv("XDG_CONFIG_HOME");
	}

	if (env_home) {
		setenv("HOME", env_home, 1);
	} else {
		unsetenv("HOME");
	}

	pslist_free_vals(&g_cfg_file_paths, NULL);

	g_cfg_file_destroy();

	// clean_files();

	g_cfg_destroy();

	free(_dir_path);
	_dir_path = NULL;
	free(_file_name);
	_file_name = NULL;
	free(_file_path);
	_file_path = NULL;

	return 0;
}


static void g_cfg_file_write__bad_yaml(void **state) {
	free(g_cfg_file->file_path);
	g_cfg_file->file_path = strdup("something");

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, NULL, char*);

	g_cfg_file_write();
}

static void g_cfg_file_write__none(void **state) {
	pslist_append(&g_cfg_file_paths, strdup("/path/to/zero"));

	char *expected = strdup("XXXX");

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, expected, char*);

	expect_function_call(__wrap_fd_wd_cfg_dir_destroy);

	expect_str(__wrap_mkdir_p, path, "/path/to");
	expect_int_value(__wrap_mkdir_p, mode, 0755);
	will_return_int(__wrap_mkdir_p, true);

	expect_str(__wrap_file_write, path, "/path/to/zero");
	expect_str(__wrap_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_file_write, mode, "w");
	will_return_int(__wrap_file_write, true);

	expect_str(__wrap_file_write, path, "/path/to/zero");
	expect_str(__wrap_file_write, contents, expected);
	expect_str(__wrap_file_write, mode, "a");
	will_return_int(__wrap_file_write, true);

	expect_function_call(__wrap_fd_wd_cfg_dir_create);

	g_cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: /path/to/zero\n");

	assert_str_equal(g_cfg_file->file_path, "/path/to/zero");
	assert_str_equal(g_cfg_file->dir_path, "/path/to");
	assert_str_equal(g_cfg_file->file_name, "zero");
	assert_str_equal(g_cfg_file->resolved_path, "/path/to/zero");
	assert_ptr_equal(g_cfg_file->resolved_path, pslist_at(g_cfg_file_paths, 0));
	assert_int_equal(g_cfg_file->modified, false);
}

static void g_cfg_file_write__cannot_write_use_alternative(void **state) {
	pslist_append(&g_cfg_file_paths, strdup("/path/to/zero"));
	pslist_append(&g_cfg_file_paths, strdup("/path/to/one"));
	pslist_append(&g_cfg_file_paths, strdup("/path/to/two"));
	pslist_append(&g_cfg_file_paths, strdup("/path/to/three"));
	pslist_append(&g_cfg_file_paths, strdup("/path/to/four"));

	g_cfg_file->file_path = strdup("/path/to/two");
	g_cfg_file->dir_path = strdup("nothing");
	g_cfg_file->file_name = strdup("missing");
	g_cfg_file->resolved_path = pslist_at(g_cfg_file_paths, 2);

	char *expected = strdup("XXXXxxxX");

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, strdup(expected), char*);

	expect_function_call(__wrap_fd_wd_cfg_dir_destroy);

	expect_str(__wrap_file_write, path, "/path/to/two");
	expect_str(__wrap_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_file_write, mode, "w");
	will_return_int(__wrap_file_write, false);

	expect_str(__wrap_mkdir_p, path, "/path/to");
	expect_int_value(__wrap_mkdir_p, mode, 0755);
	will_return_int(__wrap_mkdir_p, true);

	expect_str(__wrap_file_write, path, "/path/to/zero");
	expect_str(__wrap_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_file_write, mode, "w");
	will_return_int(__wrap_file_write, false);

	expect_str(__wrap_mkdir_p, path, "/path/to");
	expect_int_value(__wrap_mkdir_p, mode, 0755);
	will_return_int(__wrap_mkdir_p, false);

	expect_str(__wrap_mkdir_p, path, "/path/to");
	expect_int_value(__wrap_mkdir_p, mode, 0755);
	will_return_int(__wrap_mkdir_p, true);

	expect_str(__wrap_file_write, path, "/path/to/three");
	expect_str(__wrap_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_file_write, mode, "w");
	will_return_int(__wrap_file_write, true);

	expect_str(__wrap_file_write, path, "/path/to/three");
	expect_str(__wrap_file_write, contents, expected);
	expect_str(__wrap_file_write, mode, "a");
	will_return_int(__wrap_file_write, true);

	expect_function_call(__wrap_fd_wd_cfg_dir_create);

	g_cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: /path/to/three\n");

	assert_str_equal(g_cfg_file->file_path, "/path/to/three");
	assert_str_equal(g_cfg_file->dir_path, "/path/to");
	assert_str_equal(g_cfg_file->file_name, "three");
	assert_str_equal(g_cfg_file->resolved_path, "/path/to/three");
	assert_ptr_equal(g_cfg_file->resolved_path, pslist_at(g_cfg_file_paths, 3));
	assert_int_equal(g_cfg_file->modified, false);

	free(expected);
}

static void g_cfg_file_write__cannot_write_no_alternative(void **state) {
	pslist_append(&g_cfg_file_paths, strdup("/path/to/zero"));
	pslist_append(&g_cfg_file_paths, strdup("/path/to/one"));

	g_cfg_file->file_path = strdup("/path/to/zero");
	g_cfg_file->dir_path = strdup("/path/to");
	g_cfg_file->file_name = strdup("one");
	g_cfg_file->resolved_path = pslist_at(g_cfg_file_paths, 0);

	char *expected = strdup("XXXX");

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, strdup(expected), char*);

	expect_function_call(__wrap_fd_wd_cfg_dir_destroy);

	expect_str(__wrap_file_write, path, "/path/to/zero");
	expect_str(__wrap_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_file_write, mode, "w");
	will_return_int(__wrap_file_write, false);

	expect_str(__wrap_mkdir_p, path, "/path/to");
	expect_int_value(__wrap_mkdir_p, mode, 0755);
	will_return_int(__wrap_mkdir_p, true);

	expect_str(__wrap_file_write, path, "/path/to/one");
	expect_str(__wrap_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_file_write, mode, "w");
	will_return_int(__wrap_file_write, false);

	g_cfg_file_write();

	assert_nul(g_cfg_file->file_path);
	assert_nul(g_cfg_file->dir_path);
	assert_nul(g_cfg_file->file_name);
	assert_nul(g_cfg_file->resolved_path);
	assert_int_equal(g_cfg_file->modified, false);

	free(expected);
}

static void g_cfg_file_write__existing(void **state) {
	g_cfg_file->file_path = strdup("tst/tmp/write-existing-cfg.yaml");

	FILE *f = fopen(g_cfg_file->file_path, "w");
	assert_non_nul(f);
	if (f) {
		fclose(f);
	}

	char *expected = strdup("XXXX");

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, strdup(expected), char*);

	expect_str(__wrap_file_write, path, g_cfg_file->file_path);
	expect_str(__wrap_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_file_write, mode, "w");
	will_return_int(__wrap_file_write, true);

	expect_str(__wrap_file_write, path, g_cfg_file->file_path);
	expect_str(__wrap_file_write, contents, expected);
	expect_str(__wrap_file_write, mode, "a");
	will_return_int(__wrap_file_write, true);

	g_cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: tst/tmp/write-existing-cfg.yaml\n");

	assert_int_equal(g_cfg_file->modified, true);

	free(expected);
}

static void g_cfg_file_paths_init__min(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	unsetenv("HOME");

	g_cfg_file_paths_init("inexistent");

	assert_str_equal(pslist_at(g_cfg_file_paths, 0), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_cfg_file_paths, 1), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(pslist_length(g_cfg_file_paths), 2);
}

static void g_cfg_file_paths_init__xch(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	g_cfg_file_paths_init(NULL);

	assert_str_equal(pslist_at(g_cfg_file_paths, 0), "xch/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_cfg_file_paths, 1), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_cfg_file_paths, 2), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(pslist_length(g_cfg_file_paths), 3);
}

static void g_cfg_file_paths_init__home(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	setenv("HOME", "hom", 1);

	g_cfg_file_paths_init(NULL);

	assert_str_equal(pslist_at(g_cfg_file_paths, 0), "hom/.config/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_cfg_file_paths, 1), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_cfg_file_paths, 2), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(pslist_length(g_cfg_file_paths), 3);
}

static void g_cfg_file_paths_init__user(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	g_cfg_file_paths_init(".");

	assert_str_equal(pslist_at(g_cfg_file_paths, 0), ".");

	assert_str_equal(pslist_at(g_cfg_file_paths, 1), "xch/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_cfg_file_paths, 2), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_cfg_file_paths, 3), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(pslist_length(g_cfg_file_paths), 4);
}

static void g_cfg_file_read__no_file(void **state) {
	will_return_int(__wrap_g_cfg_file_resolve, false);

	g_cfg_file_read();

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

static void g_cfg_file_read__valid_file(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	struct Cfg *cfg_read = cfg_default();
	cfg_read->auto_scale_max = 888;
	cfg_read->log_threshold = FATAL;
	cfg_read->scale_round_to = 4;

	will_return_int(__wrap_g_cfg_file_resolve, true);

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, cfg_read, struct Cfg*);

	g_cfg_file_read();

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

static void g_cfg_file_read__invalid_file(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	will_return_int(__wrap_g_cfg_file_resolve, true);

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, NULL, struct Cfg*);

	g_cfg_file_read();

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

static void g_cfg_file_read__missing_defaults(void **state) {
	_file_path = strdup("file_path");
	_file_name = strdup("file_name");
	_dir_path = strdup("dir_path");

	struct Cfg *cfg_read = cfg_init();
	sset_add(cfg_read->order_name_desc, "first head");
	cfg_read->align = BOTTOM;
	cfg_read->auto_scale = OFF;
	cfg_read->scale_round_to = 2;

	will_return_int(__wrap_g_cfg_file_resolve, true);

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, cfg_read, struct Cfg*);

	g_cfg_file_read();

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

static void g_cfg_file_reload__no_file(void **state) {
	struct Cfg *cfg_orig = cfg_default();
	g_cfg = cfg_orig;

	// no mock calls expected

	g_cfg_file_reload();

	assert_ptr_equal(g_cfg, cfg_orig);
}

static void g_cfg_file_reload__invalid_file(void **state) {
	struct Cfg *cfg_orig = cfg_default();
	g_cfg = cfg_orig;
	g_cfg->auto_scale_max = 111;

	g_cfg_file->file_path = strdup("file_path");
	g_cfg_file->file_name = strdup("file_name");
	g_cfg_file->dir_path = strdup("dir_path");

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, NULL, struct Cfg*);

	g_cfg_file_reload();

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

static void g_cfg_file_reload__valid_file(void **state) {
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

	g_cfg_file_reload();

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
		TEST_BA(g_cfg_file_write__bad_yaml),
		TEST_BA(g_cfg_file_write__none),
		TEST_BA(g_cfg_file_write__cannot_write_use_alternative),
		TEST_BA(g_cfg_file_write__cannot_write_no_alternative),
		TEST_BA(g_cfg_file_write__existing),

		TEST_BA(g_cfg_file_paths_init__min),
		TEST_BA(g_cfg_file_paths_init__home),
		TEST_BA(g_cfg_file_paths_init__xch),
		TEST_BA(g_cfg_file_paths_init__user),

		TEST_BA(g_cfg_file_read__no_file),
		TEST_BA(g_cfg_file_read__valid_file),
		TEST_BA(g_cfg_file_read__invalid_file),
		TEST_BA(g_cfg_file_read__missing_defaults),

		TEST_BA(g_cfg_file_reload__no_file),
		TEST_BA(g_cfg_file_reload__invalid_file),
		TEST_BA(g_cfg_file_reload__valid_file),
	};

	return RUN_BA(tests);
}


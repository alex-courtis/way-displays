#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "asserts.h"
#include "expects.h"
#include "util-file.h"

#include <cmocka.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg/cfg.h"
#include "enum.h"
#include "pslist.h"
#include "sset.h"

#include "cfg/file.h"

void candidates_init(const char *user_path);

extern struct Pslist *g_candidates;

char *env_xdg_config_home = NULL;
char *env_home = NULL;

static int before_all(void **state) {
	env_xdg_config_home = getenv("XDG_CONFIG_HOME");
	if (env_xdg_config_home) {
		env_xdg_config_home = strdup(env_xdg_config_home);
	}

	env_home = getenv("HOME");
	if (env_home) {
		env_home = strdup(env_home);
	}

	return 0;
}

static int after_all(void **state) {
	free(env_xdg_config_home);
	free(env_home);

	return 0;
}

static int before_each(void **state) {
	pslist_free_vals(&g_candidates, NULL);

	memset(&g_cfg_file, 0, sizeof(struct CfgFile));

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

	pslist_free_vals(&g_candidates, NULL);

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
}

static void g_cfg_file_write__none(void **state) {
	pslist_append(&g_candidates, strdup("/path/zero/0"));

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
	assert_ptr_equal(g_cfg_file.file_path_resolved, pslist_at(g_candidates, 0));
	assert_false(g_cfg_file.written);
}

static void g_cfg_file_write__cannot_write_use_alternative(void **state) {
	pslist_append(&g_candidates, strdup("/path/zero/0"));
	pslist_append(&g_candidates, strdup("/path/one/1"));
	pslist_append(&g_candidates, strdup("/path/two/2"));
	pslist_append(&g_candidates, strdup("/path/three/3"));
	pslist_append(&g_candidates, strdup("/path/four/4"));

	strncpy(g_cfg_file.file_path, "/path/two/2", PATH_MAX - 1);
	strncpy(g_cfg_file.dir_path, "nothing", PATH_MAX - 1);
	strncpy(g_cfg_file.file_name, "missing", PATH_MAX - 1);
	g_cfg_file.file_path_resolved = pslist_at(g_candidates, 2);

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
	assert_ptr_equal(g_cfg_file.file_path_resolved, pslist_at(g_candidates, 3));
	assert_false(g_cfg_file.written);

	free(expected);
}

static void g_cfg_file_write__cannot_write_no_alternative(void **state) {
	pslist_append(&g_candidates, strdup("/path/zero/0"));
	pslist_append(&g_candidates, strdup("/path/one/1"));

	strncpy(g_cfg_file.file_path, "/path/zero/0", PATH_MAX - 1);
	strncpy(g_cfg_file.dir_path, "/path/zero", PATH_MAX - 1);
	strncpy(g_cfg_file.file_name, "one", PATH_MAX - 1);
	g_cfg_file.file_path_resolved = pslist_at(g_candidates, 0);

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
	assert_nul(g_cfg_file.file_path_resolved);
	assert_false(g_cfg_file.written);

	assert_log(ERROR,
			"\nUnable to write to /path/zero/0\n"
			"\nUnable to write to /path/one/1\n"
			);

	free(expected);
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
}

static void candidates_init__min(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	unsetenv("HOME");

	candidates_init("inexistent");

	assert_str_equal(pslist_at(g_candidates, 0), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_candidates, 1), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(pslist_length(g_candidates), 2);
}

static void candidates_init__xch(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	candidates_init(NULL);

	assert_str_equal(pslist_at(g_candidates, 0), "xch/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_candidates, 1), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_candidates, 2), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(pslist_length(g_candidates), 3);
}

static void candidates_init__home(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	setenv("HOME", "hom", 1);

	candidates_init(NULL);

	assert_str_equal(pslist_at(g_candidates, 0), "hom/.config/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_candidates, 1), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_candidates, 2), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(pslist_length(g_candidates), 3);
}

static void candidates_init__user(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	candidates_init(".");

	assert_str_equal(pslist_at(g_candidates, 0), ".");

	assert_str_equal(pslist_at(g_candidates, 1), "xch/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_candidates, 2), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(pslist_at(g_candidates, 3), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(pslist_length(g_candidates), 4);
}

static void g_cfg_file_init_read__no_file(void **state) {
	// dummy to stop known path reading
	pslist_append(&g_candidates, NULL);

	g_cfg_file_init_read(NULL);

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(g_cfg, cfg_expected);

	char *log_expected = read_file("tst/server/load-no-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);
}

static void g_cfg_file_init_read__file_not_resolved(void **state) {
	pslist_append(&g_candidates, strdup("known-path"));

	expect_str(__wrap_fs_canonical_path, path, "known-path");
	will_return_ptr_type(__wrap_fs_canonical_path, NULL, char*);

	g_cfg_file_init_read(NULL);

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(g_cfg, cfg_expected);

	char *log_expected = read_file("tst/server/load-no-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);
}

static void g_cfg_file_init_read__valid_file(void **state) {
	pslist_append(&g_candidates, strdup("known-path"));

	struct Cfg *cfg_read = cfg_default();
	cfg_read->auto_scale_max = 888;
	cfg_read->log_threshold = FATAL;
	cfg_read->scale_round_to = 4;

	expect_str(__wrap_fs_canonical_path, path, "known-path");
	will_return_ptr_type(__wrap_fs_canonical_path, strdup("canonical-path"), char*);

	expect_str(__wrap_yaml_unmarshal_file, path, "canonical-path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, cfg_read, struct Cfg*);

	g_cfg_file_init_read(NULL);

	assert_ptr_equal(g_cfg, cfg_read);

	struct Cfg *cfg_expected = cfg_default();
	cfg_expected->auto_scale_max = 888;
	cfg_expected->log_threshold = FATAL;
	cfg_expected->scale_round_to = 4;

	assert_cfg_equal(g_cfg, cfg_expected);

	char *log_expected = read_file("tst/server/load-valid-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);
}

static void g_cfg_file_init_read__invalid_file(void **state) {
	pslist_append(&g_candidates, strdup("known-path"));

	expect_str(__wrap_fs_canonical_path, path, "known-path");
	will_return_ptr_type(__wrap_fs_canonical_path, strdup("invalid-cfg.yaml"), char*);

	expect_str(__wrap_yaml_unmarshal_file, path, "invalid-cfg.yaml");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, NULL, struct Cfg*);

	g_cfg_file_init_read(NULL);

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(g_cfg, cfg_expected);

	char *log_expected = read_file("tst/server/load-invalid-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);
}

static void g_cfg_file_init_read__missing_defaults(void **state) {
	pslist_append(&g_candidates, strdup("known-path"));

	expect_str(__wrap_fs_canonical_path, path, "known-path");
	will_return_ptr_type(__wrap_fs_canonical_path, strdup("file_path"), char*);

	struct Cfg *cfg_read = cfg_init();
	sset_add(cfg_read->order_name_desc, "first head");
	cfg_read->align = BOTTOM;
	cfg_read->auto_scale = OFF;
	cfg_read->scale_round_to = 2;

	strncpy(g_cfg_file.file_path, "file_path", PATH_MAX - 1);

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, cfg_read, struct Cfg*);

	g_cfg_file_init_read(NULL);

	assert_ptr_equal(g_cfg, cfg_read);

	struct Cfg *cfg_expected = cfg_default();
	sset_add(cfg_expected->order_name_desc, "first head");
	cfg_expected->align = BOTTOM;
	cfg_expected->auto_scale = OFF;
	cfg_expected->scale_round_to = 2;

	assert_cfg_equal(g_cfg, cfg_expected);

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

	strncpy(g_cfg_file.file_path, "file_path", PATH_MAX - 1);
	strncpy(g_cfg_file.file_name, "file_name", PATH_MAX - 1);
	strncpy(g_cfg_file.dir_path, "dir_path", PATH_MAX - 1);

	expect_str(__wrap_yaml_unmarshal_file, path, "file_path");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, NULL, struct Cfg*);

	g_cfg_file_reload();

	assert_ptr_equal(g_cfg, cfg_orig);

	struct Cfg *cfg_expected = cfg_default();
	cfg_expected->auto_scale_max = 111;

	assert_cfg_equal(g_cfg, cfg_expected);
	assert_str_equal(g_cfg_file.file_path, "file_path");
	assert_str_equal(g_cfg_file.file_name, "file_name");
	assert_str_equal(g_cfg_file.dir_path, "dir_path");

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

	strncpy(g_cfg_file.file_path, "file_path", PATH_MAX - 1);
	strncpy(g_cfg_file.file_name, "file_name", PATH_MAX - 1);
	strncpy(g_cfg_file.dir_path, "dir_path", PATH_MAX - 1);

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
	assert_str_equal(g_cfg_file.file_path, "file_path");
	assert_str_equal(g_cfg_file.file_name, "file_name");
	assert_str_equal(g_cfg_file.dir_path, "dir_path");

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

		TEST_BA(candidates_init__min),
		TEST_BA(candidates_init__home),
		TEST_BA(candidates_init__xch),
		TEST_BA(candidates_init__user),

		TEST_BA(g_cfg_file_init_read__no_file),
		TEST_BA(g_cfg_file_init_read__file_not_resolved),
		TEST_BA(g_cfg_file_init_read__valid_file),
		TEST_BA(g_cfg_file_init_read__invalid_file),
		TEST_BA(g_cfg_file_init_read__missing_defaults),

		TEST_BA(g_cfg_file_reload__no_file),
		TEST_BA(g_cfg_file_reload__invalid_file),
		TEST_BA(g_cfg_file_reload__valid_file),
	};

	return RUN_BA(tests);
}


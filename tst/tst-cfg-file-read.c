#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "assert-sset.h"
#include "asserts.h"
#include "expects.h"
#include "util-col.h"
#include "util-file.h"
#include "util-init.h"

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
	g_candidates = sset_init();

	memset(&g_cfg_file, 0, sizeof(struct CfgFile));

	return 0;
}

static int after_each(void **state) {
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

	sset_free(g_candidates);
	g_candidates = NULL;

	memset(&g_cfg_file, 0, sizeof(struct CfgFile));

	g_cfg_destroy();

	return 0;
}

static void g_candidates_init__min(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	unsetenv("HOME");

	candidates_init("inexistent");

	const struct Sset *expected = sset_init();
	sset_add_many(expected,
			"/usr/local/etc/way-displays/cfg.yaml",
			ROOT_ETC"/way-displays/cfg.yaml",
			NULL);

	assert_sset_equal(g_candidates, expected);

	sset_free(expected);

	assert_logs_empty();
}

static void g_candidates_init__xch(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	candidates_init(NULL);

	const struct Sset *expected = sset_init();
	sset_add_many(expected,
			"xch/way-displays/cfg.yaml",
			"/usr/local/etc/way-displays/cfg.yaml",
			ROOT_ETC"/way-displays/cfg.yaml",
			NULL);

	assert_sset_equal(g_candidates, expected);

	sset_free(expected);

	assert_logs_empty();
}

static void g_candidates_init__home(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	setenv("HOME", "hom", 1);

	candidates_init(NULL);

	const struct Sset *expected = sset_init();
	sset_add_many(expected,
			"hom/.config/way-displays/cfg.yaml",
			"/usr/local/etc/way-displays/cfg.yaml",
			ROOT_ETC"/way-displays/cfg.yaml",
			NULL);

	assert_sset_equal(g_candidates, expected);

	sset_free(expected);

	assert_logs_empty();
}

static void g_candidates_init__user(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	candidates_init(".");

	const struct Sset *expected = sset_init();
	sset_add_many(expected,
			".",
			"xch/way-displays/cfg.yaml",
			"/usr/local/etc/way-displays/cfg.yaml",
			ROOT_ETC"/way-displays/cfg.yaml",
			NULL);

	assert_sset_equal(g_candidates, expected);

	sset_free(expected);

	assert_logs_empty();
}

static void g_cfg_file_init_read__no_file(void **state) {
	g_cfg_file_init_read(NULL);

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(g_cfg, cfg_expected);

	char *log_expected = read_file("tst/cfg-file/load-no-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);

	assert_logs_empty();
}

static void g_cfg_file_init_read__file_not_resolved(void **state) {
	sset_add(g_candidates, "known-path");

	expect_str(__wrap_fs_canonical_path, path, "known-path");
	will_return_ptr_type(__wrap_fs_canonical_path, NULL, char*);

	g_cfg_file_init_read(NULL);

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(g_cfg, cfg_expected);

	char *log_expected = read_file("tst/cfg-file/load-no-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);

	assert_logs_empty();
}

static void g_cfg_file_init_read__valid_file(void **state) {
	sset_add(g_candidates, "known-path");

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

	char *log_expected = read_file("tst/cfg-file/load-valid-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);

	assert_logs_empty();
}

static void g_cfg_file_init_read__invalid_file(void **state) {
	sset_add(g_candidates, "known-path");

	expect_str(__wrap_fs_canonical_path, path, "known-path");
	will_return_ptr_type(__wrap_fs_canonical_path, strdup("invalid-cfg.yaml"), char*);

	expect_str(__wrap_yaml_unmarshal_file, path, "invalid-cfg.yaml");
	will_return_ptr_type(__wrap_yaml_unmarshal_file, NULL, struct Cfg*);

	g_cfg_file_init_read(NULL);

	struct Cfg *cfg_expected = cfg_default();

	assert_cfg_equal(g_cfg, cfg_expected);

	char *log_expected = read_file("tst/cfg-file/load-invalid-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);

	assert_logs_empty();
}

static void g_cfg_file_init_read__missing_defaults(void **state) {
	sset_add(g_candidates, "known-path");

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

	char *log_expected = read_file("tst/cfg-file/load-missing-defaults.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);

	assert_logs_empty();
}

static void g_cfg_file_reload__no_file(void **state) {
	struct Cfg *cfg_orig = cfg_default();
	g_cfg = cfg_orig;

	// no mock calls expected

	g_cfg_file_reload();

	assert_ptr_equal(g_cfg, cfg_orig);

	assert_logs_empty();
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

	char *log_expected = read_file("tst/cfg-file/reload-invalid-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);

	assert_logs_empty();
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

	char *log_expected = read_file("tst/cfg-file/reload-valid-file.log");
	assert_log(INFO, log_expected);

	free(log_expected);
	cfg_free(cfg_expected);

	assert_logs_empty();
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(g_candidates_init__min),
		TEST_BA(g_candidates_init__home),
		TEST_BA(g_candidates_init__xch),
		TEST_BA(g_candidates_init__user),

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


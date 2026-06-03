#include "tst.h"

#include "asserts-log.h"
#include "asserts.h"
#include "expects.h"
#include "wrap-log.h"

#include <cmocka.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "slist.h"
#include "yaml/marshal.h"

#include "cfg.h"

extern struct SList *cfg_file_paths;

char *env_xdg_config_home = NULL;
char *env_home = NULL;

// cppcheck-suppress staticFunction
char *__wrap_yaml_marshal(const void *data, yaml_doc_fn fn, const char *human) {
	check_expected_ptr(data);
	check_expected_ptr(human);

	return mock_ptr_type_checked(char*);
}

// cppcheck-suppress staticFunction
bool __wrap_file_write(const char *path, const char *contents, const char *mode) {
	check_expected_ptr(path);
	check_expected_ptr(contents);
	check_expected_ptr(mode);
	return mock_type(bool);
}

// cppcheck-suppress staticFunction
bool __wrap_mkdir_p(char *path, mode_t mode) {
	check_expected_ptr(path);
	check_expected_int(mode);
	return mock_type(bool);
}

// cppcheck-suppress staticFunction
void __wrap_fd_wd_cfg_dir_create(void) {
	function_called();
}

// cppcheck-suppress staticFunction
void __wrap_fd_wd_cfg_dir_destroy(void) {
	function_called();
}

static void clean_files(void) {
	remove("tst/tmp/write-existing-cfg.yaml");
	remove("tst/tmp/resolved.yaml");
	remove("tst/tmp/resolve/link.yaml");
	rmdir("tst/tmp/resolve");
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
	logs_clear();

	slist_free_vals(&cfg_file_paths, NULL);

	clean_files();

	g_cfg = cfg_default();

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

	slist_free_vals(&cfg_file_paths, NULL);

	clean_files();

	cfg_destroy();

	return 0;
}


static void cfg_file_write__bad_yaml(void **state) {
	g_cfg->file_path = strdup("something");

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, NULL, char*);

	cfg_file_write();

	assert_logs_empty();
}

static void cfg_file_write__none(void **state) {
	slist_append(&cfg_file_paths, strdup("/path/to/zero"));

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

	cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: /path/to/zero\n");
	assert_logs_empty();

	assert_str_equal(g_cfg->file_path, "/path/to/zero");
	assert_str_equal(g_cfg->dir_path, "/path/to");
	assert_str_equal(g_cfg->file_name, "zero");
	assert_str_equal(g_cfg->resolved_from, "/path/to/zero");
	assert_ptr_equal(g_cfg->resolved_from, slist_at(cfg_file_paths, 0));
	assert_int_equal(g_cfg->updated, false);
}

static void cfg_file_write__cannot_write_use_alternative(void **state) {
	slist_append(&cfg_file_paths, strdup("/path/to/zero"));
	slist_append(&cfg_file_paths, strdup("/path/to/one"));
	slist_append(&cfg_file_paths, strdup("/path/to/two"));
	slist_append(&cfg_file_paths, strdup("/path/to/three"));
	slist_append(&cfg_file_paths, strdup("/path/to/four"));

	g_cfg->file_path = strdup("/path/to/two");
	g_cfg->dir_path = strdup("nothing");
	g_cfg->file_name = strdup("missing");
	g_cfg->resolved_from = slist_at(cfg_file_paths, 2);

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

	cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: /path/to/three\n");
	assert_logs_empty();

	assert_str_equal(g_cfg->file_path, "/path/to/three");
	assert_str_equal(g_cfg->dir_path, "/path/to");
	assert_str_equal(g_cfg->file_name, "three");
	assert_str_equal(g_cfg->resolved_from, "/path/to/three");
	assert_ptr_equal(g_cfg->resolved_from, slist_at(cfg_file_paths, 3));
	assert_int_equal(g_cfg->updated, false);

	free(expected);
}

static void cfg_file_write__cannot_write_no_alternative(void **state) {
	slist_append(&cfg_file_paths, strdup("/path/to/zero"));
	slist_append(&cfg_file_paths, strdup("/path/to/one"));

	g_cfg->file_path = strdup("/path/to/zero");
	g_cfg->dir_path = strdup("/path/to");
	g_cfg->file_name = strdup("one");
	g_cfg->resolved_from = slist_at(cfg_file_paths, 0);

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

	cfg_file_write();

	assert_nul(g_cfg->file_path);
	assert_nul(g_cfg->dir_path);
	assert_nul(g_cfg->file_name);
	assert_nul(g_cfg->resolved_from);
	assert_int_equal(g_cfg->updated, false);

	free(expected);

	assert_logs_empty();
}

static void cfg_file_write__existing(void **state) {
	g_cfg->file_path = strdup("tst/tmp/write-existing-cfg.yaml");

	FILE *f = fopen(g_cfg->file_path, "w");
	assert_non_nul(f);
	if (f) {
		fclose(f);
	}

	char *expected = strdup("XXXX");

	expect_ptr(__wrap_yaml_marshal, data, g_cfg);
	expect_str(__wrap_yaml_marshal, human, "cfg");
	will_return_ptr_type(__wrap_yaml_marshal, strdup(expected), char*);

	expect_str(__wrap_file_write, path, g_cfg->file_path);
	expect_str(__wrap_file_write, contents, COMMENT_YAML_SCHEMA);
	expect_str(__wrap_file_write, mode, "w");
	will_return_int(__wrap_file_write, true);

	expect_str(__wrap_file_write, path, g_cfg->file_path);
	expect_str(__wrap_file_write, contents, expected);
	expect_str(__wrap_file_write, mode, "a");
	will_return_int(__wrap_file_write, true);

	cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: tst/tmp/write-existing-cfg.yaml\n");
	assert_logs_empty();

	assert_int_equal(g_cfg->updated, true);

	free(expected);
}

static void cfg_file_paths_init__min(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	unsetenv("HOME");

	cfg_file_paths_init("inexistent");

	assert_str_equal(slist_at(cfg_file_paths, 0), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 1), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 2);

	assert_logs_empty();
}

static void cfg_file_paths_init__xch(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	cfg_file_paths_init(NULL);

	assert_str_equal(slist_at(cfg_file_paths, 0), "xch/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 1), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 2), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 3);

	assert_logs_empty();
}

static void cfg_file_paths_init__home(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	setenv("HOME", "hom", 1);

	cfg_file_paths_init(NULL);

	assert_str_equal(slist_at(cfg_file_paths, 0), "hom/.config/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 1), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 2), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 3);

	assert_logs_empty();
}

static void cfg_file_paths_init__user(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	cfg_file_paths_init(".");

	assert_str_equal(slist_at(cfg_file_paths, 0), ".");

	assert_str_equal(slist_at(cfg_file_paths, 1), "xch/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 2), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 3), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 4);

	assert_logs_empty();
}

static void cfg_resolve_file_path__not_found(void **state) {
	char cwd[PATH_MAX];
	char file_path[PATH_MAX + 20];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	snprintf(file_path, sizeof(file_path), "%s/inexistent.yaml", cwd);

	slist_append(&cfg_file_paths, strdup(file_path));

	assert_false(cfg_resolve_file_path(g_cfg));

	assert_nul(g_cfg->file_path);
	assert_nul(g_cfg->dir_path);
	assert_nul(g_cfg->file_name);
	assert_nul(g_cfg->resolved_from);

	assert_logs_empty();
}

static void cfg_resolve_file_path__direct(void **state) {
	char cwd[PATH_MAX];
	char dir_path[PATH_MAX + 20];
	char file_path[PATH_MAX + 40];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	snprintf(dir_path, sizeof(dir_path), "%s/tst/tmp", cwd);
	snprintf(file_path, sizeof(file_path), "%s/resolved.yaml", dir_path);
	slist_append(&cfg_file_paths, strdup(file_path));

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	if (f) {
		fclose(f);
	}

	assert_true(cfg_resolve_file_path(g_cfg));

	assert_str_equal(g_cfg->file_path, file_path);
	assert_str_equal(g_cfg->dir_path, dir_path);
	assert_str_equal(g_cfg->file_name, "resolved.yaml");
	assert_str_equal(g_cfg->resolved_from, file_path);
	assert_ptr_equal(g_cfg->resolved_from, slist_at(cfg_file_paths, 0));

	assert_logs_empty();
}

static void cfg_resolve_file_path__linked(void **state) {
	char cwd[PATH_MAX];
	char dir_path[PATH_MAX + 20];
	char file_path[PATH_MAX + 40];
	char linked_path[PATH_MAX + 50];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	assert_int_equal(mkdir("tst/tmp/resolve", 0755), 0);

	snprintf(dir_path, sizeof(dir_path), "%s/tst/tmp", cwd);
	snprintf(file_path, sizeof(file_path), "%s/resolved.yaml", dir_path);
	snprintf(linked_path, sizeof(linked_path), "%s/tst/tmp/resolve/link.yaml", cwd);
	slist_append(&cfg_file_paths, strdup(linked_path));

	FILE *f = fopen(file_path, "w");
	assert_non_nul(f);
	if (f) {
		fclose(f);
	}
	assert_int_equal(symlink(file_path, linked_path), 0);

	assert_true(cfg_resolve_file_path(g_cfg));

	assert_str_equal(g_cfg->file_path, file_path);
	assert_str_equal(g_cfg->dir_path, dir_path);
	assert_str_equal(g_cfg->file_name, "resolved.yaml");
	assert_str_equal(g_cfg->resolved_from, linked_path);
	assert_ptr_equal(g_cfg->resolved_from, slist_at(cfg_file_paths, 0));

	assert_logs_empty();
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(cfg_file_write__bad_yaml),
		TEST(cfg_file_write__none),
		TEST(cfg_file_write__cannot_write_use_alternative),
		TEST(cfg_file_write__cannot_write_no_alternative),
		TEST(cfg_file_write__existing),

		TEST(cfg_file_paths_init__min),
		TEST(cfg_file_paths_init__home),
		TEST(cfg_file_paths_init__xch),
		TEST(cfg_file_paths_init__user),

		TEST(cfg_resolve_file_path__not_found),
		TEST(cfg_resolve_file_path__direct),
		TEST(cfg_resolve_file_path__linked),
	};

	return RUN(tests);
}


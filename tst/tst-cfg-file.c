#include "tst.h"
#include "asserts.h"

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

#include "cfg.h"

char *env_xdg_config_home = NULL;
char *env_home = NULL;


char *__wrap_marshal_cfg(struct Cfg *cfg) {
	check_expected(cfg);
	return mock_type(char*);
}

bool __wrap_file_write(const char *path, const char *contents) {
	check_expected(path);
	check_expected(contents);
	return mock_type(bool);
}

bool __wrap_mkdir_p(char *path, mode_t mode) {
	check_expected(path);
	check_expected(mode);
	return mock_type(bool);
}

void __wrap_fd_wd_cfg_dir_create(void) {
	mock();
}

void __wrap_fd_wd_cfg_dir_destroy(void) {
	mock();
}

void clean_files(void) {
	remove("tst/tmp/write-existing-cfg.yaml");
	remove("tst/tmp/resolved.yaml");
	remove("tst/tmp/resolve/link.yaml");
	rmdir("tst/tmp/resolve");
}


int before_all(void **state) {
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

int after_all(void **state) {
	free(env_xdg_config_home);
	free(env_home);

	rmdir("tst/tmp");

	return 0;
}

int before_each(void **state) {
	slist_free_vals(&cfg_file_paths, NULL);

	clean_files();

	cfg = cfg_default();

	return 0;
}

int after_each(void **state) {
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

	assert_logs_empty();

	return 0;
}


void cfg_file_write__bad_yaml(void **state) {
	cfg->file_path = strdup("something");

	expect_string(__wrap_marshal_cfg, cfg, cfg);
	will_return(__wrap_marshal_cfg, NULL);

	cfg_file_write();
}

void cfg_file_write__none(void **state) {
	slist_append(&cfg_file_paths, strdup("/path/to/zero"));

	char *expected = strdup("XXXX");

	expect_string(__wrap_marshal_cfg, cfg, cfg);
	will_return(__wrap_marshal_cfg, expected);

	will_return(__wrap_fd_wd_cfg_dir_destroy, NULL);

	expect_string(__wrap_mkdir_p, path, "/path/to");
	expect_value(__wrap_mkdir_p, mode, 0755);
	will_return(__wrap_mkdir_p, true);

	expect_string(__wrap_file_write, path, "/path/to/zero");
	expect_string(__wrap_file_write, contents, expected);
	will_return(__wrap_file_write, true);

	will_return(__wrap_fd_wd_cfg_dir_create, NULL);

	cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: /path/to/zero\n");

	assert_str_equal(cfg->file_path, "/path/to/zero");
	assert_str_equal(cfg->dir_path, "/path/to");
	assert_str_equal(cfg->file_name, "zero");
	assert_str_equal(cfg->resolved_from, "/path/to/zero");
	assert_ptr_equal(cfg->resolved_from, slist_at(cfg_file_paths, 0));
	assert_int_equal(cfg->updated, false);
}

void cfg_file_write__cannot_write_use_alternative(void **state) {
	slist_append(&cfg_file_paths, strdup("/path/to/zero"));
	slist_append(&cfg_file_paths, strdup("/path/to/one"));
	slist_append(&cfg_file_paths, strdup("/path/to/two"));
	slist_append(&cfg_file_paths, strdup("/path/to/three"));
	slist_append(&cfg_file_paths, strdup("/path/to/four"));

	cfg->file_path = strdup("/path/to/two");
	cfg->dir_path = strdup("nothing");
	cfg->file_name = strdup("missing");
	cfg->resolved_from = slist_at(cfg_file_paths, 2);

	char *expected = strdup("XXXX");

	expect_string(__wrap_marshal_cfg, cfg, cfg);
	will_return(__wrap_marshal_cfg, strdup(expected));

	will_return(__wrap_fd_wd_cfg_dir_destroy, NULL);

	expect_string(__wrap_file_write, path, "/path/to/two");
	expect_string(__wrap_file_write, contents, expected);
	will_return(__wrap_file_write, false);

	expect_string(__wrap_mkdir_p, path, "/path/to");
	expect_value(__wrap_mkdir_p, mode, 0755);
	will_return(__wrap_mkdir_p, true);

	expect_string(__wrap_file_write, path, "/path/to/zero");
	expect_string(__wrap_file_write, contents, expected);
	will_return(__wrap_file_write, false);

	expect_string(__wrap_mkdir_p, path, "/path/to");
	expect_value(__wrap_mkdir_p, mode, 0755);
	will_return(__wrap_mkdir_p, false);

	expect_string(__wrap_mkdir_p, path, "/path/to");
	expect_value(__wrap_mkdir_p, mode, 0755);
	will_return(__wrap_mkdir_p, true);

	expect_string(__wrap_file_write, path, "/path/to/three");
	expect_string(__wrap_file_write, contents, expected);
	will_return(__wrap_file_write, true);

	will_return(__wrap_fd_wd_cfg_dir_create, NULL);

	cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: /path/to/three\n");

	assert_str_equal(cfg->file_path, "/path/to/three");
	assert_str_equal(cfg->dir_path, "/path/to");
	assert_str_equal(cfg->file_name, "three");
	assert_str_equal(cfg->resolved_from, "/path/to/three");
	assert_ptr_equal(cfg->resolved_from, slist_at(cfg_file_paths, 3));
	assert_int_equal(cfg->updated, false);

	free(expected);
}

void cfg_file_write__cannot_write_no_alternative(void **state) {
	slist_append(&cfg_file_paths, strdup("/path/to/zero"));
	slist_append(&cfg_file_paths, strdup("/path/to/one"));

	cfg->file_path = strdup("/path/to/zero");
	cfg->dir_path = strdup("/path/to");
	cfg->file_name = strdup("one");
	cfg->resolved_from = slist_at(cfg_file_paths, 0);

	char *expected = strdup("XXXX");

	expect_string(__wrap_marshal_cfg, cfg, cfg);
	will_return(__wrap_marshal_cfg, strdup(expected));

	will_return(__wrap_fd_wd_cfg_dir_destroy, NULL);

	expect_string(__wrap_file_write, path, "/path/to/zero");
	expect_string(__wrap_file_write, contents, expected);
	will_return(__wrap_file_write, false);

	expect_string(__wrap_mkdir_p, path, "/path/to");
	expect_value(__wrap_mkdir_p, mode, 0755);
	will_return(__wrap_mkdir_p, true);

	expect_string(__wrap_file_write, path, "/path/to/one");
	expect_string(__wrap_file_write, contents, expected);
	will_return(__wrap_file_write, false);

	cfg_file_write();

	assert_nul(cfg->file_path);
	assert_nul(cfg->dir_path);
	assert_nul(cfg->file_name);
	assert_nul(cfg->resolved_from);
	assert_int_equal(cfg->updated, false);

	free(expected);
}

void cfg_file_write__existing(void **state) {
	cfg->file_path = strdup("tst/tmp/write-existing-cfg.yaml");

	FILE *f = fopen(cfg->file_path, "w");
	assert_non_nul(f);
	if (f) {
		fclose(f);
	}

	char *expected = strdup("XXXX");

	expect_string(__wrap_marshal_cfg, cfg, cfg);
	will_return(__wrap_marshal_cfg, strdup(expected));

	expect_string(__wrap_file_write, path, cfg->file_path);
	expect_string(__wrap_file_write, contents, expected);
	will_return(__wrap_file_write, true);

	cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: tst/tmp/write-existing-cfg.yaml\n");

	assert_int_equal(cfg->updated, true);

	free(expected);
}

void cfg_file_paths_init__min(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	unsetenv("HOME");

	cfg_file_paths_init("inexistent");

	assert_str_equal(slist_at(cfg_file_paths, 0), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 1), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 2);
}

void cfg_file_paths_init__xch(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	cfg_file_paths_init(NULL);

	assert_str_equal(slist_at(cfg_file_paths, 0), "xch/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 1), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 2), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 3);
}

void cfg_file_paths_init__home(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	setenv("HOME", "hom", 1);

	cfg_file_paths_init(NULL);

	assert_str_equal(slist_at(cfg_file_paths, 0), "hom/.config/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 1), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 2), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 3);
}

void cfg_file_paths_init__user(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	cfg_file_paths_init(".");

	assert_str_equal(slist_at(cfg_file_paths, 0), ".");

	assert_str_equal(slist_at(cfg_file_paths, 1), "xch/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 2), "/usr/local/etc/way-displays/cfg.yaml");

	assert_str_equal(slist_at(cfg_file_paths, 3), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 4);
}

void resolve_cfg_file__not_found(void **state) {
	char cwd[PATH_MAX];
	char file_path[PATH_MAX + 20];

	assert_non_nul(getcwd(cwd, PATH_MAX));

	snprintf(file_path, sizeof(file_path), "%s/inexistent.yaml", cwd);

	slist_append(&cfg_file_paths, strdup(file_path));

	assert_false(resolve_cfg_file(cfg));

	assert_nul(cfg->file_path);
	assert_nul(cfg->dir_path);
	assert_nul(cfg->file_name);
	assert_nul(cfg->resolved_from);
}

void resolve_cfg_file__direct(void **state) {
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

	assert_true(resolve_cfg_file(cfg));

	assert_str_equal(cfg->file_path, file_path);
	assert_str_equal(cfg->dir_path, dir_path);
	assert_str_equal(cfg->file_name, "resolved.yaml");
	assert_str_equal(cfg->resolved_from, file_path);
	assert_ptr_equal(cfg->resolved_from, slist_at(cfg_file_paths, 0));
}

void resolve_cfg_file__linked(void **state) {
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

	assert_true(resolve_cfg_file(cfg));

	assert_str_equal(cfg->file_path, file_path);
	assert_str_equal(cfg->dir_path, dir_path);
	assert_str_equal(cfg->file_name, "resolved.yaml");
	assert_str_equal(cfg->resolved_from, linked_path);
	assert_ptr_equal(cfg->resolved_from, slist_at(cfg_file_paths, 0));
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

		TEST(resolve_cfg_file__not_found),
		TEST(resolve_cfg_file__direct),
		TEST(resolve_cfg_file__linked),
	};

	return RUN(tests);
}


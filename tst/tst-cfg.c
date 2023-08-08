#include "tst.h"
#include "asserts.h"
#include "util.h"

#include <cmocka.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "global.h"
#include "slist.h"
#include "log.h"

#include "cfg.h"

struct Cfg *merge_set(struct Cfg *to, struct Cfg *from);
struct Cfg *merge_del(struct Cfg *to, struct Cfg *from);
void validate_warn(struct Cfg *cfg);
void validate_fix(struct Cfg *cfg);
bool resolve_cfg_file(struct Cfg *cfg);

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


void clean_files(void) {
	remove("write-existing-cfg.yaml");
	remove("resolved.yaml");
	remove("resolve/link.yaml");
	rmdir("resolve");
}


struct State {
	struct Cfg *from;
	struct Cfg *to;
	struct Cfg *expected;
};

int before_all(void **state) {
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

int after_all(void **state) {
	free(env_xdg_config_home);
	free(env_home);

	return 0;
}

int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	slist_free_vals(&cfg_file_paths, NULL);

	clean_files();

	cfg = cfg_default();

	s->from = cfg_default();
	s->to = cfg_default();
	s->expected = cfg_default();

	*state = s;
	return 0;
}

int after_each(void **state) {
	struct State *s = *state;

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

	cfg_free(s->from);
	cfg_free(s->to);
	cfg_free(s->expected);

	free(s);
	return 0;
}


void merge_set__arrange(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->expected->arrange = COL;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__align(void **state) {
	struct State *s = *state;

	s->from->align = MIDDLE;
	s->expected->align = MIDDLE;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__order(void **state) {
	struct State *s = *state;

	slist_append(&s->to->order_name_desc, strdup("A"));
	slist_append(&s->from->order_name_desc, strdup("X"));

	slist_append(&s->expected->order_name_desc, strdup("X"));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__auto_scale(void **state) {
	struct State *s = *state;

	s->from->auto_scale = OFF;
	s->expected->auto_scale = OFF;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__user_scale(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_scales, cfg_user_scale_init("to", 1));
	slist_append(&s->to->user_scales, cfg_user_scale_init("both", 2));

	slist_append(&s->from->user_scales, cfg_user_scale_init("from", 3));
	slist_append(&s->from->user_scales, cfg_user_scale_init("both", 4));

	slist_append(&s->expected->user_scales, cfg_user_scale_init("to", 1));
	slist_append(&s->expected->user_scales, cfg_user_scale_init("both", 4));
	slist_append(&s->expected->user_scales, cfg_user_scale_init("from", 3));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_modes, cfg_user_mode_init("to", false, 1, 2, 3, false));
	slist_append(&s->to->user_modes, cfg_user_mode_init("both", false, 4, 5, 6, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("from", false, 7, 8, 9, true));
	slist_append(&s->from->user_modes, cfg_user_mode_init("both", false, 10, 11, 12, true));

	slist_append(&s->expected->user_modes, cfg_user_mode_init("to", false, 1, 2, 3, false));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("both", false, 10, 11, 12, true));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("from", false, 7, 8, 9, true));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__adaptive_sync_off(void **state) {
	struct State *s = *state;

	slist_append(&s->to->adaptive_sync_off_name_desc, strdup("to"));
	slist_append(&s->to->adaptive_sync_off_name_desc, strdup("both"));

	slist_append(&s->from->adaptive_sync_off_name_desc, strdup("from"));
	slist_append(&s->from->adaptive_sync_off_name_desc, strdup("both"));

	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("to"));
	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("both"));
	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("from"));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__disabled(void **state) {
	struct State *s = *state;

	slist_append(&s->to->disabled_name_desc, strdup("to"));
	slist_append(&s->to->disabled_name_desc, strdup("both"));

	slist_append(&s->from->disabled_name_desc, strdup("from"));
	slist_append(&s->from->disabled_name_desc, strdup("both"));

	slist_append(&s->expected->disabled_name_desc, strdup("to"));
	slist_append(&s->expected->disabled_name_desc, strdup("both"));
	slist_append(&s->expected->disabled_name_desc, strdup("from"));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__scale(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_scales, cfg_user_scale_init("1", 1));
	slist_append(&s->to->user_scales, cfg_user_scale_init("2", 2));

	slist_append(&s->from->user_scales, cfg_user_scale_init("2", 3));
	slist_append(&s->from->user_scales, cfg_user_scale_init("3", 4));

	slist_append(&s->expected->user_scales, cfg_user_scale_init("1", 1));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_modes, cfg_user_mode_init("1", false, 1, 1, 1, false));
	slist_append(&s->to->user_modes, cfg_user_mode_init("2", false, 2, 2, 2, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("2", false, 2, 2, 2, false));
	slist_append(&s->from->user_modes, cfg_user_mode_init("3", false, 3, 3, 3, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("1", false, 1, 1, 1, false));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__adaptive_sync_off(void **state) {
	struct State *s = *state;

	slist_append(&s->to->adaptive_sync_off_name_desc, strdup("1"));
	slist_append(&s->to->adaptive_sync_off_name_desc, strdup("2"));

	slist_append(&s->from->adaptive_sync_off_name_desc, strdup("2"));
	slist_append(&s->from->adaptive_sync_off_name_desc, strdup("3"));

	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("1"));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__disabled(void **state) {
	struct State *s = *state;

	slist_append(&s->to->disabled_name_desc, strdup("1"));
	slist_append(&s->to->disabled_name_desc, strdup("2"));

	slist_append(&s->from->disabled_name_desc, strdup("2"));
	slist_append(&s->from->disabled_name_desc, strdup("3"));

	slist_append(&s->expected->disabled_name_desc, strdup("1"));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void validate_fix__col(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->from->align = TOP;

	s->expected->arrange = COL;
	s->expected->align = LEFT;

	validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring invalid ALIGN TOP for COLUMN arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.\n");

	assert_cfg_equal(s->from, s->expected);
}

void validate_fix__row(void **state) {
	struct State *s = *state;

	s->from->arrange = ROW;
	s->from->align = RIGHT;

	s->expected->arrange = ROW;
	s->expected->align = TOP;

	validate_fix(s->from);

	assert_log(WARNING, "\nIgnoring invalid ALIGN RIGHT for ROW arrange. Valid values are TOP, MIDDLE and BOTTOM. Using default TOP.\n");

	assert_cfg_equal(s->from, s->expected);
}

void validate_fix__scale(void **state) {
	struct State *s = *state;

	slist_append(&s->from->user_scales, cfg_user_scale_init("ok", 1));

	slist_append(&s->from->user_scales, cfg_user_scale_init("neg", -1));

	slist_append(&s->from->user_scales, cfg_user_scale_init("zero", 0));

	validate_fix(s->from);

	char *expected_log = read_file("tst/cfg/validate-fix-scale.log");
	assert_log(WARNING, expected_log);

	slist_append(&s->expected->user_scales, cfg_user_scale_init("ok", 1));

	assert_cfg_equal(s->from, s->expected);

	free(expected_log);
}

void validate_fix__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->from->user_modes, cfg_user_mode_init("ok", false, 1, 2, 3, false));
	slist_append(&s->from->user_modes, cfg_user_mode_init("max", true, -1, -1, -1, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("negative width", false, -99, 2, 3, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("negative height", false, 1, -99, 3, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("negative hz", false, 1, 2, -99, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("missing width", false, -1, 2, 3, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("missing height", false, 1, -1, 3, false));

	validate_fix(s->from);

	char *expected_log = read_file("tst/cfg/validate-fix-mode.log");
	assert_log(WARNING, expected_log);

	slist_append(&s->expected->user_modes, cfg_user_mode_init("ok", false, 1, 2, 3, false));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("max", true, -1, -1, -1, false));

	assert_cfg_equal(s->from, s->expected);

	free(expected_log);
}

void validate_warn__(void **state) {
	struct State *s = *state;

	slist_append(&s->expected->user_scales, cfg_user_scale_init("sss", 1));
	slist_append(&s->expected->user_scales, cfg_user_scale_init("ssssssss", 2));

	slist_append(&s->expected->user_modes, cfg_user_mode_init("mmm", false, 1, 1, 1, false));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("mmmmmmmm", false, 1, 1, 1, false));

	slist_append(&s->expected->order_name_desc, strdup("ooo"));
	slist_append(&s->expected->order_name_desc, strdup("oooooooooo"));

	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("vvv"));
	slist_append(&s->expected->adaptive_sync_off_name_desc, strdup("vvvvvvvvvv"));

	slist_append(&s->expected->max_preferred_refresh_name_desc, strdup("ppp"));
	slist_append(&s->expected->max_preferred_refresh_name_desc, strdup("pppppppppp"));

	slist_append(&s->expected->disabled_name_desc, strdup("ddd"));
	slist_append(&s->expected->disabled_name_desc, strdup("dddddddddd"));

	validate_warn(s->expected);

	char *expected_log = read_file("tst/cfg/validate-warn.log");
	assert_log(WARNING, expected_log);

	free(expected_log);
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
	will_return(__wrap_marshal_cfg, strdup(expected));

	expect_string(__wrap_mkdir_p, path, "/path/to");
	expect_value(__wrap_mkdir_p, mode, 0755);
	will_return(__wrap_mkdir_p, true);

	expect_string(__wrap_file_write, path, "/path/to/zero");
	expect_string(__wrap_file_write, contents, expected);
	will_return(__wrap_file_write, true);

	cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: /path/to/zero\n");

	assert_string_equal(cfg->file_path, "/path/to/zero");
	assert_string_equal(cfg->dir_path, "/path/to");
	assert_string_equal(cfg->file_name, "zero");
	assert_string_equal(cfg->resolved_from, "/path/to/zero");
	assert_ptr_equal(cfg->resolved_from, slist_at(cfg_file_paths, 0));
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

	cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: /path/to/three\n");

	assert_string_equal(cfg->file_path, "/path/to/three");
	assert_string_equal(cfg->dir_path, "/path/to");
	assert_string_equal(cfg->file_name, "three");
	assert_string_equal(cfg->resolved_from, "/path/to/three");
	assert_ptr_equal(cfg->resolved_from, slist_at(cfg_file_paths, 3));

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

	assert_null(cfg->file_path);
	assert_null(cfg->dir_path);
	assert_null(cfg->file_name);
	assert_null(cfg->resolved_from);

	free(expected);
}

void cfg_file_write__existing(void **state) {
	cfg->file_path = strdup("write-existing-cfg.yaml");

	FILE *f = fopen(cfg->file_path, "w");
	assert_non_null(f);
	assert_int_equal(fclose(f), 0);

	char *expected = strdup("XXXX");

	expect_string(__wrap_marshal_cfg, cfg, cfg);
	will_return(__wrap_marshal_cfg, strdup(expected));

	expect_string(__wrap_file_write, path, cfg->file_path);
	expect_string(__wrap_file_write, contents, expected);
	will_return(__wrap_file_write, true);

	cfg_file_write();

	assert_log(INFO, "\nWrote configuration file: write-existing-cfg.yaml\n");

	free(expected);
}

void cfg_file_paths_init__min(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	unsetenv("HOME");

	cfg_file_paths_init("inexistent");

	assert_string_equal(slist_at(cfg_file_paths, 0), "/usr/local/etc/way-displays/cfg.yaml");

	assert_string_equal(slist_at(cfg_file_paths, 1), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 2);
}

void cfg_file_paths_init__xch(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	cfg_file_paths_init(NULL);

	assert_string_equal(slist_at(cfg_file_paths, 0), "xch/way-displays/cfg.yaml");

	assert_string_equal(slist_at(cfg_file_paths, 1), "/usr/local/etc/way-displays/cfg.yaml");

	assert_string_equal(slist_at(cfg_file_paths, 2), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 3);
}

void cfg_file_paths_init__home(void **state) {
	unsetenv("XDG_CONFIG_HOME");
	setenv("HOME", "hom", 1);

	cfg_file_paths_init(NULL);

	assert_string_equal(slist_at(cfg_file_paths, 0), "hom/.config/way-displays/cfg.yaml");

	assert_string_equal(slist_at(cfg_file_paths, 1), "/usr/local/etc/way-displays/cfg.yaml");

	assert_string_equal(slist_at(cfg_file_paths, 2), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 3);
}

void cfg_file_paths_init__user(void **state) {
	setenv("XDG_CONFIG_HOME", "xch", 1);
	setenv("HOME", "hom", 1);

	cfg_file_paths_init(".");

	assert_string_equal(slist_at(cfg_file_paths, 0), ".");

	assert_string_equal(slist_at(cfg_file_paths, 1), "xch/way-displays/cfg.yaml");

	assert_string_equal(slist_at(cfg_file_paths, 2), "/usr/local/etc/way-displays/cfg.yaml");

	assert_string_equal(slist_at(cfg_file_paths, 3), ROOT_ETC"/way-displays/cfg.yaml");

	assert_int_equal(slist_length(cfg_file_paths), 4);
}

void resolve_cfg_file__not_found(void **state) {
	char cwd[PATH_MAX];
	char file_path[PATH_MAX + 64];

	getcwd(cwd, PATH_MAX);

	snprintf(file_path, PATH_MAX + 64, "%s/inexistent.yaml", cwd);

	slist_append(&cfg_file_paths, strdup(file_path));

	assert_false(resolve_cfg_file(cfg));

	assert_null(cfg->file_path);
	assert_null(cfg->dir_path);
	assert_null(cfg->file_name);
	assert_null(cfg->resolved_from);
}

void resolve_cfg_file__direct(void **state) {
	char cwd[PATH_MAX];
	char file_path[PATH_MAX * 2];

	getcwd(cwd, PATH_MAX);

	snprintf(file_path, sizeof(file_path), "%s/resolved.yaml", cwd);
	slist_append(&cfg_file_paths, strdup(file_path));

	FILE *f = fopen(file_path, "w");
	fclose(f);

	assert_true(resolve_cfg_file(cfg));

	assert_string_equal(cfg->file_path, file_path);
	assert_string_equal(cfg->dir_path, cwd);
	assert_string_equal(cfg->file_name, "resolved.yaml");
	assert_string_equal(cfg->resolved_from, file_path);
	assert_ptr_equal(cfg->resolved_from, slist_at(cfg_file_paths, 0));
}

void resolve_cfg_file__linked(void **state) {
	char cwd[PATH_MAX];
	char file_path[PATH_MAX * 2];
	char linked_path[PATH_MAX * 2];

	getcwd(cwd, PATH_MAX);

	assert_int_equal(mkdir("resolve", 0755), 0);

	snprintf(file_path, sizeof(file_path), "%s/resolved.yaml", cwd);
	snprintf(linked_path, sizeof(file_path), "%s/resolve/link.yaml", cwd);
	slist_append(&cfg_file_paths, strdup(linked_path));

	FILE *f = fopen(file_path, "w");
	fclose(f);
	symlink(file_path, linked_path);

	assert_true(resolve_cfg_file(cfg));

	assert_string_equal(cfg->file_path, file_path);
	assert_string_equal(cfg->dir_path, cwd);
	assert_string_equal(cfg->file_name, "resolved.yaml");
	assert_string_equal(cfg->resolved_from, linked_path);
	assert_ptr_equal(cfg->resolved_from, slist_at(cfg_file_paths, 0));
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(merge_set__arrange),
		TEST(merge_set__align),
		TEST(merge_set__order),
		TEST(merge_set__auto_scale),
		TEST(merge_set__user_scale),
		TEST(merge_set__mode),
		TEST(merge_set__adaptive_sync_off),
		TEST(merge_set__disabled),

		TEST(merge_del__scale),
		TEST(merge_del__mode),
		TEST(merge_del__adaptive_sync_off),
		TEST(merge_del__disabled),

		TEST(validate_fix__col),
		TEST(validate_fix__row),
		TEST(validate_fix__scale),
		TEST(validate_fix__mode),

		TEST(validate_warn__),

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


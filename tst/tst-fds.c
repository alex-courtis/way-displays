#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cfg.h"
#include "log.h"
#include "global.h"

#include "fds.h"

char DIR_TMP[PATH_MAX];

int before_all(void **state) {
	char cwd[PATH_MAX];
	getcwd(cwd, PATH_MAX);
	sprintf(DIR_TMP, "%s/tst/tmp", cwd);
	mkdir(DIR_TMP, 0755);

	return 0;
}

int after_all(void **state) {
	rmdir(DIR_TMP);

	return 0;
}

int before_each(void **state) {
	cfg = cfg_default();

	return 0;
}

int after_each(void **state) {
	assert_logs_empty();

	cfg_destroy();

	fd_cfg_dir = -1;
	wd_cfg_dir = -1;

	return 0;
}

void fd_wd_cfg_dir_create__no_dir(void **state) {
	fd_wd_cfg_dir_create();

	assert_int_equal(fd_cfg_dir, -1);
	assert_int_equal(wd_cfg_dir, -1);
}

void fd_wd_cfg_dir_create__bad_dir(void **state) {
	cfg->dir_path = strdup("/inexistent");

	expect_value(__wrap_wd_exit_message, __status, EXIT_FAILURE);

	fd_wd_cfg_dir_create();

	assert_int_equal(fd_cfg_dir, -1);
	assert_int_equal(wd_cfg_dir, -1);

	assert_log(ERROR, "\nunable to create config directory watch for /inexistent, exiting\n");
}

void fd_wd_cfg_dir_create__ok(void **state) {
	cfg->dir_path = strdup(DIR_TMP);

	fd_wd_cfg_dir_create();

	assert_int_not_equal(fd_cfg_dir, -1);
	assert_int_not_equal(wd_cfg_dir, -1);

	assert_int_not_equal(inotify_rm_watch(fd_cfg_dir, wd_cfg_dir), -1);

	assert_int_equal(close(fd_cfg_dir), 0);
}

void fd_wd_cfg_dir_destroy__bad(void **state) {
	fd_cfg_dir = 123;
	wd_cfg_dir = 456;

	fd_wd_cfg_dir_destroy();

	assert_int_equal(fd_cfg_dir, -1);
	assert_int_equal(wd_cfg_dir, -1);

	assert_log(ERROR, "\nunable to remove config directory watch\n\nunable to close config directory watch\n");
}

void fd_wd_cfg_dir_destroy__ok(void **state) {
	fd_cfg_dir = inotify_init1(IN_NONBLOCK);
	wd_cfg_dir = inotify_add_watch(fd_cfg_dir, DIR_TMP, IN_CLOSE_WRITE);

	assert_int_not_equal(fd_cfg_dir, -1);
	assert_int_not_equal(wd_cfg_dir, -1);

	fd_wd_cfg_dir_destroy();

	assert_int_equal(fd_cfg_dir, -1);
	assert_int_equal(wd_cfg_dir, -1);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(fd_wd_cfg_dir_create__ok),
		TEST(fd_wd_cfg_dir_create__no_dir),
		TEST(fd_wd_cfg_dir_create__bad_dir),

		TEST(fd_wd_cfg_dir_destroy__bad),
		TEST(fd_wd_cfg_dir_destroy__ok),
	};

	return RUN(tests);
}


#include "tst.h"

#include "assert-log.h"
#include "asserts.h"

#include <cmocka.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cfg.h"
#include "cfg/file.h"
#include "enum.h"
#include "str.h"

#include "fds.h"

char *DIR_TMP = NULL;

static int before_all(void **state) {
	char cwd[PATH_MAX];
	assert_non_nul(getcwd(cwd, PATH_MAX));
	DIR_TMP = snprintf_alloc(PATH_MAX, "%s/tst/tmp", cwd);
	mkdir(DIR_TMP, 0755);

	return 0;
}

static int after_all(void **state) {
	rmdir(DIR_TMP);
	free(DIR_TMP);

	return 0;
}

static int before_each(void **state) {
	g_cfg = cfg_default();

	memset(&g_cfg_file, 0, sizeof(struct CfgFile));

	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	g_cfg_destroy();

	fd_cfg_dir = -1;
	wd_cfg_dir = -1;

	return 0;
}

static void fd_wd_cfg_dir_create__no_dir(void **state) {
	fd_wd_cfg_dir_create();

	assert_int_equal(fd_cfg_dir, -1);
	assert_int_equal(wd_cfg_dir, -1);
}

static void fd_wd_cfg_dir_create__bad_dir(void **state) {
	strncpy(g_cfg_file.dir_path, "/inexistent", PATH_MAX - 1);

	expect_int_value(__wrap_wd_exit_message, __status, EXIT_FAILURE);

	fd_wd_cfg_dir_create();

	assert_int_equal(fd_cfg_dir, -1);
	assert_int_equal(wd_cfg_dir, -1);

	assert_log(FATAL, "\nunable to create config directory watch for /inexistent, exiting\n");
}

static void fd_wd_cfg_dir_create__ok(void **state) {
	strncpy(g_cfg_file.dir_path, DIR_TMP, PATH_MAX - 1);

	fd_wd_cfg_dir_create();

	assert_int_not_equal(fd_cfg_dir, -1);
	assert_int_not_equal(wd_cfg_dir, -1);

	assert_int_not_equal(inotify_rm_watch(fd_cfg_dir, wd_cfg_dir), -1);

	assert_int_equal(close(fd_cfg_dir), 0);
}

static void fd_wd_cfg_dir_destroy__bad(void **state) {
	fd_cfg_dir = 123;
	wd_cfg_dir = 456;

	fd_wd_cfg_dir_destroy();

	assert_int_equal(fd_cfg_dir, -1);
	assert_int_equal(wd_cfg_dir, -1);

	assert_log(ERROR, "\nunable to remove config directory watch\n\nunable to close config directory watch\n");
}

static void fd_wd_cfg_dir_destroy__ok(void **state) {
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
		TEST_BA(fd_wd_cfg_dir_create__ok),
		TEST_BA(fd_wd_cfg_dir_create__no_dir),
		TEST_BA(fd_wd_cfg_dir_create__bad_dir),

		TEST_BA(fd_wd_cfg_dir_destroy__bad),
		TEST_BA(fd_wd_cfg_dir_destroy__ok),
	};

	return RUN_BA(tests);
}


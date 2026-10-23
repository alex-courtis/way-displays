#include "tst.h"

#include "assert-log.h"
#include "expects.h"

#include <cmocka.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#include "enum.h"
#include "process.h"

static char pid_path[] = "/tmp/way-displays.test-XXXXXX.pid";
static char *saved_vtnr;
static int fixture_fd;
static int pid_fd;

int __real_flock(int fd, int operation);

int __wrap_flock(int fd, int operation) { // cppcheck-suppress unusedFunction
	// Keep the real lock behavior and close the server's descriptor after each test.
	pid_fd = fd;
	return __real_flock(fd, operation);
}

void __wrap_exit(int status) { // cppcheck-suppress staticFunction
	check_expected_int(status);
	mock_assert(0, "exit", __FILE__, __LINE__);
	abort();
}

void __wrap_cli_usage(FILE *out) { // cppcheck-suppress staticFunction
	check_expected_ptr(out);
}

static int before_each(void **state) {
	const char *vtnr = getenv("XDG_VTNR");
	saved_vtnr = vtnr ? strdup(vtnr) : NULL;
	pid_fd = -1;

	strcpy(pid_path, "/tmp/way-displays.test-XXXXXX.pid");
	fixture_fd = mkstemps(pid_path, 4);
	assert_int_not_equal(fixture_fd, -1);

	char test_vtnr[12];
	snprintf(test_vtnr, sizeof(test_vtnr), "%.11s", pid_path + strlen("/tmp/way-displays."));
	assert_int_equal(setenv("XDG_VTNR", test_vtnr, 1), 0);

	return 0;
}

static int after_each(void **state) {
	if (pid_fd != -1)
		assert_int_equal(close(pid_fd), 0);
	assert_int_equal(close(fixture_fd), 0);
	assert_int_equal(unlink(pid_path), 0);

	if (saved_vtnr) {
		assert_int_equal(setenv("XDG_VTNR", saved_vtnr, 1), 0);
		free(saved_vtnr);
	} else {
		assert_int_equal(unsetenv("XDG_VTNR"), 0);
	}

	return 0;
}

static void assert_pid_file(pid_t expected) {
	char buf[32] = { 0 };
	assert_int_not_equal(pread(fixture_fd, buf, sizeof(buf) - 1, 0), -1);

	char expected_buf[32];
	snprintf(expected_buf, sizeof(expected_buf), "%d", expected);
	assert_string_equal(buf, expected_buf);
}

static void pid_file_create__stale_pid(void **state) {
	// The parent is alive but does not own the PID file's lock.
	assert_true(dprintf(fixture_fd, "%d", getppid()) > 0);

	pid_file_create();

	assert_pid_file(getpid());
	assert_int_equal(__real_flock(fixture_fd, LOCK_EX | LOCK_NB), -1);
	assert_int_equal(errno, EWOULDBLOCK);
	assert_logs_empty();
}

static void pid_file_create__empty_file(void **state) {
	pid_file_create();

	assert_pid_file(getpid());
	assert_logs_empty();
}

static void pid_file_create__locked_file(void **state) {
	assert_true(dprintf(fixture_fd, "%d", getppid()) > 0);
	assert_int_equal(__real_flock(fixture_fd, LOCK_EX | LOCK_NB), 0);

	expect_ptr(__wrap_cli_usage, out, stderr);
	expect_int_value(__wrap_exit, status, EXIT_FAILURE);
	expect_assert_failure(pid_file_create());

	assert_pid_file(getppid());
	char expected_log[128];
	snprintf(expected_log, sizeof(expected_log), "another instance %d is running, exiting\n\n", getppid());
	assert_log(FATAL, expected_log);
	assert_logs_empty();
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(pid_file_create__stale_pid),
		TEST_BA(pid_file_create__empty_file),
		TEST_BA(pid_file_create__locked_file),
	};

	return RUN(tests);
}

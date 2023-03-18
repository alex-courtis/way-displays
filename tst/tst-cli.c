#include "tst.h"
#include "wraps-log.h"

#include <cmocka.h>
#include <stdlib.h>
#include <unistd.h>

#include "cli.h"
#include "ipc.h"

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	return 0;
}

int after_each(void **state) {
	return 0;
}

extern void __wrap_wd_exit(int __status) {
	check_expected(__status);
}

void parse_write__nargs(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_log_error("--write takes no arguments", NULL, NULL, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_write(1, NULL));
}

void parse_write__ok(void **state) {
	optind = 0;

	struct IpcRequest *request = parse_write(0, NULL);

	assert_non_null(request);
	assert_int_equal(request->op, CFG_WRITE);
}

void parse_set__mode_nargs(void **state) {
	optind = 0;
	optarg = "MODE";

	expect_log_error("%s requires two to four arguments", optarg, NULL, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_set(1, NULL));

	expect_log_error("%s requires two to four arguments", optarg, NULL, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_set(5, NULL));
}

void parse_set__arrange_align_nargs(void **state) {
	optind = 0;
	optarg = "ARRANGE_ALIGN";

	expect_log_error("%s requires two arguments", optarg, NULL, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_set(0, NULL));
}

void parse_set__scale_nargs(void **state) {
	optind = 0;
	optarg = "SCALE";

	expect_log_error("%s requires two arguments", optarg, NULL, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_set(0, NULL));
}

void parse_set__auto_scale_nargs(void **state) {
	optind = 0;
	optarg = "AUTO_SCALE";

	expect_log_error("%s requires one argument", optarg, NULL, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_set(0, NULL));
}

void parse_set__disabled_nargs(void **state) {
	optind = 0;
	optarg = "DISABLED";

	expect_log_error("%s requires one argument", optarg, NULL, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_set(0, NULL));
}

void parse_set__order_nargs(void **state) {
	optind = 0;
	optarg = "ORDER";

	expect_log_error("%s requires at least one argument", optarg, NULL, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_set(0, NULL));
}

void parse_set__invalid(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_log_error("invalid %s: %s", "set", optarg, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_set(0, NULL));
}

void parse_set__ok(void **state) {
	optind = 0;
	static char *argv[1] = { "arg0", };

	optarg = "DISABLED";

	struct IpcRequest *request = parse_set(1, argv);

	assert_non_null(request);
	assert_int_equal(request->op, CFG_SET);
}

void parse_del__mode_nargs(void **state) {
	optind = 0;
	optarg = "MODE";

	expect_log_error("%s requires one argument", optarg, NULL, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_del(0, NULL));
}

void parse_del__scale_nargs(void **state) {
	optind = 0;
	optarg = "SCALE";

	expect_log_error("%s requires one argument", optarg, NULL, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_del(0, NULL));
}

void parse_del__disabled_nargs(void **state) {
	optind = 0;
	optarg = "DISABLED";

	expect_log_error("%s requires one argument", optarg, NULL, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_del(0, NULL));
}

void parse_del__invalid(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_log_error("invalid %s: %s", "delete", optarg, NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_del(0, NULL));
}

void parse_del__ok(void **state) {
	optind = 0;
	static char *argv[1] = { "arg0", };

	optarg = "MODE";

	struct IpcRequest *request = parse_del(1, argv);

	assert_non_null(request);
	assert_int_equal(request->op, CFG_DEL);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(parse_write__nargs),
		TEST(parse_write__ok),

		TEST(parse_set__mode_nargs),
		TEST(parse_set__arrange_align_nargs),
		TEST(parse_set__scale_nargs),
		TEST(parse_set__auto_scale_nargs),
		TEST(parse_set__disabled_nargs),
		TEST(parse_set__order_nargs),
		TEST(parse_set__invalid),
		TEST(parse_set__ok),

		TEST(parse_del__mode_nargs),
		TEST(parse_del__scale_nargs),
		TEST(parse_del__disabled_nargs),
		TEST(parse_del__invalid),
		TEST(parse_del__ok),
	};

	return RUN(tests);
}


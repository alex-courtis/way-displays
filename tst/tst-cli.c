#include "tst.h"
#include "asserts.h"
#include "expects.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cfg.h"
#include "ipc.h"
#include "list.h"
#include "log.h"

struct Cfg *parse_element(enum IpcRequestOperation op, enum CfgElement element, int argc, char **argv);
struct IpcRequest *parse_write(int argc, char **argv);
struct IpcRequest *parse_set(int argc, char **argv);
struct IpcRequest *parse_del(int argc, char **argv);
bool parse_log_threshold(char *optarg);


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


void parse_element__arrange_align_invalid_arrange(void **state) {
	optind = 0;
	char *argv[] = { "ROW", "INVALID" };

	expect_log_error("invalid %s%s", "ARRANGE_ALIGN", " ROW INVALID", NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_element(CFG_SET, ARRANGE_ALIGN, 2, argv));
}

void parse_element__arrange_align_invalid_align(void **state) {
	optind = 0;
	char *argv[] = { "INVALID", "LEFT" };

	expect_log_error("invalid %s%s", "ARRANGE_ALIGN", " INVALID LEFT", NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_element(CFG_SET, ARRANGE_ALIGN, 2, argv));
}

void parse_element__arrange_align_ok(void **state) {
	optind = 0;
	char *argv[] = { "ROW", "LEFT" };

	struct Cfg *actual = parse_element(CFG_SET, ARRANGE_ALIGN, 2, argv);

	struct Cfg expected = {
		.arrange = ROW,
		.align = LEFT,
	};

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);
}

void parse_element__auto_scale_invalid(void **state) {
	optind = 0;
	char *argv[] = { "INVALID", };

	expect_log_error("invalid %s%s", "AUTO_SCALE", " INVALID", NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_element(CFG_SET, AUTO_SCALE, 1, argv));
}

void parse_element__auto_scale_ok(void **state) {
	optind = 0;
	char *argv[] = { "ON", };

	struct Cfg *actual = parse_element(CFG_SET, AUTO_SCALE, 1, argv);

	struct Cfg expected = {
		.auto_scale = ON,
	};

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);
}

void parse_element__scale_set_invalid(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "NOTANUMBER", };

	expect_log_error("invalid %s%s", "SCALE", " DISPL NOTANUMBER", NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_element(CFG_SET, SCALE, 2, argv));
}

void parse_element__scale_set_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1234.5" };

	struct Cfg *actual = parse_element(CFG_SET, SCALE, 2, argv);

	struct UserScale expectedUserScale = {
		.name_desc = "DISPL",
		.scale = 1234.5,
	};
	struct Cfg expected = {
		.user_scales = NULL,
	};
	slist_append(&expected.user_scales, &expectedUserScale);

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);

	slist_free(&expected.user_scales);
}

void parse_element__scale_del_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", };

	struct Cfg *actual = parse_element(CFG_DEL, SCALE, 1, argv);

	struct UserScale expectedUserScale = {
		.name_desc = "DISPL",
		.scale = 1,
	};
	struct Cfg expected = { 0 };
	slist_append(&expected.user_scales, &expectedUserScale);

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);

	slist_free(&expected.user_scales);
}

void parse_element__mode_set_invalid_width(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "NAN", "2", "3", };

	expect_log_error("invalid %s%s", "MODE", " DISPL NAN 2 3", NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_element(CFG_SET, MODE, 4, argv));
}

void parse_element__mode_set_invalid_height(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "NAN", "3", };

	expect_log_error("invalid %s%s", "MODE", " DISPL 1 NAN 3", NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_element(CFG_SET, MODE, 4, argv));
}

void parse_element__mode_set_invalid_refresh(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "2", "NAN", };

	expect_log_error("invalid %s%s", "MODE", " DISPL 1 2 NAN", NULL, NULL);
	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_null(parse_element(CFG_SET, MODE, 4, argv));
}

void parse_element__mode_set_max(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "MAX" };

	struct Cfg *actual = parse_element(CFG_SET, MODE, 2, argv);

	struct UserMode *expectedUserMode = cfg_user_mode_default();
	expectedUserMode->name_desc = strdup("DISPL");
	expectedUserMode->max = true;

	struct Cfg expected = { 0 };
	slist_append(&expected.user_modes, expectedUserMode);

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);

	slist_free(&expected.user_modes);
	cfg_user_mode_free(expectedUserMode);
}

void parse_element__mode_set_res(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "2" };

	struct Cfg *actual = parse_element(CFG_SET, MODE, 3, argv);

	struct UserMode *expectedUserMode = cfg_user_mode_default();
	expectedUserMode->name_desc = strdup("DISPL");
	expectedUserMode->max = false;
	expectedUserMode->width = 1;
	expectedUserMode->height = 2;

	struct Cfg expected = { 0 };
	slist_append(&expected.user_modes, expectedUserMode);

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);

	slist_free(&expected.user_modes);
	cfg_user_mode_free(expectedUserMode);
}

void parse_element__mode_set_res_refresh(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "2", "3", };

	struct Cfg *actual = parse_element(CFG_SET, MODE, 4, argv);

	struct UserMode *expectedUserMode = cfg_user_mode_default();
	expectedUserMode->name_desc = strdup("DISPL");
	expectedUserMode->max = false;
	expectedUserMode->width = 1;
	expectedUserMode->height = 2;
	expectedUserMode->refresh_hz = 3;

	struct Cfg expected = { 0 };
	slist_append(&expected.user_modes, expectedUserMode);

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);

	slist_free(&expected.user_modes);
	cfg_user_mode_free(expectedUserMode);
}

void parse_element__mode_del_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", };

	struct Cfg *actual = parse_element(CFG_DEL, MODE, 1, argv);

	struct UserMode *expectedUserMode = cfg_user_mode_default();
	expectedUserMode->name_desc = strdup("DISPL");
	expectedUserMode->max = true;

	struct Cfg expected = { 0 };
	slist_append(&expected.user_modes, expectedUserMode);

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);

	slist_free(&expected.user_modes);
	cfg_user_mode_free(expectedUserMode);
}

void parse_element__adaptive_sync_off_ok(void **state) {
	optind = 0;
	char *argv[] = { "ONE", "TWO", };

	struct Cfg *actual = parse_element(CFG_SET, VRR_OFF, 2, argv);

	struct Cfg expected = { 0 };
	slist_append(&expected.adaptive_sync_off_name_desc, "ONE");
	slist_append(&expected.adaptive_sync_off_name_desc, "TWO");

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);

	slist_free(&expected.disabled_name_desc);
}

void parse_element__disabled_ok(void **state) {
	optind = 0;
	char *argv[] = { "ONE", "TWO", };

	struct Cfg *actual = parse_element(CFG_SET, DISABLED, 2, argv);

	struct Cfg expected = { 0 };
	slist_append(&expected.disabled_name_desc, "ONE");
	slist_append(&expected.disabled_name_desc, "TWO");

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);

	slist_free(&expected.disabled_name_desc);
}

void parse_element__order_ok(void **state) {
	optind = 0;
	char *argv[] = { "ONE", "TWO", };

	struct Cfg *actual = parse_element(CFG_SET, ORDER, 2, argv);

	struct Cfg expected = { 0 };
	slist_append(&expected.order_name_desc, "ONE");
	slist_append(&expected.order_name_desc, "TWO");

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);

	slist_free(&expected.order_name_desc);
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

	ipc_request_free(request);
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

void parse_set__adaptive_sync_off_nargs(void **state) {
	optind = 0;
	optarg = "VRR_OFF";

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
	char *argv[] = { "arg0", };

	optarg = "DISABLED";

	struct IpcRequest *request = parse_set(1, argv);

	assert_non_null(request);
	assert_int_equal(request->op, CFG_SET);

	ipc_request_free(request);
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
	char *argv[] = { "arg0", };

	optarg = "MODE";

	struct IpcRequest *request = parse_del(1, argv);

	assert_non_null(request);
	assert_int_equal(request->op, CFG_DEL);

	ipc_request_free(request);
}

void parse_log_threshold__invalid(void **state) {
	expect_log_error("invalid --log-threshold %s", "INVALID", NULL, NULL, NULL);

	assert_false(parse_log_threshold("INVALID"));
}

void parse_log_threshold__ok(void **state) {
	expect_value(__wrap_log_set_threshold, threshold, WARNING);
	expect_value(__wrap_log_set_threshold, cli, true);

	assert_true(parse_log_threshold("WARNING"));
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(parse_element__arrange_align_invalid_arrange),
		TEST(parse_element__arrange_align_invalid_align),
		TEST(parse_element__arrange_align_ok),

		TEST(parse_element__auto_scale_invalid),
		TEST(parse_element__auto_scale_ok),

		TEST(parse_element__scale_set_invalid),
		TEST(parse_element__scale_set_ok),
		TEST(parse_element__scale_del_ok),

		TEST(parse_element__mode_set_invalid_width),
		TEST(parse_element__mode_set_invalid_height),
		TEST(parse_element__mode_set_invalid_refresh),
		TEST(parse_element__mode_set_max),
		TEST(parse_element__mode_set_res),
		TEST(parse_element__mode_set_res_refresh),
		TEST(parse_element__mode_del_ok),

		TEST(parse_element__adaptive_sync_off_ok),

		TEST(parse_element__disabled_ok),

		TEST(parse_element__order_ok),

		TEST(parse_write__nargs),
		TEST(parse_write__ok),

		TEST(parse_set__mode_nargs),
		TEST(parse_set__arrange_align_nargs),
		TEST(parse_set__scale_nargs),
		TEST(parse_set__auto_scale_nargs),
		TEST(parse_set__disabled_nargs),
		TEST(parse_set__adaptive_sync_off_nargs),
		TEST(parse_set__order_nargs),
		TEST(parse_set__invalid),
		TEST(parse_set__ok),

		TEST(parse_del__mode_nargs),
		TEST(parse_del__scale_nargs),
		TEST(parse_del__disabled_nargs),
		TEST(parse_del__invalid),
		TEST(parse_del__ok),

		TEST(parse_log_threshold__invalid),
		TEST(parse_log_threshold__ok),
	};

	return RUN(tests);
}


#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client-protocol.h>

#include "cli.h"
#include "cfg.h"
#include "ipc.h"
#include "slist.h"
#include "log.h"

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
	assert_logs_empty();

	return 0;
}


void parse_element__arrange_align_invalid_arrange(void **state) {
	optind = 0;
	char *argv[] = { "ROW", "INVALID" };

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, ARRANGE_ALIGN, 2, argv));

	assert_log(FATAL, "invalid ARRANGE_ALIGN ROW INVALID\n");
}

void parse_element__arrange_align_invalid_align(void **state) {
	optind = 0;
	char *argv[] = { "INVALID", "LEFT" };

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, ARRANGE_ALIGN, 2, argv));

	assert_log(FATAL, "invalid ARRANGE_ALIGN INVALID LEFT\n");
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

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, AUTO_SCALE, 1, argv));

	assert_log(FATAL, "invalid AUTO_SCALE INVALID\n");
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

void parse_element__transform_invalid(void **state) {
	optind = 0;
	char *argv[] = { "displ", "INVALID", };

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, TRANSFORM, 2, argv));

	assert_log(FATAL, "invalid TRANSFORM displ INVALID\n");
}

void parse_element__transform_ok(void **state) {
	optind = 0;
	char *argv[] = { "displ", "flipped-270", };

	struct Cfg *actual = parse_element(CFG_SET, TRANSFORM, 2, argv);

	struct Cfg *expected = cfg_init();
	slist_append(&expected->user_transforms, cfg_user_transform_init("displ", WL_OUTPUT_TRANSFORM_FLIPPED_270));

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);
}

void parse_element__transform_del_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", };

	struct Cfg *actual = parse_element(CFG_DEL, TRANSFORM, 1, argv);

	struct UserTransform expectedUserTransform = {
		.name_desc = "DISPL",
		.transform = WL_OUTPUT_TRANSFORM_90,
	};
	struct Cfg expected = { 0 };
	slist_append(&expected.user_transforms, &expectedUserTransform);

	assert_cfg_equal(actual, &expected);

	cfg_free(actual);

	slist_free(&expected.user_transforms);
}

void parse_element__scale_set_invalid(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "NOTANUMBER", };

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, SCALE, 2, argv));

	assert_log(FATAL, "invalid SCALE DISPL NOTANUMBER\n");
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

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, MODE, 4, argv));

	assert_log(FATAL, "invalid MODE DISPL NAN 2 3\n");
}

void parse_element__mode_set_invalid_height(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "NAN", "3", };

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, MODE, 4, argv));

	assert_log(FATAL, "invalid MODE DISPL 1 NAN 3\n");
}

void parse_element__mode_set_invalid_refresh(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "2", "NAN", };

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, MODE, 4, argv));

	assert_log(FATAL, "invalid MODE DISPL 1 2 NAN\n");
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
	char *argv[] = { "DISPL", "1", "2", "12.3456", };

	struct Cfg *actual = parse_element(CFG_SET, MODE, 4, argv);

	struct UserMode *expectedUserMode = cfg_user_mode_default();
	expectedUserMode->name_desc = strdup("DISPL");
	expectedUserMode->max = false;
	expectedUserMode->width = 1;
	expectedUserMode->height = 2;
	expectedUserMode->refresh_mhz = 12346;

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
	slist_free(&expected.adaptive_sync_off_name_desc);
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

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_write(1, NULL));

	assert_log(FATAL, "--write takes no arguments\n");
}

void parse_write__ok(void **state) {
	optind = 0;

	struct IpcRequest *request = parse_write(0, NULL);

	assert_non_nul(request);
	assert_int_equal(request->command, CFG_WRITE);

	ipc_request_free(request);
}

void parse_set__mode_nargs(void **state) {
	optind = 0;
	optarg = "MODE";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(1, NULL));

	assert_log(FATAL, "MODE requires two to four arguments\n");

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(5, NULL));

	assert_log(FATAL, "MODE requires two to four arguments\n");
}

void parse_set__arrange_align_nargs(void **state) {
	optind = 0;
	optarg = "ARRANGE_ALIGN";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "ARRANGE_ALIGN requires two arguments\n");
}

void parse_set__scale_nargs(void **state) {
	optind = 0;
	optarg = "SCALE";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "SCALE requires two arguments\n");
}

void parse_set__transform_nargs(void **state) {
	optind = 0;
	optarg = "TRANSFORM";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "TRANSFORM requires two arguments\n");
}

void parse_set__auto_scale_nargs(void **state) {
	optind = 0;
	optarg = "AUTO_SCALE";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "AUTO_SCALE requires one argument\n");
}

void parse_set__disabled_nargs(void **state) {
	optind = 0;
	optarg = "DISABLED";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "DISABLED requires one argument\n");
}

void parse_set__adaptive_sync_off_nargs(void **state) {
	optind = 0;
	optarg = "VRR_OFF";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "VRR_OFF requires one argument\n");
}

void parse_set__order_nargs(void **state) {
	optind = 0;
	optarg = "ORDER";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "ORDER requires at least one argument\n");
}

void parse_set__invalid(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "invalid set: INVALID\n");
}

void parse_set__ok(void **state) {
	optind = 0;
	char *argv[] = { "arg0", };

	optarg = "DISABLED";

	struct IpcRequest *request = parse_set(1, argv);

	assert_non_nul(request);
	assert_int_equal(request->command, CFG_SET);

	ipc_request_free(request);
}

void parse_del__mode_nargs(void **state) {
	optind = 0;
	optarg = "MODE";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_del(0, NULL));

	assert_log(FATAL, "MODE requires one argument\n");
}

void parse_del__scale_nargs(void **state) {
	optind = 0;
	optarg = "SCALE";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_del(0, NULL));

	assert_log(FATAL, "SCALE requires one argument\n");
}

void parse_del__disabled_nargs(void **state) {
	optind = 0;
	optarg = "DISABLED";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_del(0, NULL));

	assert_log(FATAL, "DISABLED requires one argument\n");
}

void parse_del__adaptive_sync_off_nargs(void **state) {
	optind = 0;
	optarg = "VRR_OFF";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_del(0, NULL));

	assert_log(FATAL, "VRR_OFF requires one argument\n");
}

void parse_del__invalid(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_del(0, NULL));

	assert_log(FATAL, "invalid delete: INVALID\n");
}

void parse_del__ok(void **state) {
	optind = 0;
	char *argv[] = { "arg0", };

	optarg = "MODE";

	struct IpcRequest *request = parse_del(1, argv);

	assert_non_nul(request);
	assert_int_equal(request->command, CFG_DEL);

	ipc_request_free(request);
}

void parse_log_threshold__invalid(void **state) {
	assert_int_equal(parse_log_threshold("INVALID"), 0);

	assert_log(FATAL, "invalid --log-threshold INVALID\n");
}

void parse_log_threshold__ok(void **state) {
	assert_int_equal(parse_log_threshold("WARNING"), WARNING);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(parse_element__arrange_align_invalid_arrange),
		TEST(parse_element__arrange_align_invalid_align),
		TEST(parse_element__arrange_align_ok),

		TEST(parse_element__auto_scale_invalid),
		TEST(parse_element__auto_scale_ok),

		TEST(parse_element__transform_invalid),
		TEST(parse_element__transform_ok),
		TEST(parse_element__transform_del_ok),

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
		TEST(parse_set__transform_nargs),
		TEST(parse_set__auto_scale_nargs),
		TEST(parse_set__disabled_nargs),
		TEST(parse_set__adaptive_sync_off_nargs),
		TEST(parse_set__order_nargs),
		TEST(parse_set__invalid),
		TEST(parse_set__ok),

		TEST(parse_del__mode_nargs),
		TEST(parse_del__scale_nargs),
		TEST(parse_del__disabled_nargs),
		TEST(parse_del__adaptive_sync_off_nargs),
		TEST(parse_del__invalid),
		TEST(parse_del__ok),

		TEST(parse_log_threshold__invalid),
		TEST(parse_log_threshold__ok),
	};

	return RUN(tests);
}


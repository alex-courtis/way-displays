#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "asserts.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client-protocol.h>

#include "cfg/cfg.h"
#include "cfg/disabled.h"
#include "enum.h"
#include "ipc.h"
#include "mode.h"
#include "spmap.h"
#include "simap.h"

#include "args.h"

static void args_cfg__arrange_align_invalid_arrange(void **state) {
	optind = 0;
	char *argv[] = { "ROW", "INVALID" };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_cfg(CFG_SET, ARRANGE_ALIGN, 2, argv));

	assert_log(FATAL, "invalid ARRANGE_ALIGN ROW INVALID\n");

	assert_logs_empty();
}

static void args_cfg__arrange_align_invalid_align(void **state) {
	optind = 0;
	char *argv[] = { "INVALID", "LEFT" };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_cfg(CFG_SET, ARRANGE_ALIGN, 2, argv));

	assert_log(FATAL, "invalid ARRANGE_ALIGN INVALID LEFT\n");

	assert_logs_empty();
}

static void args_cfg__arrange_align_ok(void **state) {
	optind = 0;
	char *argv[] = { "ROW", "LEFT" };

	struct Cfg *actual = args_cfg(CFG_SET, ARRANGE_ALIGN, 2, argv);

	struct Cfg *expected = cfg_init();
	expected->arrange = ROW;
	expected->align = LEFT;

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__auto_scale_invalid(void **state) {
	optind = 0;
	char *argv[] = { "INVALID", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_cfg(CFG_SET, AUTO_SCALE, 1, argv));

	assert_log(FATAL, "invalid AUTO_SCALE INVALID\n");

	assert_logs_empty();
}

static void args_cfg__auto_scale_set(void **state) {
	optind = 0;
	char *argv[] = { "ON", };

	struct Cfg *actual = args_cfg(CFG_SET, AUTO_SCALE, 1, argv);

	struct Cfg *expected = cfg_init();
	expected->auto_scale = ON;

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__auto_scale_toggle(void **state) {
	optind = 0;
	char *argv[] = { 0 };

	struct Cfg *actual = args_cfg(CFG_TOGGLE, AUTO_SCALE, 0, argv);

	struct Cfg *expected = cfg_init();
	expected->auto_scale = ON;

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__scaling_invalid(void **state) {
	optind = 0;
	char *argv[] = { "INVALID", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_cfg(CFG_SET, SCALING, 1, argv));

	assert_log(FATAL, "invalid SCALING INVALID\n");

	assert_logs_empty();
}

static void args_cfg__scaling_set(void **state) {
	optind = 0;
	char *argv[] = { "OFF", };

	struct Cfg *actual = args_cfg(CFG_SET, SCALING, 1, argv);

	struct Cfg *expected = cfg_init();
	expected->scaling = OFF;

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__scaling_toggle(void **state) {
	optind = 0;
	char *argv[] = { 0 };

	struct Cfg *actual = args_cfg(CFG_TOGGLE, SCALING, 1, argv);

	struct Cfg *expected = cfg_init();
	expected->scaling = ON;

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__transform_invalid(void **state) {
	optind = 0;
	char *argv[] = { "displ", "INVALID", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_cfg(CFG_SET, TRANSFORM, 2, argv));

	assert_log(FATAL, "invalid TRANSFORM displ INVALID\n");

	assert_logs_empty();
}

static void args_cfg__transform_ok(void **state) {
	optind = 0;
	char *argv[] = { "displ", "flipped-270", };

	struct Cfg *actual = args_cfg(CFG_SET, TRANSFORM, 2, argv);

	struct Cfg *expected = cfg_init();
	simap_put(expected->transforms, "displ", WL_OUTPUT_TRANSFORM_FLIPPED_270);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__transform_del_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", };

	struct Cfg *actual = args_cfg(CFG_DEL, TRANSFORM, 1, argv);

	struct Cfg *expected = cfg_init();
	simap_put(expected->transforms, "DISPL", WL_OUTPUT_TRANSFORM_90);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__scale_set_invalid(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "NOTANUMBER", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_cfg(CFG_SET, SCALE, 2, argv));

	assert_log(FATAL, "invalid SCALE DISPL NOTANUMBER\n");

	assert_logs_empty();
}

static void args_cfg__scale_set_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1234.5" };

	struct Cfg *actual = args_cfg(CFG_SET, SCALE, 2, argv);

	struct Cfg *expected = cfg_init();
	simap_put(expected->scales, "DISPL", 1234500);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__scale_del_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", };

	struct Cfg *actual = args_cfg(CFG_DEL, SCALE, 1, argv);

	struct Cfg *expected = cfg_init();
	simap_put(expected->scales, "DISPL", 1);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__mode_set_invalid_width(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "NAN", "2", "3", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_cfg(CFG_SET, MODE, 4, argv));

	assert_log(FATAL, "invalid MODE DISPL NAN 2 3\n");

	assert_logs_empty();
}

static void args_cfg__mode_set_invalid_height(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "NAN", "3", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_cfg(CFG_SET, MODE, 4, argv));

	assert_log(FATAL, "invalid MODE DISPL 1 NAN 3\n");

	assert_logs_empty();
}

static void args_cfg__mode_set_invalid_refresh(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "2", "NAN", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_cfg(CFG_SET, MODE, 4, argv));

	assert_log(FATAL, "invalid MODE DISPL 1 2 NAN\n");

	assert_logs_empty();
}

static void args_cfg__mode_set_max(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "MAX" };

	struct Cfg *actual = args_cfg(CFG_SET, MODE, 2, argv);

	struct Mode *expectedUserMode = mode_init();
	expectedUserMode->max = true;

	struct Cfg *expected = cfg_init();
	spmap_put(expected->modes, "DISPL", expectedUserMode);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__mode_set_res(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "2" };

	struct Cfg *actual = args_cfg(CFG_SET, MODE, 3, argv);

	struct Mode *expectedUserMode = mode_init();
	expectedUserMode->max = false;
	expectedUserMode->width = 1;
	expectedUserMode->height = 2;

	struct Cfg *expected = cfg_init();
	spmap_put(expected->modes, "DISPL", expectedUserMode);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__mode_set_res_refresh(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "2", "12.3456", };

	struct Cfg *actual = args_cfg(CFG_SET, MODE, 4, argv);

	struct Mode *expectedUserMode = mode_init();
	expectedUserMode->max = false;
	expectedUserMode->width = 1;
	expectedUserMode->height = 2;
	expectedUserMode->refresh_mhz = 12346;

	struct Cfg *expected = cfg_init();
	spmap_put(expected->modes, "DISPL", expectedUserMode);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__callback_cmd_set_ok(void **state) {
	optind = 0;
	char *argv[] = { "foo bar\nbaz", };

	struct Cfg *actual = args_cfg(CFG_SET, CALLBACK_CMD, 2, argv);

	struct Cfg *expected = cfg_init();
	expected->callback_cmd = strdup(argv[0]);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__callback_cmd_del_ok(void **state) {
	optind = 0;
	char *argv[] = { 0 };

	struct Cfg *actual = args_cfg(CFG_DEL, CALLBACK_CMD, 0, argv);

	struct Cfg *expected = cfg_init();
	expected->callback_cmd = strdup("");

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__mode_del_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", };

	struct Cfg *actual = args_cfg(CFG_DEL, MODE, 1, argv);

	struct Mode *expectedUserMode = mode_init();
	expectedUserMode->max = true;

	struct Cfg *expected = cfg_init();
	spmap_put(expected->modes, "DISPL", expectedUserMode);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__adaptive_sync_off_ok(void **state) {
	optind = 0;
	char *argv[] = { "ONE", "TWO", };

	struct Cfg *actual = args_cfg(CFG_SET, VRR_OFF, 2, argv);

	struct Cfg *expected = cfg_init();
	sset_add_many(expected->adaptive_sync_off, "ONE", "TWO", NULL);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__disabled_ok(void **state) {
	optind = 0;
	char *argv[] = { "ONE", "TWO", };

	struct Cfg *actual = args_cfg(CFG_SET, DISABLED, 2, argv);

	struct Cfg *expected = cfg_init();
	spmap_put_many(expected->disableds,
			"ONE", cfg_disabled_init(),
			"TWO", cfg_disabled_init(),
			NULL);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_cfg__order_ok(void **state) {
	optind = 0;
	char *argv[] = { "ONE", "TWO", };

	struct Cfg *actual = args_cfg(CFG_SET, ORDER, 2, argv);

	struct Cfg *expected = cfg_init();
	sset_add_many(expected->order_name_desc, "ONE", "TWO", NULL);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void args_ipc_write__nargs(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_write(1));

	assert_log(FATAL, "--write takes no arguments\n");

	assert_logs_empty();
}

static void args_ipc_write__ok(void **state) {
	optind = 0;

	struct IpcRequest *request = args_ipc_write(0);

	assert_non_nul(request);
	assert_int_equal(request->command, CFG_WRITE);

	ipc_request_free(request);

	assert_logs_empty();
}

static void args_ipc_get__nargs(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_get(1));

	assert_log(FATAL, "--get takes no arguments\n");

	assert_logs_empty();
}

static void args_ipc_get__ok(void **state) {
	optind = 0;

	struct IpcRequest *request = args_ipc_get(0);

	assert_non_nul(request);
	assert_int_equal(request->command, GET);

	ipc_request_free(request);

	assert_logs_empty();
}

static void args_ipc_list__nargs(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_list(1));

	assert_log(FATAL, "--list takes no arguments\n");

	assert_logs_empty();
}

static void args_ipc_list__ok(void **state) {
	optind = 0;

	struct IpcRequest *request = args_ipc_list(0);

	assert_non_nul(request);
	assert_int_equal(request->command, LIST);

	ipc_request_free(request);

	assert_logs_empty();
}

static void args_ipc_reapply__nargs(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_reapply(1));

	assert_log(FATAL, "--reapply takes no arguments\n");

	assert_logs_empty();
}

static void args_ipc_reapply__ok(void **state) {
	optind = 0;

	struct IpcRequest *request = args_ipc_reapply(0);

	assert_non_nul(request);
	assert_int_equal(request->command, REAPPLY);

	ipc_request_free(request);

	assert_logs_empty();
}

static void args_ipc_set__mode_nargs(void **state) {
	optind = 0;
	optarg = "MODE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_set(1, NULL));

	assert_log(FATAL, "--set MODE requires two to four arguments\n");

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_set(5, NULL));

	assert_log(FATAL, "--set MODE requires two to four arguments\n");

	assert_logs_empty();
}

static void args_ipc_set__arrange_align_nargs(void **state) {
	optind = 0;
	optarg = "ARRANGE_ALIGN";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_set(0, NULL));

	assert_log(FATAL, "--set ARRANGE_ALIGN requires two arguments\n");

	assert_logs_empty();
}

static void args_ipc_set__scale_nargs(void **state) {
	optind = 0;
	optarg = "SCALE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_set(0, NULL));

	assert_log(FATAL, "--set SCALE requires two arguments\n");

	assert_logs_empty();
}

static void args_ipc_set__transform_nargs(void **state) {
	optind = 0;
	optarg = "TRANSFORM";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_set(0, NULL));

	assert_log(FATAL, "--set TRANSFORM requires two arguments\n");

	assert_logs_empty();
}

static void args_ipc_set__auto_scale_nargs(void **state) {
	optind = 0;
	optarg = "AUTO_SCALE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_set(0, NULL));

	assert_log(FATAL, "--set AUTO_SCALE requires one argument\n");

	assert_logs_empty();
}

static void args_ipc_set__disabled_nargs(void **state) {
	optind = 0;
	optarg = "DISABLED";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_set(0, NULL));

	assert_log(FATAL, "--set DISABLED requires one argument\n");

	assert_logs_empty();
}

static void args_ipc_set__adaptive_sync_off_nargs(void **state) {
	optind = 0;
	optarg = "VRR_OFF";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_set(0, NULL));

	assert_log(FATAL, "--set VRR_OFF requires one argument\n");

	assert_logs_empty();
}

static void args_ipc_set__order_nargs(void **state) {
	optind = 0;
	optarg = "ORDER";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_set(0, NULL));

	assert_log(FATAL, "--set ORDER requires at least one argument\n");

	assert_logs_empty();
}

static void args_ipc_set__invalid(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_set(0, NULL));

	assert_log(FATAL, "invalid --set: INVALID\n");

	assert_logs_empty();
}

static void args_ipc_set__ok(void **state) {
	optind = 0;
	char *argv[] = { "arg0", };

	optarg = "DISABLED";

	struct IpcRequest *request = args_ipc_set(1, argv);

	assert_non_nul(request);
	assert_int_equal(request->command, CFG_SET);

	ipc_request_free(request);

	assert_logs_empty();
}

static void args_ipc_del__mode_nargs(void **state) {
	optind = 0;
	optarg = "MODE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_del(0, NULL));

	assert_log(FATAL, "--delete MODE requires one argument\n");

	assert_logs_empty();
}

static void args_ipc_del__scale_nargs(void **state) {
	optind = 0;
	optarg = "SCALE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_del(0, NULL));

	assert_log(FATAL, "--delete SCALE requires one argument\n");

	assert_logs_empty();
}

static void args_ipc_del__disabled_nargs(void **state) {
	optind = 0;
	optarg = "DISABLED";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_del(0, NULL));

	assert_log(FATAL, "--delete DISABLED requires one argument\n");

	assert_logs_empty();
}

static void args_ipc_del__adaptive_sync_off_nargs(void **state) {
	optind = 0;
	optarg = "VRR_OFF";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_del(0, NULL));

	assert_log(FATAL, "--delete VRR_OFF requires one argument\n");

	assert_logs_empty();
}

static void args_ipc_del__callback_cmd_nargs(void **state) {
	optind = 0;
	optarg = "CALLBACK_CMD";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_del(1, NULL));

	assert_log(FATAL, "--delete CALLBACK_CMD takes no arguments\n");

	assert_logs_empty();
}

static void args_ipc_del__invalid(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_del(0, NULL));

	assert_log(FATAL, "invalid --delete: INVALID\n");

	assert_logs_empty();
}

static void args_ipc_del__ok(void **state) {
	optind = 0;
	char *argv[] = { "arg0", };

	optarg = "MODE";

	struct IpcRequest *request = args_ipc_del(1, argv);

	assert_non_nul(request);
	assert_int_equal(request->command, CFG_DEL);

	ipc_request_free(request);

	assert_logs_empty();
}

static void args_ipc_toggle__scaling_nargs(void **state) {
	optind = 0;
	optarg = "SCALING";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_toggle(1, NULL));

	assert_log(FATAL, "--toggle SCALING takes no arguments\n");

	assert_logs_empty();
}

static void args_ipc_toggle__auto_scale_nargs(void **state) {
	optind = 0;
	optarg = "AUTO_SCALE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_toggle(1, NULL));

	assert_log(FATAL, "--toggle AUTO_SCALE takes no arguments\n");

	assert_logs_empty();
}

static void args_ipc_toggle__vrr_off_nargs(void **state) {
	optind = 0;
	optarg = "VRR_OFF";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_toggle(0, NULL));

	assert_log(FATAL, "--toggle VRR_OFF requires one argument\n");

	assert_logs_empty();
}

static void args_ipc_toggle__disabled_nargs(void **state) {
	optind = 0;
	optarg = "DISABLED";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_toggle(0, NULL));

	assert_log(FATAL, "--toggle DISABLED requires one argument\n");

	assert_logs_empty();
}

static void args_ipc_toggle__invalid(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(args_ipc_toggle(0, NULL));

	assert_log(FATAL, "invalid --toggle: INVALID\n");

	assert_logs_empty();
}

static void args_ipc_toggle__ok(void **state) {
	optind = 0;

	optarg = "SCALING";

	struct IpcRequest *request = args_ipc_toggle(0, NULL);

	assert_non_nul(request);
	assert_int_equal(request->command, CFG_TOGGLE);

	ipc_request_free(request);

	assert_logs_empty();
}

static void args_log_threshold__invalid(void **state) {
	assert_int_equal(args_log_threshold("INVALID"), 0);

	assert_log(FATAL, "invalid --log-threshold INVALID\n");

	assert_logs_empty();
}

static void args_log_threshold__ok(void **state) {
	assert_int_equal(args_log_threshold("WARNING"), WARNING);

	assert_logs_empty();
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(args_cfg__arrange_align_invalid_arrange),
		TEST(args_cfg__arrange_align_invalid_align),
		TEST(args_cfg__arrange_align_ok),

		TEST(args_cfg__auto_scale_invalid),
		TEST(args_cfg__auto_scale_set),
		TEST(args_cfg__auto_scale_toggle),

		TEST(args_cfg__scaling_invalid),
		TEST(args_cfg__scaling_set),
		TEST(args_cfg__scaling_toggle),

		TEST(args_cfg__transform_invalid),
		TEST(args_cfg__transform_ok),
		TEST(args_cfg__transform_del_ok),

		TEST(args_cfg__scale_set_invalid),
		TEST(args_cfg__scale_set_ok),
		TEST(args_cfg__scale_del_ok),

		TEST(args_cfg__mode_set_invalid_width),
		TEST(args_cfg__mode_set_invalid_height),
		TEST(args_cfg__mode_set_invalid_refresh),
		TEST(args_cfg__mode_set_max),
		TEST(args_cfg__mode_set_res),
		TEST(args_cfg__mode_set_res_refresh),
		TEST(args_cfg__mode_del_ok),

		TEST(args_cfg__callback_cmd_set_ok),
		TEST(args_cfg__callback_cmd_del_ok),

		TEST(args_cfg__adaptive_sync_off_ok),

		TEST(args_cfg__disabled_ok),

		TEST(args_cfg__order_ok),

		TEST(args_ipc_write__nargs),
		TEST(args_ipc_write__ok),

		TEST(args_ipc_get__nargs),
		TEST(args_ipc_get__ok),

		TEST(args_ipc_list__nargs),
		TEST(args_ipc_list__ok),

		TEST(args_ipc_reapply__nargs),
		TEST(args_ipc_reapply__ok),

		TEST(args_ipc_set__mode_nargs),
		TEST(args_ipc_set__arrange_align_nargs),
		TEST(args_ipc_set__scale_nargs),
		TEST(args_ipc_set__transform_nargs),
		TEST(args_ipc_set__auto_scale_nargs),
		TEST(args_ipc_set__disabled_nargs),
		TEST(args_ipc_set__adaptive_sync_off_nargs),
		TEST(args_ipc_set__order_nargs),
		TEST(args_ipc_set__invalid),
		TEST(args_ipc_set__ok),

		TEST(args_ipc_del__mode_nargs),
		TEST(args_ipc_del__scale_nargs),
		TEST(args_ipc_del__disabled_nargs),
		TEST(args_ipc_del__adaptive_sync_off_nargs),
		TEST(args_ipc_del__callback_cmd_nargs),
		TEST(args_ipc_del__invalid),
		TEST(args_ipc_del__ok),

		TEST(args_ipc_toggle__scaling_nargs),
		TEST(args_ipc_toggle__auto_scale_nargs),
		TEST(args_ipc_toggle__vrr_off_nargs),
		TEST(args_ipc_toggle__disabled_nargs),
		TEST(args_ipc_toggle__invalid),
		TEST(args_ipc_toggle__ok),

		TEST(args_log_threshold__invalid),
		TEST(args_log_threshold__ok),
	};

	return RUN(tests);
}


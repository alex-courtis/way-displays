#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-client-protocol.h>

#include "cfg.h"
#include "cfg/disabled.h"
#include "cfg/user-mode.h"
#include "ipc.h"
#include "log.h"
#include "pset.h"
#include "smap.h"
#include "smapi.h"
#include "sset.h"

struct Cfg *parse_element(enum IpcCommand command, enum CfgElement element, int argc, char **argv);
struct IpcRequest *parse_write(int argc, char **argv);
struct IpcRequest *parse_reapply(int argc, char **argv);
struct IpcRequest *parse_set(int argc, char **argv);
struct IpcRequest *parse_del(int argc, char **argv);
struct IpcRequest *parse_toggle(int argc, char **argv);
enum LogThreshold parse_log_threshold(char *optarg);


static void parse_element__arrange_align_invalid_arrange(void **state) {
	optind = 0;
	char *argv[] = { "ROW", "INVALID" };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, ARRANGE_ALIGN, 2, argv));

	assert_log(FATAL, "invalid ARRANGE_ALIGN ROW INVALID\n");
	assert_logs_empty();
}

static void parse_element__arrange_align_invalid_align(void **state) {
	optind = 0;
	char *argv[] = { "INVALID", "LEFT" };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, ARRANGE_ALIGN, 2, argv));

	assert_log(FATAL, "invalid ARRANGE_ALIGN INVALID LEFT\n");
	assert_logs_empty();
}

static void parse_element__arrange_align_ok(void **state) {
	optind = 0;
	char *argv[] = { "ROW", "LEFT" };

	struct Cfg *actual = parse_element(CFG_SET, ARRANGE_ALIGN, 2, argv);

	struct Cfg *expected = cfg_init();
	expected->arrange = ROW;
	expected->align = LEFT;

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__auto_scale_invalid(void **state) {
	optind = 0;
	char *argv[] = { "INVALID", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, AUTO_SCALE, 1, argv));

	assert_log(FATAL, "invalid AUTO_SCALE INVALID\n");
	assert_logs_empty();
}

static void parse_element__auto_scale_ok(void **state) {
	optind = 0;
	char *argv[] = { "ON", };

	struct Cfg *actual = parse_element(CFG_SET, AUTO_SCALE, 1, argv);

	struct Cfg *expected = cfg_init();
	expected->auto_scale = ON;

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__transform_invalid(void **state) {
	optind = 0;
	char *argv[] = { "displ", "INVALID", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, TRANSFORM, 2, argv));

	assert_log(FATAL, "invalid TRANSFORM displ INVALID\n");
	assert_logs_empty();
}

static void parse_element__transform_ok(void **state) {
	optind = 0;
	char *argv[] = { "displ", "flipped-270", };

	struct Cfg *actual = parse_element(CFG_SET, TRANSFORM, 2, argv);

	struct Cfg *expected = cfg_init();
	smapi_put(expected->transforms, "displ", WL_OUTPUT_TRANSFORM_FLIPPED_270);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__transform_del_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", };

	struct Cfg *actual = parse_element(CFG_DEL, TRANSFORM, 1, argv);

	struct Cfg *expected = cfg_init();
	smapi_put(expected->transforms, "DISPL", WL_OUTPUT_TRANSFORM_90);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__scale_set_invalid(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "NOTANUMBER", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, SCALE, 2, argv));

	assert_log(FATAL, "invalid SCALE DISPL NOTANUMBER\n");
	assert_logs_empty();
}

static void parse_element__scale_set_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1234.5" };

	struct Cfg *actual = parse_element(CFG_SET, SCALE, 2, argv);

	struct Cfg *expected = cfg_init();
	smapi_put(expected->scales, "DISPL", 1234500);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__scale_del_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", };

	struct Cfg *actual = parse_element(CFG_DEL, SCALE, 1, argv);

	struct Cfg *expected = cfg_init();
	smapi_put(expected->scales, "DISPL", 1);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__mode_set_invalid_width(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "NAN", "2", "3", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, MODE, 4, argv));

	assert_log(FATAL, "invalid MODE DISPL NAN 2 3\n");
	assert_logs_empty();
}

static void parse_element__mode_set_invalid_height(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "NAN", "3", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, MODE, 4, argv));

	assert_log(FATAL, "invalid MODE DISPL 1 NAN 3\n");
	assert_logs_empty();
}

static void parse_element__mode_set_invalid_refresh(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "2", "NAN", };

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_element(CFG_SET, MODE, 4, argv));

	assert_log(FATAL, "invalid MODE DISPL 1 2 NAN\n");
	assert_logs_empty();
}

static void parse_element__mode_set_max(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "MAX" };

	struct Cfg *actual = parse_element(CFG_SET, MODE, 2, argv);

	struct UserMode *expectedUserMode = user_mode_init_default();
	expectedUserMode->max = true;

	struct Cfg *expected = cfg_init();
	smap_put(expected->user_modes, "DISPL", expectedUserMode);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__mode_set_res(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "2" };

	struct Cfg *actual = parse_element(CFG_SET, MODE, 3, argv);

	struct UserMode *expectedUserMode = user_mode_init_default();
	expectedUserMode->max = false;
	expectedUserMode->width = 1;
	expectedUserMode->height = 2;

	struct Cfg *expected = cfg_init();
	smap_put(expected->user_modes, "DISPL", expectedUserMode);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__mode_set_res_refresh(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", "1", "2", "12.3456", };

	struct Cfg *actual = parse_element(CFG_SET, MODE, 4, argv);

	struct UserMode *expectedUserMode = user_mode_init_default();
	expectedUserMode->max = false;
	expectedUserMode->width = 1;
	expectedUserMode->height = 2;
	expectedUserMode->refresh_mhz = 12346;

	struct Cfg *expected = cfg_init();
	smap_put(expected->user_modes, "DISPL", expectedUserMode);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__mode_del_ok(void **state) {
	optind = 0;
	char *argv[] = { "DISPL", };

	struct Cfg *actual = parse_element(CFG_DEL, MODE, 1, argv);

	struct UserMode *expectedUserMode = user_mode_init_default();
	expectedUserMode->max = true;

	struct Cfg *expected = cfg_init();
	smap_put(expected->user_modes, "DISPL", expectedUserMode);

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__adaptive_sync_off_ok(void **state) {
	optind = 0;
	char *argv[] = { "ONE", "TWO", };

	struct Cfg *actual = parse_element(CFG_SET, VRR_OFF, 2, argv);

	struct Cfg *expected = cfg_init();
	sset_add(expected->adaptive_sync_off, "ONE");
	sset_add(expected->adaptive_sync_off, "TWO");

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__disabled_ok(void **state) {
	optind = 0;
	char *argv[] = { "ONE", "TWO", };

	struct Cfg *actual = parse_element(CFG_SET, DISABLED, 2, argv);

	struct Cfg *expected = cfg_init();
	pset_add(expected->disableds, disabled_init_always("ONE"));
	pset_add(expected->disableds, disabled_init_always("TWO"));

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_element__order_ok(void **state) {
	optind = 0;
	char *argv[] = { "ONE", "TWO", };

	struct Cfg *actual = parse_element(CFG_SET, ORDER, 2, argv);

	struct Cfg *expected = cfg_init();
	sset_add(expected->order_name_desc, "ONE");
	sset_add(expected->order_name_desc, "TWO");

	assert_cfg_equal(actual, expected);

	cfg_free(actual);
	cfg_free(expected);

	assert_logs_empty();
}

static void parse_write__nargs(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_write(1, NULL));

	assert_log(FATAL, "--write takes no arguments\n");
	assert_logs_empty();
}

static void parse_write__ok(void **state) {
	optind = 0;

	struct IpcRequest *request = parse_write(0, NULL);

	assert_non_nul(request);
	assert_int_equal(request->command, CFG_WRITE);

	ipc_request_free(request);

	assert_logs_empty();
}

static void parse_reapply__nargs(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_reapply(1, NULL));

	assert_log(FATAL, "--reapply takes no arguments\n");
	assert_logs_empty();
}

static void parse_reapply__ok(void **state) {
	optind = 0;

	struct IpcRequest *request = parse_reapply(0, NULL);

	assert_non_nul(request);
	assert_int_equal(request->command, REAPPLY);

	ipc_request_free(request);

	assert_logs_empty();
}

static void parse_set__mode_nargs(void **state) {
	optind = 0;
	optarg = "MODE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(1, NULL));

	assert_log(FATAL, "MODE requires two to four arguments\n");
	assert_logs_empty();

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(5, NULL));

	assert_log(FATAL, "MODE requires two to four arguments\n");
	assert_logs_empty();
}

static void parse_set__arrange_align_nargs(void **state) {
	optind = 0;
	optarg = "ARRANGE_ALIGN";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "ARRANGE_ALIGN requires two arguments\n");
	assert_logs_empty();
}

static void parse_set__scale_nargs(void **state) {
	optind = 0;
	optarg = "SCALE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "SCALE requires two arguments\n");
	assert_logs_empty();
}

static void parse_set__transform_nargs(void **state) {
	optind = 0;
	optarg = "TRANSFORM";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "TRANSFORM requires two arguments\n");
	assert_logs_empty();
}

static void parse_set__auto_scale_nargs(void **state) {
	optind = 0;
	optarg = "AUTO_SCALE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "AUTO_SCALE requires one argument\n");
	assert_logs_empty();
}

static void parse_set__disabled_nargs(void **state) {
	optind = 0;
	optarg = "DISABLED";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "DISABLED requires one argument\n");
	assert_logs_empty();
}

static void parse_set__adaptive_sync_off_nargs(void **state) {
	optind = 0;
	optarg = "VRR_OFF";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "VRR_OFF requires one argument\n");
	assert_logs_empty();
}

static void parse_set__order_nargs(void **state) {
	optind = 0;
	optarg = "ORDER";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "ORDER requires at least one argument\n");
	assert_logs_empty();
}

static void parse_set__invalid(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_set(0, NULL));

	assert_log(FATAL, "invalid set: INVALID\n");
	assert_logs_empty();
}

static void parse_set__ok(void **state) {
	optind = 0;
	char *argv[] = { "arg0", };

	optarg = "DISABLED";

	struct IpcRequest *request = parse_set(1, argv);

	assert_non_nul(request);
	assert_int_equal(request->command, CFG_SET);

	ipc_request_free(request);

	assert_logs_empty();
}

static void parse_del__mode_nargs(void **state) {
	optind = 0;
	optarg = "MODE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_del(0, NULL));

	assert_log(FATAL, "MODE requires one argument\n");
	assert_logs_empty();
}

static void parse_del__scale_nargs(void **state) {
	optind = 0;
	optarg = "SCALE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_del(0, NULL));

	assert_log(FATAL, "SCALE requires one argument\n");
	assert_logs_empty();
}

static void parse_del__disabled_nargs(void **state) {
	optind = 0;
	optarg = "DISABLED";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_del(0, NULL));

	assert_log(FATAL, "DISABLED requires one argument\n");
	assert_logs_empty();
}

static void parse_del__adaptive_sync_off_nargs(void **state) {
	optind = 0;
	optarg = "VRR_OFF";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_del(0, NULL));

	assert_log(FATAL, "VRR_OFF requires one argument\n");
	assert_logs_empty();
}

static void parse_del__invalid(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_del(0, NULL));

	assert_log(FATAL, "invalid delete: INVALID\n");
	assert_logs_empty();
}

static void parse_del__ok(void **state) {
	optind = 0;
	char *argv[] = { "arg0", };

	optarg = "MODE";

	struct IpcRequest *request = parse_del(1, argv);

	assert_non_nul(request);
	assert_int_equal(request->command, CFG_DEL);

	ipc_request_free(request);

	assert_logs_empty();
}

static void parse_toggle__scaling_nargs(void **state) {
	optind = 0;
	optarg = "SCALING";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_toggle(1, NULL));

	assert_log(FATAL, "SCALING takes no arguments\n");
	assert_logs_empty();
}

static void parse_toggle__auto_scale_nargs(void **state) {
	optind = 0;
	optarg = "AUTO_SCALE";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_toggle(1, NULL));

	assert_log(FATAL, "AUTO_SCALE takes no arguments\n");
	assert_logs_empty();
}

static void parse_toggle__vrr_off_nargs(void **state) {
	optind = 0;
	optarg = "VRR_OFF";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_toggle(0, NULL));

	assert_log(FATAL, "VRR_OFF requires one argument\n");
	assert_logs_empty();
}

static void parse_toggle__disabled_nargs(void **state) {
	optind = 0;
	optarg = "DISABLED";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_toggle(0, NULL));

	assert_log(FATAL, "DISABLED requires one argument\n");
	assert_logs_empty();
}

static void parse_toggle__invalid(void **state) {
	optind = 0;
	optarg = "INVALID";

	expect_int_value(__wrap_wd_exit, __status, EXIT_FAILURE);

	assert_nul(parse_toggle(0, NULL));

	assert_log(FATAL, "invalid toggle: INVALID\n");
	assert_logs_empty();
}

static void parse_toggle__ok(void **state) {
	optind = 0;

	optarg = "SCALING";

	struct IpcRequest *request = parse_toggle(0, NULL);

	assert_non_nul(request);
	assert_int_equal(request->command, CFG_TOGGLE);

	ipc_request_free(request);

	assert_logs_empty();
}

static void parse_log_threshold__invalid(void **state) {
	assert_int_equal(parse_log_threshold("INVALID"), 0);

	assert_log(FATAL, "invalid --log-threshold INVALID\n");
	assert_logs_empty();
}

static void parse_log_threshold__ok(void **state) {
	assert_int_equal(parse_log_threshold("WARNING"), WARNING);

	assert_logs_empty();
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

		TEST(parse_reapply__nargs),
		TEST(parse_reapply__ok),

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

		TEST(parse_toggle__scaling_nargs),
		TEST(parse_toggle__auto_scale_nargs),
		TEST(parse_toggle__vrr_off_nargs),
		TEST(parse_toggle__disabled_nargs),
		TEST(parse_toggle__invalid),
		TEST(parse_toggle__ok),

		TEST(parse_log_threshold__invalid),
		TEST(parse_log_threshold__ok),
	};

	return RUN(tests);
}


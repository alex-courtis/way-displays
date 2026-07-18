#include "tst.h"

#include "assert-log.h"
#include "asserts.h"
#include "data.h"
#include "expect-ppmap.h"
#include "expects.h"
#include "util-col.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "cfg/cfg.h"
#include "displ.h"
#include "enum.h"
#include "head.h"
#include "mode.h"
#include "ppmap.h"
#include "wlr-output-management-unstable-v1.h"

#include "act.h"

extern int g_cancellation_retries;

static int before_each(void **state) {
	g_cfg = cfg_default();

	g_displ = displ_init();

	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	displ_free(g_displ);

	g_cfg_destroy();

	return 0;
}

static void act_apply__no_heads(void **state) {
	act_apply();
}

static void act_apply__no_changes(void **state) {
	ppmap_put(g_displ->heads, H0, head_init());

	act_apply();

	assert_int_equal(g_displ->delta.element, 0);
	assert_nul(g_displ->delta.head);
	assert_nul(g_displ->delta.zmode);
	assert_nul(g_displ->delta.human);
	assert_int_equal(g_displ->state, IDLE);
}

static void act_apply__reapply(void **state) {
	struct zwlr_output_head_v1 *zhead = (struct zwlr_output_head_v1*)"dummy";
	struct zwlr_output_configuration_v1 *zconfig = (struct zwlr_output_configuration_v1*)"dummy";

	struct Head *head = head_init();
	head->reapply_required = true;
	ppmap_put(g_displ->heads, zhead, head);

	expect_ptr(__wrap_create_zwlr_output_config_listener, displ, g_displ);
	will_return_ptr_type(__wrap_create_zwlr_output_config_listener, zconfig, struct zwlr_output_configuration_v1*);

	expect_int_value(__wrap_print_head, t, INFO);
	expect_int_value(__wrap_print_head, event, DELTA);
	expect_ptr(__wrap_print_head, head, head);

	expect_ptr(__wrap__zwlr_output_configuration_v1_disable_head, zwlr_output_configuration_v1, zconfig);
	expect_ptr(__wrap__zwlr_output_configuration_v1_disable_head, head, zhead);

	expect_ptr(__wrap__zwlr_output_configuration_v1_apply, zwlr_output_configuration_v1, zconfig);

	act_apply();

	assert_int_equal(g_displ->delta.element, 0);
	assert_ptr_equal(g_displ->delta.head, head);
	assert_nul(g_displ->delta.zmode);
	assert_non_nul(g_displ->delta.human);
	assert_int_equal(g_displ->state, OUTSTANDING);

	displ_delta_destroy(g_displ);
}

static void act_apply__mode(void **state) {
	struct zwlr_output_head_v1 *zhead = (struct zwlr_output_head_v1*)"dummy1";
	struct zwlr_output_mode_v1 *zmode = (struct zwlr_output_mode_v1*)"dummy2";
	struct zwlr_output_configuration_v1 *zconfig = (struct zwlr_output_configuration_v1*)"dummy3";
	struct zwlr_output_configuration_head_v1 *zconfig_head = (struct zwlr_output_configuration_head_v1*)"dummy4";

	struct Head *head = head_init();
	ppmap_put(g_displ->heads, zhead, head);

	ppmap_put(head->modes, zmode, mode_init());
	head->des.zmode = zmode;

	expect_ptr(__wrap_create_zwlr_output_config_listener, displ, g_displ);
	will_return_ptr_type(__wrap_create_zwlr_output_config_listener, zconfig, struct zwlr_output_configuration_v1*);

	expect_int_value(__wrap_print_head, t, INFO);
	expect_int_value(__wrap_print_head, event, DELTA);
	expect_ptr(__wrap_print_head, head, head);

	expect_ptr(__wrap__zwlr_output_configuration_v1_enable_head, zwlr_output_configuration_v1, zconfig);
	expect_ptr(__wrap__zwlr_output_configuration_v1_enable_head, head, zhead);
	will_return_ptr_type(__wrap__zwlr_output_configuration_v1_enable_head, zconfig_head, struct zwlr_output_configuration_head_v1*);

	expect_ptr(__wrap__zwlr_output_configuration_head_v1_set_mode, zwlr_output_configuration_head_v1, zconfig_head);
	expect_ptr(__wrap__zwlr_output_configuration_head_v1_set_mode, mode, zmode);

	expect_ptr(__wrap__zwlr_output_configuration_v1_apply, zwlr_output_configuration_v1, zconfig);

	act_apply();

	assert_int_equal(g_displ->delta.element, MODE);
	assert_ptr_equal(g_displ->delta.head, head);
	assert_ptr_equal(g_displ->delta.zmode, zmode);
	assert_non_nul(g_displ->delta.human);
	assert_int_equal(g_displ->state, OUTSTANDING);

	displ_delta_destroy(g_displ);
}

static void act_apply__vrr(void **state) {
	struct zwlr_output_head_v1 *zhead = (struct zwlr_output_head_v1*)"dummy1";
	struct zwlr_output_configuration_v1 *zconfig = (struct zwlr_output_configuration_v1*)"dummy3";
	struct zwlr_output_configuration_head_v1 *zconfig_head = (struct zwlr_output_configuration_head_v1*)"dummy4";

	struct Head *head = head_init();
	head->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	ppmap_put(g_displ->heads, zhead, head);

	expect_ptr(__wrap_create_zwlr_output_config_listener, displ, g_displ);
	will_return_ptr_type(__wrap_create_zwlr_output_config_listener, zconfig, struct zwlr_output_configuration_v1*);

	expect_int_value(__wrap_print_head, t, INFO);
	expect_int_value(__wrap_print_head, event, DELTA);
	expect_ptr(__wrap_print_head, head, head);

	expect_ptr(__wrap__zwlr_output_configuration_v1_enable_head, zwlr_output_configuration_v1, zconfig);
	expect_ptr(__wrap__zwlr_output_configuration_v1_enable_head, head, zhead);
	will_return_ptr_type(__wrap__zwlr_output_configuration_v1_enable_head, zconfig_head, struct zwlr_output_configuration_head_v1*);

	expect_ptr(__wrap__zwlr_output_configuration_head_v1_set_adaptive_sync, zwlr_output_configuration_head_v1, zconfig_head);
	expect_int_value(__wrap__zwlr_output_configuration_head_v1_set_adaptive_sync, state, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED);

	expect_ptr(__wrap__zwlr_output_configuration_v1_apply, zwlr_output_configuration_v1, zconfig);

	act_apply();

	assert_int_equal(g_displ->delta.element, VRR_OFF);
	assert_ptr_equal(g_displ->delta.head, head);
	assert_nul(g_displ->delta.zmode);
	assert_non_nul(g_displ->delta.human);
	assert_int_equal(g_displ->state, OUTSTANDING);

	displ_delta_destroy(g_displ);
}

static void act_apply__disable(void **state) {
	struct zwlr_output_head_v1 *zhead = (struct zwlr_output_head_v1*)"dummy1";
	struct zwlr_output_configuration_v1 *zconfig = (struct zwlr_output_configuration_v1*)"dummy3";

	struct Head *head = head_init();
	head->cur.enabled = true;
	head->des.enabled = false;
	ppmap_put(g_displ->heads, zhead, head);

	expect_ptr(__wrap_create_zwlr_output_config_listener, displ, g_displ);
	will_return_ptr_type(__wrap_create_zwlr_output_config_listener, zconfig, struct zwlr_output_configuration_v1*);

	expect_int_value(__wrap_print_head_map, t, INFO);
	expect_int_value(__wrap_print_head_map, event, DELTA);
	expect_ppmap(__wrap_print_head_map, heads, g_displ->heads);

	expect_ptr(__wrap__zwlr_output_configuration_v1_disable_head, zwlr_output_configuration_v1, zconfig);
	expect_ptr(__wrap__zwlr_output_configuration_v1_disable_head, head, zhead);

	expect_ptr(__wrap__zwlr_output_configuration_v1_apply, zwlr_output_configuration_v1, zconfig);

	act_apply();

	assert_int_equal(g_displ->delta.element, 0);
	assert_nul(g_displ->delta.head);
	assert_nul(g_displ->delta.zmode);
	assert_non_nul(g_displ->delta.human);
	assert_int_equal(g_displ->state, OUTSTANDING);

	displ_delta_destroy(g_displ);
}

static void act_apply__remainder(void **state) {
	struct zwlr_output_head_v1 *zhead = (struct zwlr_output_head_v1*)"dummy1";
	struct zwlr_output_configuration_v1 *zconfig = (struct zwlr_output_configuration_v1*)"dummy3";
	struct zwlr_output_configuration_head_v1 *zconfig_head = (struct zwlr_output_configuration_head_v1*)"dummy4";

	struct Head *head = head_init();
	head->cur.enabled = true;
	head->des.enabled = true;
	head->des.transform = WL_OUTPUT_TRANSFORM_90;
	head->des.scale = 1;
	head->des.x = 2;
	head->des.y = 3;
	ppmap_put(g_displ->heads, zhead, head);

	expect_ptr(__wrap_create_zwlr_output_config_listener, displ, g_displ);
	will_return_ptr_type(__wrap_create_zwlr_output_config_listener, zconfig, struct zwlr_output_configuration_v1*);

	expect_int_value(__wrap_print_head_map, t, INFO);
	expect_int_value(__wrap_print_head_map, event, DELTA);
	expect_ppmap(__wrap_print_head_map, heads, g_displ->heads);

	expect_ptr(__wrap__zwlr_output_configuration_v1_enable_head, zwlr_output_configuration_v1, zconfig);
	expect_ptr(__wrap__zwlr_output_configuration_v1_enable_head, head, zhead);
	will_return_ptr_type(__wrap__zwlr_output_configuration_v1_enable_head, zconfig_head, struct zwlr_output_configuration_head_v1*);

	expect_ptr(__wrap__zwlr_output_configuration_head_v1_set_transform, zwlr_output_configuration_head_v1, zconfig_head);
	expect_int_value(__wrap__zwlr_output_configuration_head_v1_set_transform, transform, WL_OUTPUT_TRANSFORM_90);

	expect_ptr(__wrap__zwlr_output_configuration_head_v1_set_scale, zwlr_output_configuration_head_v1, zconfig_head);
	expect_int_value(__wrap__zwlr_output_configuration_head_v1_set_scale, scale, 1);

	expect_ptr(__wrap__zwlr_output_configuration_head_v1_set_position, zwlr_output_configuration_head_v1, zconfig_head);
	expect_int_value(__wrap__zwlr_output_configuration_head_v1_set_position, x, 2);
	expect_int_value(__wrap__zwlr_output_configuration_head_v1_set_position, y, 3);

	expect_ptr(__wrap__zwlr_output_configuration_v1_apply, zwlr_output_configuration_v1, zconfig);

	act_apply();

	assert_int_equal(g_displ->delta.element, 0);
	assert_nul(g_displ->delta.head);
	assert_nul(g_displ->delta.zmode);
	assert_non_nul(g_displ->delta.human);
	assert_int_equal(g_displ->state, OUTSTANDING);

	displ_delta_destroy(g_displ);
}

static void act_handle_success__head_changing_adaptive_sync(void **state) {
	struct Head *head = head_n("head");
	head->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head->cur.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head->adaptive_sync_failed = false;

	g_displ->delta.element = VRR_OFF;
	g_displ->delta.head = head;

	expect_int_value(__wrap_callback, t, INFO);
	expect_str(__wrap_callback, msg1, "Changes successful");
	expect_str(__wrap_callback, msg2, NULL);

	act_handle_success();

	assert_log(INFO, "\nChanges successful\n");

	assert_false(head->adaptive_sync_failed);

	head_free(head);
}

static void act_handle_success__head_changing_adaptive_sync_fail(void **state) {
	struct Head *head = head_n("head");
	head->model = NULL;
	head->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head->cur.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	g_displ->delta.element = VRR_OFF;
	g_displ->delta.head = head;

	expect_int_value(__wrap_print_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_print_adaptive_sync_fail, head, head);

	expect_int_value(__wrap_callback_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_callback_adaptive_sync_fail, head, head);

	act_handle_success();

	assert_true(head->adaptive_sync_failed);

	head_free(head);
}

static void act_handle_success__head_changing_mode(void **state) {
	struct zwlr_output_mode_v1 *zmode = (struct zwlr_output_mode_v1*)"dummy1";

	struct Head *head = head_n("head");

	const struct Mode *mode = mode_init();
	ppmap_put(head->modes, zmode, mode);
	head->des.zmode = zmode;

	g_displ->delta.element = MODE;
	g_displ->delta.head = head;
	g_displ->delta.zmode = zmode;

	expect_int_value(__wrap_callback, t, INFO);
	expect_str(__wrap_callback, msg1, "Changes successful");
	expect_str(__wrap_callback, msg2, NULL);

	act_handle_success();

	assert_log(INFO, "\nChanges successful\n");

	assert_ptr_equal(head->cur.zmode, zmode);

	head_free(head);
}

static void act_handle_success__ok(void **state) {
	g_displ->delta.human = strdup("human");

	expect_int_value(__wrap_callback, t, INFO);
	expect_str(__wrap_callback, msg1, "human");
	expect_str(__wrap_callback, msg2, NULL);

	act_handle_success();

	assert_log(INFO, "\nChanges successful\n");
}

static void act_handle_failure__mode(void **state) {
	struct Head *head = head_n("nam");

	struct Mode *mode_cur = mode_whr(1, 2, 3);

	struct Mode *mode_des = mode_whr(4, 5, 6);

	ppmap_put_many(head->modes,
			MC, mode_cur,
			MD, mode_des,
			NULL);

	head->cur.zmode = MC;
	head->des.zmode = MD;

	g_displ->delta.element = MODE;
	g_displ->delta.head = head;
	g_displ->delta.zmode = MD;

	expect_int_value(__wrap_print_mode_fail, t, ERROR);
	expect_ptr(__wrap_print_mode_fail, head, head);
	expect_ptr(__wrap_print_mode_fail, zmode, MD);

	expect_int_value(__wrap_callback_mode_fail, t, ERROR);
	expect_ptr(__wrap_callback_mode_fail, head, head);
	expect_ptr(__wrap_callback_mode_fail, zmode, MD);

	act_handle_failure();

	assert_nul(head->cur.zmode);

	assert_ptr_equal(head->des.zmode, MD);

	assert_int_equal(ppmap_size(head->modes), 1);
	assert_ptr_equal(ppmap_get(head->modes, MC), mode_cur);

	assert_int_equal(ppmap_size(head->modes_failed), 1);
	assert_ptr_equal(ppmap_get(head->modes_failed, MD), mode_des);

	head_free(head);
}

static void act_handle_failure__adaptive_sync(void **state) {
	struct Head *head = head_n("nam");
	head->model = strdup("mod");
	head->cur.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	head->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	g_displ->delta.element = VRR_OFF;
	g_displ->delta.head = head;

	expect_int_value(__wrap_print_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_print_adaptive_sync_fail, head, head);

	expect_int_value(__wrap_callback_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_callback_adaptive_sync_fail, head, head);

	act_handle_failure();

	assert_true(head->adaptive_sync_failed);

	head_free(head);
}

static void act_handle_failure__unspecified(void **state) {
	g_displ->delta.human = strdup("human");

	expect_int_value(__wrap_callback, t, FATAL);
	expect_str(__wrap_callback, msg1, "human");
	expect_str(__wrap_callback, msg2, "\nChanges failed, exiting");

	expect_int_value(__wrap_wd_exit_message, __status, EXIT_FAILURE);

	act_handle_failure();

	assert_log(FATAL, "\nChanges failed, exiting\n");
}

static void act_handle_cancelled__retrying(void **state) {
	g_cancellation_retries = 4;

	expect_int_value(__wrap_callback, t, WARNING);
	expect_str(__wrap_callback, msg1, "Changes cancelled, retrying (attempt 5)");
	expect_nul(__wrap_callback, msg2);

	act_handle_cancelled();

	assert_log(WARNING, "\nChanges cancelled, retrying (attempt 5)\n");

	assert_int_equal(g_cancellation_retries, 5);
}

static void act_handle_cancelled__over_max(void **state) {
	g_cancellation_retries = 5;

	expect_int_value(__wrap_callback, t, WARNING);
	expect_str(__wrap_callback, msg1, "Changes cancelled after 5 retries");
	expect_nul(__wrap_callback, msg2);

	act_handle_cancelled();

	assert_log(WARNING, "\nChanges cancelled after 5 retries\n");

	assert_int_equal(g_cancellation_retries, 6);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(act_apply__no_heads),
		TEST_BA(act_apply__no_changes),
		TEST_BA(act_apply__reapply),
		TEST_BA(act_apply__mode),
		TEST_BA(act_apply__vrr),
		TEST_BA(act_apply__disable),
		TEST_BA(act_apply__remainder),

		TEST_BA(act_handle_success__head_changing_adaptive_sync),
		TEST_BA(act_handle_success__head_changing_adaptive_sync_fail),
		TEST_BA(act_handle_success__head_changing_mode),
		TEST_BA(act_handle_success__ok),

		TEST_BA(act_handle_failure__mode),
		TEST_BA(act_handle_failure__adaptive_sync),
		TEST_BA(act_handle_failure__unspecified),

		TEST_BA(act_handle_cancelled__retrying),
		TEST_BA(act_handle_cancelled__over_max),
	};

	return RUN(tests);
}


#include "tst.h"

#include "assert-log.h"
#include "assert-mode.h"
#include "asserts.h"
#include "expects.h"
#include "util-col.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "displ.h"
#include "head.h"
#include "log.h"
#include "pset.h"
#include "wlr-output-management-unstable-v1.h"

#include "act.h"

extern int g_cancellation_retries;

static int before_each(void **state) {
	g_cfg = cfg_default();

	g_displ = calloc(1, sizeof(struct Displ));

	return 0;
}

static int after_each(void **state) {
	assert_logs_empty();

	free(g_displ);

	g_cfg_destroy();

	return 0;
}

static void act_apply__nothing(void **state) {
	act_apply();
}

static void act_handle_success__head_changing_adaptive_sync(void **state) {
	struct Head *head = head_n("head");
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
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
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

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
	struct Head *head = head_n("head");
	struct Mode *mode = mode_h(head);
	head->desired.mode = mode;
	pset_add(head->modes, mode);

	g_displ->delta.element = MODE;
	g_displ->delta.head = head;

	expect_int_value(__wrap_callback, t, INFO);
	expect_str(__wrap_callback, msg1, "Changes successful");
	expect_str(__wrap_callback, msg2, NULL);

	act_handle_success();

	assert_log(INFO, "\nChanges successful\n");

	assert_mode_equal(head->current.mode, mode);
	assert_ptr_equal(head->current.mode, mode);

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
	const struct Mode *mode_cur = mode_h_whr(head, 1, 2, 3);
	const struct Mode *mode_des = mode_h_whr(head, 4, 5, 6);

	head->current.mode = mode_cur;
	head->desired.mode = mode_des;

	pset_add_many(head->modes,
			mode_cur,
			mode_des,
			NULL);

	g_displ->delta.element = MODE;
	g_displ->delta.head = head;

	expect_int_value(__wrap_print_mode_fail, t, ERROR);
	expect_ptr(__wrap_print_mode_fail, head, head);
	expect_ptr(__wrap_print_mode_fail, mode, mode_des);

	expect_int_value(__wrap_callback_mode_fail, t, ERROR);
	expect_ptr(__wrap_callback_mode_fail, head, head);
	expect_ptr(__wrap_callback_mode_fail, mode, mode_des);

	act_handle_failure();

	assert_nul(head->current.mode);

	assert_mode_equal(head->desired.mode, mode_des);
	assert_ptr_equal(head->desired.mode, mode_des);

	assert_int_equal(pset_size(head->modes), 1);
	assert_true(pset_contains(head->modes, mode_cur));

	assert_int_equal(pset_size(head->modes_failed), 1);
	assert_true(pset_contains(head->modes_failed, mode_des));

	head_free(head);
}

static void act_handle_failure__adaptive_sync(void **state) {
	struct Head *head = head_n("nam");
	head->model = strdup("mod");
	head->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

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
		TEST_BA(act_apply__nothing),

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


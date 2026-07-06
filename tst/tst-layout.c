#include "tst.h"

#include "assert-log.h"
#include "assert-mode.h"
#include "asserts.h"
#include "expects.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "displ.h"
#include "fn.h"
#include "head.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"

#include "layout.h"

extern int g_cancellation_retries;

struct State {
	struct SList *heads;
};


static int before_each(void **state) {
	assert_logs_empty_before();

	g_cfg = cfg_default();

	// only set this when we specifically want to test it
	free(g_cfg->callback_cmd);
	g_cfg->callback_cmd = NULL;

	g_displ = calloc(1, sizeof(struct Displ));

	struct State *s = calloc(1, sizeof(struct State));

	for (int i = 0; i < 10; i++) {
		struct Head *head = head_init();
		head->desired.enabled = true;
		const struct Mode *mode = mode_init_h_whr(head, i * 20, i * 10, 0);
		head->desired.mode = mode;
		pset_add(head->modes, mode);
		slist_append(&s->heads, head);
	}

	*state = s;
	return 0;
}

static int after_each(void **state) {
	slist_free_vals(&g_heads, (fn_free)head_free);

	free(g_displ);

	cfg_destroy();

	struct State *s = *state;

	slist_free_vals(&s->heads, (fn_free)head_free);

	free(s);
	return 0;
}

static void handle_success__head_changing_adaptive_sync(void **state) {
	struct Head *head = head_init_name("head");
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head->adaptive_sync_failed = false;

	g_displ->delta.element = VRR_OFF;
	g_displ->delta.head = head;

	expect_int_value(__wrap_call_back, t, INFO);
	expect_str(__wrap_call_back, msg1, "Changes successful");
	expect_str(__wrap_call_back, msg2, NULL);

	handle_success();

	assert_log(INFO, "\nChanges successful\n");
	assert_logs_empty();

	assert_false(head->adaptive_sync_failed);

	head_free(head);
}

static void handle_success__head_changing_adaptive_sync_fail(void **state) {
	struct Head *head = head_init_name("head");
	head->model = NULL;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	g_displ->delta.element = VRR_OFF;
	g_displ->delta.head = head;

	expect_int_value(__wrap_print_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_print_adaptive_sync_fail, head, head);

	expect_int_value(__wrap_call_back_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_call_back_adaptive_sync_fail, head, head);

	handle_success();

	assert_true(head->adaptive_sync_failed);

	assert_logs_empty();

	head_free(head);
}

static void handle_success__head_changing_mode(void **state) {
	struct Head *head = head_init_name("head");
	struct Mode *mode = mode_init();
	mode->head = head;
	head->desired.mode = mode;
	pset_add(head->modes, mode);

	g_displ->delta.element = MODE;
	g_displ->delta.head = head;

	expect_int_value(__wrap_call_back, t, INFO);
	expect_str(__wrap_call_back, msg1, "Changes successful");
	expect_str(__wrap_call_back, msg2, NULL);

	handle_success();

	assert_log(INFO, "\nChanges successful\n");
	assert_logs_empty();

	assert_mode_equal(head->current.mode, mode);
	assert_ptr_equal(head->current.mode, mode);

	head_free(head);
}

static void handle_success__ok(void **state) {
	g_displ->delta.human = strdup("human");

	expect_int_value(__wrap_call_back, t, INFO);
	expect_str(__wrap_call_back, msg1, "human");
	expect_str(__wrap_call_back, msg2, NULL);

	handle_success();

	assert_log(INFO, "\nChanges successful\n");
	assert_logs_empty();
}

static void handle_failure__mode(void **state) {
	struct Head *head = head_init_name("nam");
	const struct Mode *mode_cur = mode_init_h_whr(head, 1, 2, 3);
	const struct Mode *mode_des = mode_init_h_whr(head, 4, 5, 6);

	head->current.mode = mode_cur;
	head->desired.mode = mode_des;

	pset_add(head->modes, mode_cur);
	pset_add(head->modes, mode_des);

	g_displ->delta.element = MODE;
	g_displ->delta.head = head;

	expect_int_value(__wrap_print_mode_fail, t, ERROR);
	expect_ptr(__wrap_print_mode_fail, head, head);
	expect_ptr(__wrap_print_mode_fail, mode, mode_des);

	expect_int_value(__wrap_call_back_mode_fail, t, ERROR);
	expect_ptr(__wrap_call_back_mode_fail, head, head);
	expect_ptr(__wrap_call_back_mode_fail, mode, mode_des);

	handle_failure();

	assert_nul(head->current.mode);

	assert_mode_equal(head->desired.mode, mode_des);
	assert_ptr_equal(head->desired.mode, mode_des);

	assert_int_equal(pset_size(head->modes), 1);
	assert_true(pset_contains(head->modes, mode_cur));

	assert_int_equal(pset_size(head->modes_failed), 1);
	assert_true(pset_contains(head->modes_failed, mode_des));

	assert_logs_empty();

	head_free(head);
}

static void handle_failure__adaptive_sync(void **state) {
	struct Head *head = head_init_name("nam");
	head->model = strdup("mod");
	head->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	head->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	g_displ->delta.element = VRR_OFF;
	g_displ->delta.head = head;

	expect_int_value(__wrap_print_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_print_adaptive_sync_fail, head, head);

	expect_int_value(__wrap_call_back_adaptive_sync_fail, t, WARNING);
	expect_ptr(__wrap_call_back_adaptive_sync_fail, head, head);

	handle_failure();

	assert_true(head->adaptive_sync_failed);

	assert_logs_empty();

	head_free(head);
}

static void handle_failure__unspecified(void **state) {
	g_displ->delta.human = strdup("human");

	expect_int_value(__wrap_call_back, t, FATAL);
	expect_str(__wrap_call_back, msg1, "human");
	expect_str(__wrap_call_back, msg2, "\nChanges failed, exiting");

	expect_int_value(__wrap_wd_exit_message, __status, EXIT_FAILURE);

	handle_failure();

	assert_log(FATAL, "\nChanges failed, exiting\n");
	assert_logs_empty();
}

static void handle_cancelled__retrying(void **state) {
	g_cancellation_retries = 4;

	expect_int_value(__wrap_call_back, t, WARNING);
	expect_str(__wrap_call_back, msg1, "Changes cancelled, retrying (attempt 5)");
	expect_nul(__wrap_call_back, msg2);

	handle_cancelled();

	assert_log(WARNING, "\nChanges cancelled, retrying (attempt 5)\n");
	assert_logs_empty();

	assert_int_equal(g_cancellation_retries, 5);
}

static void handle_cancelled__over_max(void **state) {
	g_cancellation_retries = 5;

	expect_int_value(__wrap_call_back, t, WARNING);
	expect_str(__wrap_call_back, msg1, "Changes cancelled after 5 retries");
	expect_nul(__wrap_call_back, msg2);

	handle_cancelled();

	assert_log(WARNING, "\nChanges cancelled after 5 retries\n");
	assert_logs_empty();

	assert_int_equal(g_cancellation_retries, 6);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(handle_success__head_changing_adaptive_sync),
		TEST_BA(handle_success__head_changing_adaptive_sync_fail),
		TEST_BA(handle_success__head_changing_mode),
		TEST_BA(handle_success__ok),

		TEST_BA(handle_failure__mode),
		TEST_BA(handle_failure__adaptive_sync),
		TEST_BA(handle_failure__unspecified),

		TEST_BA(handle_cancelled__retrying),
		TEST_BA(handle_cancelled__over_max),
	};

	return RUN(tests);
}


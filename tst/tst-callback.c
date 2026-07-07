#include "tst.h"

#include "assert-log.h"
#include "expect-smaps.h"
#include "expects.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "displ.h"
#include "head.h"
#include "log.h"
#include "smaps.h"
#include "str.h"

#include "info/callback.h"

struct State {
	struct Head *head1;
};

int before_each(void **state) {
	assert_logs_empty_before();

	struct State *s = calloc(1, sizeof(struct State));

	g_displ = calloc(1, sizeof(struct Displ));

	g_cfg = cfg_default();

	s->head1 = head_init();

	s->head1->name = strdup("name1");
	s->head1->description = strdup("description1");
	s->head1->current.mode = mode_init_h_whr(s->head1, 100, 200, 30000);
	s->head1->desired.mode = mode_init_h_whr(s->head1, 400, 500, 60000);

	*state = s;
	return 0;
}

int after_each(void **state) {
	struct State *s = *state;

	head_free(s->head1);

	free(s);

	displ_delta_destroy();
	free(g_displ);

	cfg_destroy();

	return 0;
}

static void callback__no_callback(void **state) {
	free(g_cfg->callback_cmd);
	g_cfg->callback_cmd = NULL;

	callback(INFO, "msg1", NULL);

	assert_logs_empty();
}

static void callback__below_threshold(void **state) {
	will_return_int(__wrap_log_get_threshold, WARNING);
	callback(INFO, "msg1", NULL);

	assert_logs_empty();
}

static void callback__one(void **state) {
	const struct SMapS *env = smaps_init();
	smaps_put_if_absent(env, "CALLBACK_MSG", "msg1");
	smaps_put_if_absent(env, "CALLBACK_LEVEL", "INFO");

	free(g_cfg->callback_cmd);
	g_cfg->callback_cmd = strdup("command");

	will_return_int(__wrap_log_get_threshold, INFO);

	expect_str(__wrap_spawn_sh_cmd, command, g_cfg->callback_cmd);
	expect_smaps(__wrap_spawn_sh_cmd, env, env);

	callback(INFO, "msg1", NULL);

	char *str_env = smaps_str(env);
	char *str_log = sprintf_alloc("\nExecuting CALLBACK_CMD:\n  command\n%s\n", str_env);

	assert_log(DEBUG, str_log);

	assert_logs_empty();

	free(str_env);
	free(str_log);
	smaps_free(env);
}

static void callback__two(void **state) {
	const struct SMapS *expected_env = smaps_init();
	smaps_put_if_absent(expected_env, "CALLBACK_MSG", "msg1msg2");
	smaps_put_if_absent(expected_env, "CALLBACK_LEVEL", "FATAL");

	free(g_cfg->callback_cmd);
	g_cfg->callback_cmd = strdup("command");

	g_displ->delta.human = strdup("not successful");

	will_return_int(__wrap_log_get_threshold, INFO);

	expect_str(__wrap_spawn_sh_cmd, command, g_cfg->callback_cmd);
	expect_smaps(__wrap_spawn_sh_cmd, env, expected_env);

	callback(FATAL, "msg1", "msg2");

	char *env_str = smaps_str(expected_env);
	char *log_str = sprintf_alloc("\nExecuting CALLBACK_CMD:\n  command\n%s\n", env_str);

	assert_log(DEBUG, log_str);

	assert_logs_empty();

	free(env_str);
	free(log_str);
	smaps_free(expected_env);
}

static void callback_mode_fail__(void **state) {
	const struct State *s = *state;

	free(g_cfg->callback_cmd);
	g_cfg->callback_cmd = strdup("command");

	const struct SMapS *expected_env = smaps_init();
	smaps_put_if_absent(expected_env, "CALLBACK_MSG",
			"description1\n"
			"  Unable to set mode 400x500@60Hz (60,000mHz), retrying");
	smaps_put_if_absent(expected_env, "CALLBACK_LEVEL", "INFO");

	will_return_int(__wrap_log_get_threshold, INFO);

	expect_str(__wrap_spawn_sh_cmd, command, g_cfg->callback_cmd);
	expect_smaps(__wrap_spawn_sh_cmd, env, expected_env);

	callback_mode_fail(INFO, s->head1, s->head1->desired.mode);

	char *env_str = smaps_str(expected_env);
	char *log_str = sprintf_alloc("\nExecuting CALLBACK_CMD:\n  command\n%s\n", env_str);

	assert_log(DEBUG, log_str);

	assert_logs_empty();

	free(env_str);
	free(log_str);
	smaps_free(expected_env);
}

static void callback_adaptive_sync_fail__(void **state) {
	struct Head *head = head_init();
	head->name = strdup("name1");
	head->model = strdup("model1");
	head->description = strdup("description1");

	g_displ->delta.head = head;

	free(g_cfg->callback_cmd);
	g_cfg->callback_cmd = strdup("command");

	const struct SMapS *expected_env = smaps_init();
	smaps_put_if_absent(expected_env, "CALLBACK_MSG",
			"description1\n"
			"  Cannot enable VRR.\n"
			"  You can disable VRR for this display in cfg.yaml\n"
			"VRR_OFF:\n"
			"  - 'model1'");
	smaps_put_if_absent(expected_env, "CALLBACK_LEVEL", "WARNING");

	will_return_int(__wrap_log_get_threshold, INFO);

	expect_str(__wrap_spawn_sh_cmd, command, g_cfg->callback_cmd);
	expect_smaps(__wrap_spawn_sh_cmd, env, expected_env);

	callback_adaptive_sync_fail(WARNING, g_displ->delta.head);

	char *env_str = smaps_str(expected_env);
	char *log_str = sprintf_alloc("\nExecuting CALLBACK_CMD:\n  command\n%s\n", env_str);

	assert_log(DEBUG, log_str);

	assert_logs_empty();

	free(env_str);
	free(log_str);
	smaps_free(expected_env);
	head_free(head);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(callback__no_callback),
		TEST_BA(callback__below_threshold),
		TEST_BA(callback__one),
		TEST_BA(callback__two),

		TEST_BA(callback_mode_fail__),

		TEST_BA(callback_adaptive_sync_fail__),
	};

	return RUN(tests);
}


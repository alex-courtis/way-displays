#include "tst.h"

#include "assert-log.h"
#include "data.h"
#include "expect-ssmap.h"
#include "expects.h"
#include "util-col.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "cfg/cfg.h"
#include "displ.h"
#include "enum.h"
#include "head.h"
#include "ssmap.h"
#include "str.h"

#include "info/callback.h"

struct State {
	struct Head *head1;
};

int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	g_displ = displ_init();

	g_cfg = cfg_default();

	s->head1 = head_init();

	s->head1->name = strdup("name1");
	s->head1->description = strdup("description1");

	ppmap_put_many(s->head1->modes,
			MC, mode_whr(100, 200, 30000),
			MD, mode_whr(400, 500, 60000),
			NULL);
	s->head1->current.zwlr_mode = MC;
	s->head1->desired.zwlr_mode = MD;

	*state = s;
	return 0;
}

int after_each(void **state) {
	struct State *s = *state;

	head_free(s->head1);

	free(s);

	displ_free(g_displ);

	g_cfg_destroy();

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
	const struct SSmap *env = ssmap_init();
	ssmap_put_many(env,
			"CALLBACK_MSG", "msg1",
			"CALLBACK_LEVEL", "INFO",
			NULL);

	free(g_cfg->callback_cmd);
	g_cfg->callback_cmd = strdup("command");

	will_return_int(__wrap_log_get_threshold, INFO);

	expect_str(__wrap_spawn_sh_cmd, command, g_cfg->callback_cmd);
	expect_ssmap(__wrap_spawn_sh_cmd, env, env);

	callback(INFO, "msg1", NULL);

	char *str_env = ssmap_str(env);
	char *str_log = sprintf_alloc("\nExecuting CALLBACK_CMD:\n  command\n%s\n", str_env);

	assert_log(DEBUG, str_log);

	free(str_env);
	free(str_log);
	ssmap_free(env);

	assert_logs_empty();
}

static void callback__two(void **state) {
	const struct SSmap *expected_env = ssmap_init();
	ssmap_put_many(expected_env,
			"CALLBACK_MSG", "msg1msg2",
			"CALLBACK_LEVEL", "FATAL",
			NULL);

	free(g_cfg->callback_cmd);
	g_cfg->callback_cmd = strdup("command");

	g_displ->delta.human = strdup("not successful");

	will_return_int(__wrap_log_get_threshold, INFO);

	expect_str(__wrap_spawn_sh_cmd, command, g_cfg->callback_cmd);
	expect_ssmap(__wrap_spawn_sh_cmd, env, expected_env);

	callback(FATAL, "msg1", "msg2");

	char *env_str = ssmap_str(expected_env);
	char *log_str = sprintf_alloc("\nExecuting CALLBACK_CMD:\n  command\n%s\n", env_str);

	assert_log(DEBUG, log_str);

	free(env_str);
	free(log_str);
	ssmap_free(expected_env);

	assert_logs_empty();
}

static void callback_mode_fail__(void **state) {
	const struct State *s = *state;

	free(g_cfg->callback_cmd);
	g_cfg->callback_cmd = strdup("command");

	s->head1->zwlr_mode_pref = MD;

	const struct SSmap *expected_env = ssmap_init();
	ssmap_put_many(expected_env,
			"CALLBACK_MSG", "description1\n"
			"  Unable to set mode 400x500@60Hz (60,000mHz) (preferred), retrying",

			"CALLBACK_LEVEL", "INFO",
			NULL);

	will_return_int(__wrap_log_get_threshold, INFO);

	expect_str(__wrap_spawn_sh_cmd, command, g_cfg->callback_cmd);
	expect_ssmap(__wrap_spawn_sh_cmd, env, expected_env);

	callback_mode_fail(INFO, s->head1, s->head1->desired.zwlr_mode);

	char *env_str = ssmap_str(expected_env);
	char *log_str = sprintf_alloc("\nExecuting CALLBACK_CMD:\n  command\n%s\n", env_str);

	assert_log(DEBUG, log_str);

	free(env_str);
	free(log_str);
	ssmap_free(expected_env);

	assert_logs_empty();
}

static void callback_adaptive_sync_fail__(void **state) {
	struct Head *head = head_init();
	head->name = strdup("name1");
	head->model = strdup("model1");
	head->description = strdup("description1");

	g_displ->delta.head = head;

	free(g_cfg->callback_cmd);
	g_cfg->callback_cmd = strdup("command");

	const struct SSmap *expected_env = ssmap_init();
	ssmap_put_many(expected_env,
			"CALLBACK_MSG", "description1\n"
			"  Cannot enable VRR.\n"
			"  You can disable VRR for this display in cfg.yaml\n"
			"VRR_OFF:\n"
			"  - 'model1'",

			"CALLBACK_LEVEL", "WARNING",
			NULL);

	will_return_int(__wrap_log_get_threshold, INFO);

	expect_str(__wrap_spawn_sh_cmd, command, g_cfg->callback_cmd);
	expect_ssmap(__wrap_spawn_sh_cmd, env, expected_env);

	callback_adaptive_sync_fail(WARNING, g_displ->delta.head);

	char *env_str = ssmap_str(expected_env);
	char *log_str = sprintf_alloc("\nExecuting CALLBACK_CMD:\n  command\n%s\n", env_str);

	assert_log(DEBUG, log_str);

	free(env_str);
	free(log_str);
	ssmap_free(expected_env);
	head_free(head);

	assert_logs_empty();
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


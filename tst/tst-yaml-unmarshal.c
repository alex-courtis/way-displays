#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "asserts.h"
#include "util-file.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "cfg/disabled.h"
#include "cfg/user-mode.h"
#include "cfg/user-scale.h"
#include "cfg/user-transform.h"
#include "conditions.h"
#include "fn.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "slist.h"
#include "smap.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"
#include "wrap-libyaml.h"

#include "yaml/unmarshal-types.h"
#include "yaml/unmarshal.h"

#include "yaml/data.c"

static int before_each(void **state) {
	reset_yaml_fails();

	return 0;
}

// expected will be free'd, log_path is optional WARNING
static void _check_unmarshalled_cfg(const char *yaml_path, struct Cfg *expected, const char *log_path, const char * const file, const int line) {
	struct Cfg *actual = yaml_unmarshal_file(yaml_path, yaml_root_to_cfg);
	_assert_non_nul(actual, "actual", file, line);

	_assert_cfg_equal(actual, expected, file, line);

	if (log_path) {
		char *expected_log = read_file(log_path);
		_assert_log(WARNING, expected_log, file, line);
		free(expected_log);
	}

	_assert_logs_empty(file, line);

	cfg_free(actual);
	cfg_free(expected);
}
#define check_unmarshalled_cfg(yaml_path, expected, log_path) _check_unmarshalled_cfg(yaml_path, expected, log_path, __FILE__, __LINE__)

static void yaml_root_to_cfg__ok(void **state) {
	check_unmarshalled_cfg("tst/yaml/cfg-all.yaml", cfg_all(), NULL);
}

static void yaml_root_to_cfg__empty(void **state) {

	assert_nul(yaml_unmarshal_file("tst/yaml/cfg-empty.yaml", yaml_root_to_cfg));

	assert_log(ERROR, "\ntst/yaml/cfg-empty.yaml: no root node\n");

	assert_logs_empty();
}

static void yaml_root_to_cfg__missing(void **state) {
	assert_nul(yaml_unmarshal_file("foo/bar/baz.yaml", yaml_root_to_cfg));

	assert_log(ERROR, "\nfoo/bar/baz.yaml: inexistent\n");

	assert_logs_empty();
}

static void yaml_root_to_cfg__invalid(void **state) {
	// all invalid have been set to default
	struct Cfg *expected = cfg_default();
	pset_add(expected->disableds, disabled_init_always("BAD_DISABLED_IFS"));

	check_unmarshalled_cfg("tst/yaml/cfg-invalid.yaml", expected, "tst/yaml/cfg-invalid.log");
}

static void yaml_root_to_cfg__legacy(void **state) {
	struct Cfg *expected = cfg_init();

	// CHANGE_SUCCESS_CMD -> CALLBACK_CMD
	free(expected->callback_cmd);
	expected->callback_cmd = strdup("foo");

	// MAX_PREFERRED_REFRESH
	sset_add(expected->max_preferred_refresh_name_desc, "fifteen");
	sset_add(expected->max_preferred_refresh_name_desc, "!sixteen");

	check_unmarshalled_cfg("tst/yaml/cfg-legacy.yaml", expected, NULL);
}

static void yaml_root_to_cfg__mistyped(void **state) {
	check_unmarshalled_cfg("tst/yaml/cfg-mistyped.yaml", cfg_default(), "tst/yaml/cfg-mistyped.log");
}

static void yaml_root_to_cfg__root_mistyped(void **state) {
	assert_nul(yaml_unmarshal_file("tst/yaml/cfg-root-mistyped.yaml", yaml_root_to_cfg));

	assert_log(WARNING, "Ignoring invalid tst/yaml/cfg-root-mistyped.yaml expected map, got sequence\n");
	assert_logs_empty();
}

static void yaml_root_to_cfg__transform(void **state) {
	struct Cfg *expected = cfg_init();
	smap_put(expected->user_transforms, "one", cfg_user_transform_init(WL_OUTPUT_TRANSFORM_FLIPPED));

	check_unmarshalled_cfg("tst/yaml/cfg-transform.yaml", expected, "tst/yaml/cfg-transform.log");
}

static void yaml_root_to_cfg__scale(void **state) {
	struct Cfg *expected = cfg_init();
	smap_put(expected->user_scales, "three", user_scale_init(3));

	check_unmarshalled_cfg("tst/yaml/cfg-scale.yaml", expected, "tst/yaml/cfg-scale.log");
}

static void yaml_root_to_cfg__mode(void **state) {
	struct Cfg *expected = cfg_init();

	smap_put(expected->user_modes, "max_override", user_mode_init(true, 1920, 1080, 12340, false));
	smap_put(expected->user_modes, "five", user_mode_init(false, 1920, 1080, 12340, false));
	smap_put(expected->user_modes, "seven", user_mode_init(true, -1, -1, -1, false));

	check_unmarshalled_cfg("tst/yaml/cfg-mode.yaml", expected, "tst/yaml/cfg-mode.log");
}

static void yaml_root_to_cfg__disabled(void **state) {
	struct Cfg *expected = cfg_init();
	pset_add(expected->disableds, disabled_init_always("eight"));
	pset_add(expected->disableds, disabled_init_always("EIGHT"));
	pset_add(expected->disableds, disabled_init_always("nine"));

	struct Disabled *disabled = calloc(1, sizeof(struct Disabled));
	disabled->name_desc = strdup("twelve");

	struct Condition *cond = calloc(1, sizeof(struct Condition));
	slist_append(&cond->plugged, strdup("ONE"));
	slist_append(&cond->plugged, strdup("TWO"));
	slist_append(&disabled->conditions, cond);

	cond = calloc(1, sizeof(struct Condition));
	slist_append(&cond->unplugged, strdup("THREE"));
	slist_append(&disabled->conditions, cond);

	pset_add(expected->disableds, disabled);

	disabled = calloc(1, sizeof(struct Disabled));
	disabled->name_desc = strdup("twelve");

	cond = calloc(1, sizeof(struct Condition));
	slist_append(&cond->plugged, strdup("FOUR"));
	slist_append(&disabled->conditions, cond);

	pset_add(expected->disableds, disabled);

	pset_add(expected->disableds, disabled_init_always("BAD_DISABLED_IFS"));
	pset_add(expected->disableds, disabled_init_always("MISTYPED_IF_SCALAR"));
	pset_add(expected->disableds, disabled_init_always("MISTYPED_IF_MAP"));
	pset_add(expected->disableds, disabled_init_always("MISTYPED_UN_PLUGGED_SCALAR"));
	pset_add(expected->disableds, disabled_init_always("MISTYPED_UN_PLUGGED_MAP"));
	pset_add(expected->disableds, disabled_init_always("MISTYPED_LID_MAP"));
	pset_add(expected->disableds, disabled_init_always("NO_VALID_CONDITIONS"));

	check_unmarshalled_cfg("tst/yaml/cfg-disabled.yaml", expected, "tst/yaml/cfg-disabled.log");
}

static void yaml_root_to_cfg__scale_round_to_invalid(void **state) {
	struct Cfg *expected = cfg_init();
	expected->scale_round_to = 8;

	check_unmarshalled_cfg("tst/yaml/cfg-scale-round-to-invalid.yaml", expected, "tst/yaml/cfg-scale-round-to-invalid.log");
}

static void yaml_root_to_cfg__scale_round_to_zero(void **state) {
	struct Cfg *expected = cfg_init();
	expected->scale_round_to = 8;

	check_unmarshalled_cfg("tst/yaml/cfg-scale-round-to-zero.yaml", expected, "tst/yaml/cfg-scale-round-to-zero.log");
}

static void yaml_root_to_ipc_request__empty(void **state) {
	const struct IpcRequest *actual = yaml_unmarshal_str("", yaml_root_to_ipc_request, "ipc request");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"ipc request: empty request\n"
			"========================================\n"
			"\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_request__mistyped_root(void **state) {
	const char *yaml = "- FOO";

	const struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_nul(actual);

	assert_log(ERROR,
			"ipc request: expected map, got sequence\n"
			"========================================\n"
			"- FOO\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_request__invalid_op(void **state) {
	const char *yaml = "OP: aoeu";

	const struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_nul(actual);

	assert_log(ERROR,
			"ipc request: invalid OP aoeu, valid values: GET|LIST|REAPPLY|CFG_SET|CFG_DEL|CFG_WRITE|CFG_TOGGLE\n"
			"========================================\n"
			"OP: aoeu\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_request__mistyped_op(void **state) {
	const char *yaml = "OP:\n  FOO: BAR";

	const struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_nul(actual);

	assert_log(ERROR,
			"ipc request: invalid OP expected scalar, got map, valid values: GET|LIST|REAPPLY|CFG_SET|CFG_DEL|CFG_WRITE|CFG_TOGGLE\n"
			"========================================\n"
			"OP:\n"
			"  FOO: BAR\n"
			"----------------------------------------\n");
	assert_logs_empty();
}


static void yaml_root_to_ipc_request__no_op(void **state) {
	const char *yaml = "FOO: BAR";

	const struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_nul(actual);

	assert_log(ERROR,
			"ipc request: missing OP\n"
			"========================================\n"
			"FOO: BAR\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_request__invalid_cfg(void **state) {
	struct Cfg *expected = cfg_default();
	pset_add(expected->disableds, disabled_init_always("BAD_DISABLED_IFS"));

	char *yaml = read_file("tst/yaml/ipc-request-cfg-invalid.yaml");

	struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_non_nul(actual);
	assert_int_equal(actual->command, CFG_SET);
	assert_int_equal(actual->log_threshold, ERROR);

	assert_cfg_equal(actual->cfg, expected);

	char *expected_log = read_file("tst/yaml/ipc-request-cfg-invalid.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	free(yaml);
	ipc_request_free(actual);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_root_to_ipc_request__cfg_set(void **state) {
	struct Cfg *expected = cfg_all();

	char *yaml = read_file("tst/yaml/ipc-request-cfg-set.yaml");

	struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_non_nul(actual);
	assert_int_equal(actual->command, CFG_SET);
	assert_int_equal(actual->log_threshold, ERROR);

	assert_cfg_equal(actual->cfg, expected);

	ipc_request_free(actual);
	cfg_free(expected);
	free(yaml);

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__empty(void **state) {
	assert_nul(yaml_unmarshal_str("", yaml_root_to_ipc_response_list, "ipc response"));

	assert_log(ERROR, "\n"
			"ipc response: empty request\n"
			"========================================\n"
			"\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__mistyped_root(void **state) {
	assert_nul(yaml_unmarshal_str("foo", yaml_root_to_ipc_response_list, "ipc response"));

	assert_log(ERROR, "\n"
			"ipc response: expected map or sequence, got scalar\n"
			"========================================\n"
			"foo\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__seq_no_map(void **state) {
	assert_nul(yaml_unmarshal_str("-", yaml_root_to_ipc_response_list, "ipc response"));

	assert_log(ERROR,
			"ipc response: expected map, got scalar\n"
			"========================================\n"
			"-\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__seq_no_done(void **state) {
	expect_function_call(__wrap_lid_free);

	assert_nul(yaml_unmarshal_str("- FOO: BAR", yaml_root_to_ipc_response_list, "ipc response"));

	assert_log(ERROR,
			"ipc response: missing DONE\n"
			"========================================\n"
			"- FOO: BAR\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__seq_no_rc(void **state) {
	expect_function_call(__wrap_lid_free);

	const struct SList *actual = yaml_unmarshal_str( "- DONE: TRUE", yaml_root_to_ipc_response_list, "ipc response");

	assert_nul(actual);

	assert_log(ERROR,
			"ipc response: missing RC\n"
			"========================================\n"
			"- DONE: TRUE\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__map(void **state) {
	char *yaml = read_file("tst/yaml/ipc-responses-map.yaml");

	expect_function_call(__wrap_lid_free);

	struct SList *responses = yaml_unmarshal_str(yaml, yaml_root_to_ipc_response_list, "ipc response");

	assert_non_nul(responses);
	assert_int_equal(slist_length(responses), 1);

	struct IpcResponse *response = slist_at(responses, 0);

	assert_true(response->status.done);
	assert_int_equal(response->status.rc, 2);

	assert_non_nul(response->lid);
	assert_true(response->lid->closed);
	assert_str_equal(response->lid->device_path, "/path/to/lid");

	assert_non_nul(response->cfg);
	struct Cfg *expected_cfg = cfg_all();
	assert_cfg_equal(response->cfg, expected_cfg);

	assert_int_equal(slist_length(response->heads), 1);
	struct Head *head = slist_at(response->heads, 0);

	assert_str_equal(head->name, "name");
	assert_str_equal(head->description, "desc");
	assert_int_equal(head->width_mm, 1);
	assert_int_equal(head->height_mm, 2);
	assert_str_equal(head->make, "make");
	assert_str_equal(head->model, "model");
	assert_str_equal(head->serial_number, "serial");

	assert_int_equal(head->current.scale, wl_fixed_from_double(4));
	assert_true(head->current.enabled);
	assert_int_equal(head->current.x, 5);
	assert_int_equal(head->current.y, 6);
	assert_int_equal(head->current.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED);

	assert_int_equal(head->desired.scale, wl_fixed_from_double(7.0));
	assert_true(head->desired.enabled);
	assert_int_equal(head->desired.x, 8);
	assert_int_equal(head->desired.y, 9);
	assert_int_equal(head->desired.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	struct Mode *mode_current = head->current.mode;
	assert_non_nul(mode_current);
	assert_int_equal(mode_current->width, 10);
	assert_int_equal(mode_current->height, 11);
	assert_int_equal(mode_current->refresh_mhz, 12);
	assert_true(mode_current->preferred);

	struct Mode *mode_desired = head->desired.mode;
	assert_non_nul(mode_desired);
	assert_int_equal(mode_desired->width, 13);
	assert_int_equal(mode_desired->height, 14);
	assert_int_equal(mode_desired->refresh_mhz, 15);
	assert_false(mode_desired->preferred);

	assert_int_equal(slist_length(head->modes), 2);
	struct Mode *mode1 = slist_at(head->modes, 0);
	assert_non_nul(mode1);
	assert_int_equal(mode1->width, 10);
	assert_int_equal(mode1->height, 11);
	assert_int_equal(mode1->refresh_mhz, 12);
	assert_true(mode1->preferred);

	struct Mode *mode2 = slist_at(head->modes, 1);
	assert_non_nul(mode2);
	assert_int_equal(mode2->width, 13);
	assert_int_equal(mode2->height, 14);
	assert_int_equal(mode2->refresh_mhz, 15);
	assert_false(mode2->preferred);

	assert_int_equal(head->current.transform, 3);
	assert_int_equal(head->desired.transform, 4);

	assert_int_equal(slist_length(response->log_cap_lines), 3);

	struct LogCapLine *line = slist_at(response->log_cap_lines, 0);
	assert_non_nul(line);
	assert_int_equal(line->threshold, WARNING);
	assert_str_equal(line->line, "war");

	line = slist_at(response->log_cap_lines, 1);
	assert_non_nul(line);
	assert_int_equal(line->threshold, ERROR);
	assert_str_equal(line->line, "err");

	line = slist_at(response->log_cap_lines, 2);
	assert_non_nul(line);
	assert_int_equal(line->threshold, FATAL);
	assert_str_equal(line->line, "fat");

	assert_int_equal(head->overrided_enabled, OverrideFalse);

	slist_free_vals(&responses, (fn_free)ipc_response_free);
	cfg_free(expected_cfg);
	free(yaml);

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__seq(void **state) {
	char *yaml = read_file("tst/yaml/ipc-responses-seq-brief.yaml");

	expect_function_calls(__wrap_lid_free, 3);

	struct SList *responses = yaml_unmarshal_str(yaml, yaml_root_to_ipc_response_list, "ipc response");

	struct Cfg *cfg_expected = cfg_init();
	cfg_expected->arrange = COL;

	assert_non_nul(responses);
	assert_int_equal(slist_length(responses), 3);

	// 0
	struct IpcResponse *response = slist_at(responses, 0);
	assert_non_nul(response);
	assert_true(response->status.done);
	assert_int_equal(response->status.rc, 0);

	const struct Cfg *cfg_actual = response->cfg;
	assert_non_nul(cfg_actual);
	assert_cfg_equal(cfg_actual, cfg_expected);

	const struct Lid *lid = response->lid;
	assert_non_nul(lid);
	assert_str_equal(lid->device_path, "/path/to/lid");

	assert_non_nul(response->heads);
	assert_int_equal(slist_length(response->heads), 2);

	struct Head *head0 = slist_at(response->heads, 0);
	assert_non_nul(head0);
	assert_str_equal(head0->name, "name0");
	assert_int_equal(head0->overrided_enabled, NoOverride);

	struct Head *head1 = slist_at(response->heads, 1);
	assert_non_nul(head1);
	assert_str_equal(head1->name, "name1");
	assert_int_equal(head1->overrided_enabled, NoOverride);

	assert_int_equal(slist_length(response->log_cap_lines), 4);
	struct LogCapLine *line = slist_at(response->log_cap_lines, 0);
	assert_non_nul(line);
	assert_int_equal(line->threshold, DEBUG);
	assert_str_equal(line->line, "dbg0");

	// 1
	response = slist_at(responses, 1);
	assert_non_nul(response);
	assert_false(response->status.done);
	assert_int_equal(response->status.rc, 1);
	assert_nul(response->cfg);
	assert_nul(response->lid);
	assert_nul(response->heads);

	assert_int_equal(slist_length(response->log_cap_lines), 4);
	line = slist_at(response->log_cap_lines, 0);
	assert_non_nul(line);
	assert_int_equal(line->threshold, DEBUG);
	assert_str_equal(line->line, "dbg1");

	// 2
	response = slist_at(responses, 2);
	assert_non_nul(response);
	assert_true(response->status.done);
	assert_int_equal(response->status.rc, 2);
	assert_nul(response->cfg);
	assert_nul(response->lid);
	assert_nul(response->heads);
	assert_nul(response->log_cap_lines);

	slist_free_vals(&responses, (fn_free)ipc_response_free);
	free(yaml);
	cfg_free(cfg_expected);

	assert_logs_empty();
}

static void yaml_unmarshal_str__yaml_document_initialize_fail(void **state) {

	yaml_parser_initialize__fail = true;

	assert_nul(yaml_unmarshal_str("", yaml_root_to_cfg, "foo"));

	assert_log(ERROR, "\nfoo: yaml_parser_initialize failed\n");
	assert_logs_empty();
}

static void yaml_unmarshal_str__yaml_parser_load_fail(void **state) {

	// https://github.com/yaml/libyaml/tree/run-test-suite
	// 4HVU
	// line and column error
	char *yaml =
		"key:\n"
		"   - ok\n"
		"   - also ok\n"
		"  - wrong";

	assert_nul(yaml_unmarshal_str(yaml, yaml_root_to_cfg, "foo"));

	char *err = sprintf_alloc(
			"\nfoo line 3 column 2: did not find expected key (while parsing a block mapping)\n"
			"========================================\n"
			"%s\n"
			"----------------------------------------\n",
			yaml);

	assert_log(ERROR, err);
	assert_logs_empty();

	free(err);
}

static void yaml_unmarshal_file__yaml_document_initialize_fail(void **state) {
	yaml_parser_initialize__fail = true;

	assert_nul(yaml_unmarshal_file("tst/yaml/cfg-all.yaml", yaml_root_to_cfg));

	assert_log(ERROR, "\ntst/yaml/cfg-all.yaml: yaml_parser_initialize failed\n");
	assert_logs_empty();
}

static void yaml_unmarshal_file__yaml_parser_load_fail(void **state) {

	// https://github.com/yaml/libyaml/tree/run-test-suite
	// line error only
	assert_nul(yaml_unmarshal_file("tst/yaml/CQ3W.yaml", yaml_root_to_cfg));

	assert_log(ERROR, "\ntst/yaml/CQ3W.yaml line 2: found unexpected end of stream (while scanning a quoted scalar)\n");
	assert_logs_empty();
}

int main(void) {

	const struct CMUnitTest tests[] = {
		TEST_B(yaml_root_to_cfg__ok),
		TEST_B(yaml_root_to_cfg__empty),
		TEST_B(yaml_root_to_cfg__missing),
		TEST_B(yaml_root_to_cfg__invalid),
		TEST_B(yaml_root_to_cfg__legacy),
		TEST_B(yaml_root_to_cfg__mistyped),
		TEST_B(yaml_root_to_cfg__root_mistyped),
		TEST_B(yaml_root_to_cfg__transform),
		TEST_B(yaml_root_to_cfg__scale),
		TEST_B(yaml_root_to_cfg__mode),
		TEST_B(yaml_root_to_cfg__disabled),
		TEST_B(yaml_root_to_cfg__scale_round_to_invalid),
		TEST_B(yaml_root_to_cfg__scale_round_to_zero),

		TEST_B(yaml_root_to_ipc_request__empty),
		TEST_B(yaml_root_to_ipc_request__mistyped_root),
		TEST_B(yaml_root_to_ipc_request__invalid_op),
		TEST_B(yaml_root_to_ipc_request__mistyped_op),
		TEST_B(yaml_root_to_ipc_request__no_op),
		TEST_B(yaml_root_to_ipc_request__invalid_cfg),
		TEST_B(yaml_root_to_ipc_request__cfg_set),

		TEST_B(yaml_root_to_ipc_response_list__empty),
		TEST_B(yaml_root_to_ipc_response_list__mistyped_root),
		TEST_B(yaml_root_to_ipc_response_list__seq_no_map),
		TEST_B(yaml_root_to_ipc_response_list__seq_no_done),
		TEST_B(yaml_root_to_ipc_response_list__seq_no_rc),
		TEST_B(yaml_root_to_ipc_response_list__map),
		TEST_B(yaml_root_to_ipc_response_list__seq),

		TEST_B(yaml_unmarshal_str__yaml_document_initialize_fail),
		TEST_B(yaml_unmarshal_str__yaml_parser_load_fail),
		TEST_B(yaml_unmarshal_file__yaml_document_initialize_fail),
		TEST_B(yaml_unmarshal_file__yaml_parser_load_fail),
	};

	return RUN(tests);
}


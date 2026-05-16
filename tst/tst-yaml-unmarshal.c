#include "tst.h"
#include "asserts.h"
#include "util.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "conditions.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"

#include "yaml/unmarshal.h"
#include "yaml/unmarshal-types.h"

#include "data-yaml.c"
#include "wrap-libyaml.h"

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	logs_clear();

	reset_yaml_fails();

	return 0;
}

int after_each(void **state) {
	return 0;
}


static void yaml_root_to_cfg__ok(void **state) {

	struct Cfg *read = yaml_unmarshal_file("tst/yaml/cfg-all.yaml", yaml_root_to_cfg);
	assert_non_nul(read);

	struct Cfg *expected = cfg_all();

	assert_cfg_equal(read, expected);

	cfg_free(read);
	cfg_free(expected);

	assert_logs_empty();
}

static void yaml_root_to_cfg__empty(void **state) {

	struct Cfg *read = yaml_unmarshal_file("tst/yaml/cfg-empty.yaml", yaml_root_to_cfg);
	assert_nul(read);

	assert_log(ERROR, "\nparsing file tst/yaml/cfg-empty.yaml no root node\n");

	assert_logs_empty();
}

static void yaml_root_to_cfg__missing(void **state) {
	struct Cfg *read = yaml_unmarshal_file("foo/bar/baz.yaml", yaml_root_to_cfg);
	assert_nul(read);

	assert_log(ERROR, "\nparsing file foo/bar/baz.yaml: inexistent\n");

	assert_logs_empty();

	cfg_free(read);
}

static void yaml_root_to_cfg__invalid(void **state) {
	struct Cfg *read = yaml_unmarshal_file("tst/yaml/cfg-invalid.yaml", yaml_root_to_cfg);
	assert_non_nul(read);

	// all invalid have been set to default
	struct Cfg *expected = cfg_default();
	slist_append(&expected->disabled, cfg_disabled_always("BAD_DISABLED_IFS"));

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/yaml/cfg-invalid.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_root_to_cfg__legacy(void **state) {

	struct Cfg *read = yaml_unmarshal_file("tst/yaml/cfg-legacy.yaml", yaml_root_to_cfg);
	assert_non_nul(read);

	struct Cfg *expected = cfg_init();

	// CHANGE_SUCCESS_CMD -> CALLBACK_CMD
	free(expected->callback_cmd);
	expected->callback_cmd = strdup("foo");

	// MAX_PREFERRED_REFRESH
	slist_append(&expected->max_preferred_refresh_name_desc, strdup("fifteen"));
	slist_append(&expected->max_preferred_refresh_name_desc, strdup("!sixteen"));

	assert_cfg_equal(read, expected);

	cfg_free(read);
	cfg_free(expected);

	assert_logs_empty();
}


static void yaml_root_to_cfg__mistyped(void **state) {

	struct Cfg *read = yaml_unmarshal_file("tst/yaml/cfg-mistyped.yaml", yaml_root_to_cfg);
	assert_non_nul(read);

	// all invalid have been set to default
	struct Cfg *expected = cfg_default();

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/yaml/cfg-mistyped.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_root_to_cfg__root_mistyped(void **state) {

	struct Cfg *read = yaml_unmarshal_file("tst/yaml/cfg-root-mistyped.yaml", yaml_root_to_cfg);
	assert_nul(read);

	assert_log(WARNING, "Ignoring invalid tst/yaml/cfg-root-mistyped.yaml expected map, got sequence\n");
	assert_logs_empty();
}

static void yaml_root_to_cfg__transform(void **state) {

	struct Cfg *read = yaml_unmarshal_file("tst/yaml/cfg-transform.yaml", yaml_root_to_cfg);
	assert_non_nul(read);

	struct Cfg *expected = cfg_init();
	slist_append(&expected->user_transforms, cfg_user_transform_init("one", WL_OUTPUT_TRANSFORM_FLIPPED));

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/yaml/cfg-transform.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_root_to_cfg__scale(void **state) {

	struct Cfg *read = yaml_unmarshal_file("tst/yaml/cfg-scale.yaml", yaml_root_to_cfg);
	assert_non_nul(read);

	struct Cfg *expected = cfg_init();
	slist_append(&expected->user_scales, cfg_user_scale_init("three", 3));

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/yaml/cfg-scale.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_root_to_cfg__mode(void **state) {

	struct Cfg *read = yaml_unmarshal_file("tst/yaml/cfg-mode.yaml", yaml_root_to_cfg);
	assert_non_nul(read);

	struct Cfg *expected = cfg_init();
	slist_append(&expected->user_modes, cfg_user_mode_init("max_override", true, 1920, 1080, 12340, false));
	slist_append(&expected->user_modes, cfg_user_mode_init("five", false, 1920, 1080, 12340, false));
	slist_append(&expected->user_modes, cfg_user_mode_init("seven", true, -1, -1, -1, false));

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/yaml/cfg-mode.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_root_to_cfg__disabled(void **state) {

	struct Cfg *read = yaml_unmarshal_file("tst/yaml/cfg-disabled.yaml", yaml_root_to_cfg);
	assert_non_nul(read);

	struct Cfg *expected = cfg_init();
	slist_append(&expected->disabled, cfg_disabled_always("eight"));
	slist_append(&expected->disabled, cfg_disabled_always("EIGHT"));
	slist_append(&expected->disabled, cfg_disabled_always("nine"));

	struct Disabled *disabled = calloc(1, sizeof(struct Disabled));
	disabled->name_desc = strdup("twelve");

	struct Condition *cond = calloc(1, sizeof(struct Condition));
	slist_append(&cond->plugged, strdup("ONE"));
	slist_append(&cond->plugged, strdup("TWO"));
	slist_append(&disabled->conditions, cond);

	cond = calloc(1, sizeof(struct Condition));
	slist_append(&cond->unplugged, strdup("THREE"));
	slist_append(&disabled->conditions, cond);

	slist_append(&expected->disabled, disabled);

	slist_append(&expected->disabled, cfg_disabled_always("BAD_DISABLED_IFS"));
	slist_append(&expected->disabled, cfg_disabled_always("MISTYPED_IF_SCALAR"));
	slist_append(&expected->disabled, cfg_disabled_always("MISTYPED_IF_MAP"));
	slist_append(&expected->disabled, cfg_disabled_always("MISTYPED_UN_PLUGGED_SCALAR"));
	slist_append(&expected->disabled, cfg_disabled_always("MISTYPED_UN_PLUGGED_MAP"));

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/yaml/cfg-disabled.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_root_to_ipc_request__empty(void **state) {
	struct IpcRequest *actual = yaml_unmarshal_str("", yaml_root_to_ipc_request, "ipc request");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc request: empty request\n"
			"========================================\n"
			"\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_request__mistyped_root(void **state) {
	char *yaml = "- FOO";

	struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc request: expected map, got sequence\n"
			"========================================\n"
			"- FOO\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_request__invalid_op(void **state) {
	char *yaml = "OP: aoeu";

	struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc request: invalid OP aoeu\n"
			"========================================\n"
			"OP: aoeu\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_request__mistyped_op(void **state) {
	char *yaml = "OP:\n  FOO: BAR";

	struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc request: invalid OP expected scalar, got map\n"
			"========================================\n"
			"OP:\n"
			"  FOO: BAR\n"
			"----------------------------------------\n");
	assert_logs_empty();
}


static void yaml_root_to_ipc_request__no_op(void **state) {
	char *yaml = "FOO: BAR";

	struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc request: missing OP\n"
			"========================================\n"
			"FOO: BAR\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_request__invalid_cfg(void **state) {
	char *yaml = read_file("tst/yaml/ipc-request-cfg-invalid.yaml");

	struct Cfg *expected_cfg = cfg_default();
	slist_append(&expected_cfg->disabled, cfg_disabled_always("BAD_DISABLED_IFS"));

	struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_non_nul(actual);
	assert_int_equal(actual->command, CFG_SET);
	assert_int_equal(actual->log_threshold, ERROR);

	assert_cfg_equal(actual->cfg, expected_cfg);

	char *expected_log = read_file("tst/yaml/cfg-invalid.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	free(yaml);
	ipc_request_free(actual);
	cfg_free(expected_cfg);
	free(expected_log);
}

static void yaml_root_to_ipc_request__cfg_set(void **state) {
	char *yaml = read_file("tst/yaml/ipc-request-cfg-set.yaml");

	struct Cfg *expected_cfg = cfg_all();

	struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_non_nul(actual);
	assert_int_equal(actual->command, CFG_SET);
	assert_int_equal(actual->log_threshold, ERROR);

	assert_cfg_equal(actual->cfg, expected_cfg);

	ipc_request_free(actual);
	cfg_free(expected_cfg);
	free(yaml);

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__empty(void **state) {
	char *yaml = "";

	struct SList *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_response_list, "ipc response");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc response: empty request\n"
			"========================================\n"
			"\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__mistyped_root(void **state) {

	char *yaml = "foo";

	struct SList *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_response_list, "ipc response");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc response: expected map or sequence, got scalar\n"
			"========================================\n"
			"foo\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__seq_no_map(void **state) {
	char *yaml = "-";

	struct SList *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_response_list, "ipc response");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc response: expected map, got scalar\n"
			"========================================\n"
			"-\n"
			"----------------------------------------\n");

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__seq_no_done(void **state) {
	char *yaml = "- FOO: BAR";

	expect_function_call(__wrap_lid_free);

	struct SList *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_response_list, "ipc response");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc response: missing DONE\n"
			"========================================\n"
			"- FOO: BAR\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__seq_no_rc(void **state) {
	char *yaml = "- DONE: TRUE";

	expect_function_call(__wrap_lid_free);

	struct SList *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_response_list, "ipc response");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc response: missing RC\n"
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

	slist_free_vals(&responses, ipc_response_free);
	cfg_free(expected_cfg);
	free(yaml);

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_list__seq(void **state) {
	char *yaml = read_file("tst/yaml/ipc-responses-seq-brief.yaml");

	expect_function_calls(__wrap_lid_free, 3);

	struct SList *responses = yaml_unmarshal_str(yaml, yaml_root_to_ipc_response_list, "ipc response");

	struct Cfg cfg_expected = {
		.arrange = COL
	};

	assert_non_nul(responses);
	assert_int_equal(slist_length(responses), 3);

	// 0
	struct IpcResponse *response = slist_at(responses, 0);
	assert_non_nul(response);
	assert_true(response->status.done);
	assert_int_equal(response->status.rc, 0);

	struct Cfg *cfg_actual = response->cfg;
	assert_non_nul(cfg_actual);
	assert_cfg_equal(cfg_actual, &cfg_expected);

	struct Lid *lid = response->lid;
	assert_non_nul(lid);
	assert_str_equal(lid->device_path, "/path/to/lid");

	struct SList *heads = response->heads;
	assert_non_nul(heads);
	assert_int_equal(slist_length(heads), 2);

	struct Head *head0 = slist_at(heads, 0);
	assert_non_nul(head0);
	assert_str_equal(head0->name, "name0");
	assert_int_equal(head0->overrided_enabled, NoOverride);

	struct Head *head1 = slist_at(heads, 1);
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

	slist_free_vals(&responses, ipc_response_free);
	free(yaml);

	assert_logs_empty();
}

static void yaml_unmarshal_str__yaml_document_initialize_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_parser_initialize__fail = true;

	assert_nul(yaml_unmarshal_str("", yaml_root_to_cfg, "foo"));

	cfg_free(cfg);

	assert_log(ERROR, "\nunmarshalling foo: yaml_parser_initialize failed\n");
	assert_logs_empty();
}

static void yaml_unmarshal_str__yaml_parser_load_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_parser_load__fail = true;

	assert_nul(yaml_unmarshal_str("FOO: bar", yaml_root_to_cfg, "foo"));

	cfg_free(cfg);

	assert_log(ERROR, "\n"
			"unmarshalling foo: yaml_parser_load failed\n"
			"========================================\n"
			"FOO: bar\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void yaml_unmarshal_file__yaml_document_initialize_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_parser_initialize__fail = true;

	assert_nul(yaml_unmarshal_file("tst/yaml/cfg-all.yaml", yaml_root_to_cfg));

	cfg_free(cfg);

	assert_log(ERROR, "\nparsing file tst/yaml/cfg-all.yaml: yaml_parser_initialize failed\n");
	assert_logs_empty();
}

static void yaml_unmarshal_file__yaml_parser_load_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_parser_load__fail = true;

	assert_nul(yaml_unmarshal_file("tst/yaml/cfg-all.yaml", yaml_root_to_cfg));

	cfg_free(cfg);

	assert_log(ERROR, "\nparsing file tst/yaml/cfg-all.yaml: yaml_parser_load failed\n");
	assert_logs_empty();
}

int main(void) {

	const struct CMUnitTest tests[] = {
		TEST(yaml_root_to_cfg__ok),
		TEST(yaml_root_to_cfg__empty),
		TEST(yaml_root_to_cfg__missing),
		TEST(yaml_root_to_cfg__invalid),
		TEST(yaml_root_to_cfg__legacy),
		TEST(yaml_root_to_cfg__mistyped),
		TEST(yaml_root_to_cfg__root_mistyped),
		TEST(yaml_root_to_cfg__transform),
		TEST(yaml_root_to_cfg__scale),
		TEST(yaml_root_to_cfg__mode),
		TEST(yaml_root_to_cfg__disabled),

		TEST(yaml_root_to_ipc_request__empty),
		TEST(yaml_root_to_ipc_request__mistyped_root),
		TEST(yaml_root_to_ipc_request__invalid_op),
		TEST(yaml_root_to_ipc_request__mistyped_op),
		TEST(yaml_root_to_ipc_request__no_op),
		TEST(yaml_root_to_ipc_request__invalid_cfg),
		TEST(yaml_root_to_ipc_request__cfg_set),

		TEST(yaml_root_to_ipc_response_list__empty),
		TEST(yaml_root_to_ipc_response_list__mistyped_root),
		TEST(yaml_root_to_ipc_response_list__seq_no_map),
		TEST(yaml_root_to_ipc_response_list__seq_no_done),
		TEST(yaml_root_to_ipc_response_list__seq_no_rc),
		TEST(yaml_root_to_ipc_response_list__map),
		TEST(yaml_root_to_ipc_response_list__seq),

		TEST(yaml_unmarshal_str__yaml_document_initialize_fail),
		TEST(yaml_unmarshal_str__yaml_parser_load_fail),
		TEST(yaml_unmarshal_file__yaml_document_initialize_fail),
		TEST(yaml_unmarshal_file__yaml_parser_load_fail),
	};

	return RUN(tests);
}


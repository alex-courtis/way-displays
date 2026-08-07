#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "asserts.h"
#include "data.h"
#include "util-col.h"
#include "util-file.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#include <yaml.h>

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "enum.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "plist.h"
#include "ppmap.h"
#include "pset.h"
#include "simap.h"
#include "sset.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml/unmarshal-types.h"

#include "yaml/unmarshal.h"

// these mocks are local to this test as they specifically require __real to be present i.e. explicitly wrapped
//
int __real_yaml_parser_initialize(yaml_parser_t *parser);
int __wrap_yaml_parser_initialize(yaml_parser_t *parser) { // cppcheck-suppress staticFunction
	return has_mock() ? mock_int() : __real_yaml_parser_initialize(parser);
}

// expected will be free'd, log_path is optional WARNING
static void _check_unmarshalled_cfg(const char *yaml_path, struct Cfg *expected, const char *log_path, const char * const file, const int line) {
	struct Cfg *actual = yaml_unmarshal_file(yaml_path, yaml_root_to_cfg);
	_assert_non_nul(actual, "actual", file, line);

	_assert_int_equal(actual->disableds_empty, expected->disableds_empty, file, line);

	_assert_cfg(actual, expected, true, "assert_cfg_equal", file, line);

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

	assert_logs_empty();
}

static void yaml_root_to_cfg__empty_arrays(void **state) {
	struct Cfg *expected = cfg_init();
	expected->disableds_empty = true;

	check_unmarshalled_cfg("tst/yaml/cfg-empty-arrays.yaml", expected, NULL);

	assert_logs_empty();
}

static void yaml_root_to_cfg__unknown_fields_warn(void **state) {
	struct Cfg *expected = cfg_init();
	expected->arrange = COL;

	check_unmarshalled_cfg("tst/yaml/cfg-unknown-fields.yaml", expected, "tst/yaml/cfg-unknown-fields.log");

	assert_logs_empty();
}

static void yaml_root_to_cfg__missing(void **state) {
	assert_nul(yaml_unmarshal_file("foo/bar/baz.yaml", yaml_root_to_cfg));

	assert_log(ERROR, "\nfoo/bar/baz.yaml: inexistent\n");

	assert_logs_empty();
}

static void yaml_root_to_cfg__invalid(void **state) {
	// all invalid have been set to default
	struct Cfg *expected = cfg_default_scalars();
	expected->disableds_empty = true;

	check_unmarshalled_cfg("tst/yaml/cfg-invalid.yaml", expected, "tst/yaml/cfg-invalid.log");

	assert_logs_empty();
}

static void yaml_root_to_cfg__mistyped(void **state) {
	struct Cfg *expected = cfg_default_scalars();
	expected->disableds_empty = false;

	check_unmarshalled_cfg("tst/yaml/cfg-mistyped.yaml", expected, "tst/yaml/cfg-mistyped.log");

	assert_logs_empty();
}

static void yaml_root_to_cfg__root_mistyped(void **state) {
	assert_nul(yaml_unmarshal_file("tst/yaml/cfg-root-mistyped.yaml", yaml_root_to_cfg));

	assert_log(WARNING, "cfg-root-mistyped.yaml: invalid document, expected map, got sequence\n");

	assert_logs_empty();
}

static void yaml_root_to_cfg__transform(void **state) {
	struct Cfg *expected = cfg_init();
	simap_put(expected->transforms, "ninety", WL_OUTPUT_TRANSFORM_90);

	check_unmarshalled_cfg("tst/yaml/cfg-transform.yaml", expected, "tst/yaml/cfg-transform.log");

	assert_logs_empty();
}

static void yaml_root_to_cfg__scale(void **state) {
	struct Cfg *expected = cfg_init();
	simap_put(expected->scales, "three", 3000);
	simap_put(expected->scales, "large", 900005);

	check_unmarshalled_cfg("tst/yaml/cfg-scale.yaml", expected, "tst/yaml/cfg-scale.log");

	assert_logs_empty();
}

static void yaml_root_to_cfg__mode(void **state) {
	struct Cfg *expected = cfg_init();

	struct Mode *max_overrides_max_pref = mode_whr_max(1024, 768, 85000);
	max_overrides_max_pref->max_preferred_refresh = true;

	spmap_put_many(expected->modes,
			"width_height_hz", mode_whr(1920, 1080, 12340),
			"max_overrides", mode_whr_max(1280, 720, 60000),
			"max_overrides_max_pref", max_overrides_max_pref,
			"max_pref_overrides", mode_whr_max_pref(640, 480, 30000),
			"max_only", mode_whr_max(-1, -1, -1),
			"max_pref_only", mode_whr_max_pref(-1, -1, -1),
			"unknown_key", mode_whr(800, 600, -1),
			NULL);

	check_unmarshalled_cfg("tst/yaml/cfg-mode.yaml", expected, "tst/yaml/cfg-mode.log");

	assert_logs_empty();
}

static void yaml_root_to_cfg__disabled(void **state) {
	struct Cfg *expected = cfg_init();

	struct CfgDisabled *conditionally = cfg_disabled_init();

	struct CfgCondition *cond = cfg_condition_init();
	sset_add_many(cond->plugged, "first", "second", NULL);
	pset_add(conditionally->conditions, cond);

	cond = cfg_condition_init();
	sset_add_many(cond->plugged, "first", "second", NULL);
	cond->lid = LID_OPEN;
	pset_add(conditionally->conditions, cond);

	cond = cfg_condition_init();
	sset_add_many(cond->unplugged, "third", NULL);
	cond->lid = LID_CLOSED;
	pset_add(conditionally->conditions, cond);

	cond = cfg_condition_init();
	sset_add(cond->plugged, "fourth");
	pset_add(conditionally->conditions, cond);
	sset_add_many(cond->unplugged, "fifth", "sixth", NULL);
	cond->lid = LID_NOT_PRESENT;
	pset_add(conditionally->conditions, cond);

	struct CfgDisabled *unknown_key = cfg_disabled_init();
	cond = cfg_condition_init();
	cond->lid = LID_OPEN;
	pset_add(unknown_key->conditions, cond);

	struct CfgDisabled *some_bad_conditions = cfg_disabled_init();
	cond = cfg_condition_init();
	cond->lid = LID_CLOSED;
	pset_add(some_bad_conditions->conditions, cond);
	cond = cfg_condition_init();
	sset_add(cond->plugged, "ninth");
	pset_add(some_bad_conditions->conditions, cond);

	spmap_put_many(expected->disableds,
			"unconditional",       cfg_disabled_init(),
			"conditional",         conditionally,
			"unknown_key",         unknown_key,
			"some_bad_conditions", some_bad_conditions,
			NULL);

	check_unmarshalled_cfg("tst/yaml/cfg-disabled.yaml", expected, "tst/yaml/cfg-disabled.log");

	assert_logs_empty();
}

static void yaml_root_to_cfg__disabled__disableds_empty(void **state) {

	// empty map, really empty
	struct Cfg *expected = cfg_init();
	expected->disableds_empty = true;
	check_unmarshalled_cfg("tst/yaml/cfg-disabled-empty.yaml", expected, NULL);

	// bad entries, also empty
	expected = cfg_init();
	expected->disableds_empty = true;
	check_unmarshalled_cfg("tst/yaml/cfg-disabled-all-bad.yaml", expected, "tst/yaml/cfg-disabled-all-bad.log");
}

static void yaml_root_to_cfg__scale_round_to_invalid(void **state) {
	struct Cfg *expected = cfg_init();
	expected->scale_round_to = 8;

	check_unmarshalled_cfg("tst/yaml/cfg-scale-round-to-invalid.yaml", expected, "tst/yaml/cfg-scale-round-to-invalid.log");

	assert_logs_empty();
}

static void yaml_root_to_cfg__scale_round_to_zero(void **state) {
	struct Cfg *expected = cfg_init();
	expected->scale_round_to = 8;

	check_unmarshalled_cfg("tst/yaml/cfg-scale-round-to-zero.yaml", expected, "tst/yaml/cfg-scale-round-to-zero.log");

	assert_logs_empty();
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
			"ipc request: invalid document, expected map, got sequence\n"
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
			"ipc request: invalid OP aoeu, expected enum GET|LIST|REAPPLY|CFG_SET|CFG_DEL|CFG_WRITE|CFG_TOGGLE\n"
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
			"ipc request: invalid OP, expected enum GET|LIST|REAPPLY|CFG_SET|CFG_DEL|CFG_WRITE|CFG_TOGGLE\n"
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
			"ipc request: invalid document, expected OP\n"
			"========================================\n"
			"FOO: BAR\n"
			"----------------------------------------\n");

	assert_logs_empty();
}

static void yaml_root_to_ipc_request__invalid_cfg(void **state) {
	struct Cfg *expected = cfg_default_scalars();

	char *yaml = read_file("tst/yaml/ipc-request-cfg-invalid.yaml");

	struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_non_nul(actual);
	assert_int_equal(actual->command, CFG_SET);
	assert_int_equal(actual->log_threshold, ERROR);

	assert_cfg_equal(actual->cfg, expected);

	char *expected_log = read_file("tst/yaml/ipc-request-cfg-invalid.log");
	assert_log(WARNING, expected_log);

	free(yaml);
	ipc_request_free(actual);
	cfg_free(expected);
	free(expected_log);

	assert_logs_empty();
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

static void yaml_root_to_ipc_response_plist__empty(void **state) {
	assert_nul(yaml_unmarshal_str("", yaml_root_to_ipc_response_plist, "ipc response"));

	assert_log(ERROR, "\n"
			"ipc response: empty request\n"
			"========================================\n"
			"\n"
			"----------------------------------------\n");

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_plist__mistyped_root(void **state) {
	assert_nul(yaml_unmarshal_str("foo", yaml_root_to_ipc_response_plist, "ipc response"));

	assert_log(ERROR,
			"ipc response: invalid document, expected sequence or map, got scalar\n"
			"========================================\n"
			"foo\n"
			"----------------------------------------\n");

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_plist__seq_no_map(void **state) {
	assert_nul(yaml_unmarshal_str("-", yaml_root_to_ipc_response_plist, "ipc response"));

	assert_log(ERROR,
			"ipc response: invalid document, expected map, got scalar\n"
			"========================================\n"
			"-\n"
			"----------------------------------------\n");

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_plist__seq_no_done(void **state) {
	expect_function_call(__wrap_lid_free);

	assert_nul(yaml_unmarshal_str("- FOO: BAR", yaml_root_to_ipc_response_plist, "ipc response"));

	assert_log(ERROR,
			"ipc response: invalid document, expected DONE\n"
			"========================================\n"
			"- FOO: BAR\n"
			"----------------------------------------\n");

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_plist__seq_no_rc(void **state) {
	expect_function_call(__wrap_lid_free);

	const struct Pset *actual = yaml_unmarshal_str( "- DONE: TRUE", yaml_root_to_ipc_response_plist, "ipc response");

	assert_nul(actual);

	assert_log(ERROR,
			"ipc response: invalid document, expected RC\n"
			"========================================\n"
			"- DONE: TRUE\n"
			"----------------------------------------\n");

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_plist__map(void **state) {
	char *yaml = read_file("tst/yaml/ipc-responses-map.yaml");

	expect_function_call(__wrap_lid_free);

	const struct Plist *responses = yaml_unmarshal_str(yaml, yaml_root_to_ipc_response_plist, "ipc response");

	assert_non_nul(responses);
	assert_int_equal(plist_size(responses), 1);

	const struct IpcResponse *response = plist_at(responses, 0);

	assert_true(response->status.done);
	assert_int_equal(response->status.rc, 2);

	assert_non_nul(response->lid);
	assert_true(response->lid->closed);
	assert_str_equal(response->lid->device_path, "/path/to/lid");

	assert_non_nul(response->cfg);
	struct Cfg *expected_cfg = cfg_all();
	assert_cfg_equal(response->cfg, expected_cfg);

	assert_int_equal(plist_size(response->heads), 1);
	const struct Head *head = plist_at(response->heads, 0);

	assert_str_equal(head->name, "name");
	assert_str_equal(head->description, "desc");
	assert_int_equal(head->width_mm, 1);
	assert_int_equal(head->height_mm, 2);
	assert_str_equal(head->make, "make");
	assert_str_equal(head->model, "model");
	assert_str_equal(head->serial_number, "serial");

	assert_int_equal(head->cur.scale, wl_fixed_from_double(4));
	assert_true(head->cur.enabled);
	assert_int_equal(head->cur.x, 5);
	assert_int_equal(head->cur.y, 6);
	assert_int_equal(head->cur.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED);

	assert_int_equal(head->des.scale, wl_fixed_from_double(7.0));
	assert_true(head->des.enabled);
	assert_int_equal(head->des.x, 8);
	assert_int_equal(head->des.y, 9);
	assert_int_equal(head->des.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	const struct Mode *mode_current = ppmap_get(head->modes, head->cur.zmode);
	assert_non_nul(mode_current);
	assert_int_equal(mode_current->width, 10);
	assert_int_equal(mode_current->height, 11);
	assert_int_equal(mode_current->refresh_mhz, 12);

	const struct Mode *mode_desired = ppmap_get(head->modes, head->des.zmode);
	assert_non_nul(mode_desired);
	assert_int_equal(mode_desired->width, 13);
	assert_int_equal(mode_desired->height, 14);
	assert_int_equal(mode_desired->refresh_mhz, 15);

	assert_int_equal(ppmap_size(head->modes), 2);
	const struct Mode *mode1 = ppmap_at(head->modes, 0).val;
	assert_non_nul(mode1);
	assert_int_equal(mode1->width, 10);
	assert_int_equal(mode1->height, 11);
	assert_int_equal(mode1->refresh_mhz, 12);

	const struct Mode *mode2 = ppmap_at(head->modes, 1).val;
	assert_non_nul(mode2);
	assert_int_equal(mode2->width, 13);
	assert_int_equal(mode2->height, 14);
	assert_int_equal(mode2->refresh_mhz, 15);

	const struct Mode *mode_pref = ppmap_get(head->modes, head->zmode_pref);
	assert_non_nul(mode_pref);
	assert_int_equal(mode_pref->width, 10);
	assert_int_equal(mode_pref->height, 11);
	assert_int_equal(mode_pref->refresh_mhz, 12);

	assert_int_equal(ppmap_size(head->modes_failed), 1);

	const struct Mode *mode_failed = ppmap_at(head->modes_failed, 0).val;
	assert_non_nul(mode_failed);
	assert_int_equal(mode_failed->width, 16);
	assert_int_equal(mode_failed->height, 17);
	assert_int_equal(mode_failed->refresh_mhz, 18);

	assert_int_equal(head->cur.transform, 3);
	assert_int_equal(head->des.transform, 4);

	assert_int_equal(plist_size(response->log_cap_lines), 3);
	const struct LogCapLine *line = plist_at(response->log_cap_lines, 0);
	assert_int_equal(line->threshold, WARNING);
	assert_str_equal(line->line, "war");

	line = plist_at(response->log_cap_lines, 1);
	assert_int_equal(line->threshold, ERROR);
	assert_str_equal(line->line, "err");

	line = plist_at(response->log_cap_lines, 2);
	assert_non_nul(line);
	assert_int_equal(line->threshold, FATAL);
	assert_str_equal(line->line, "fat");

	assert_int_equal(head->overrided_enabled, OverrideFalse);

	plist_free_vals(responses);
	cfg_free(expected_cfg);
	free(yaml);

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_plist__seq(void **state) {
	char *yaml = read_file("tst/yaml/ipc-responses-seq-brief.yaml");

	expect_function_calls(__wrap_lid_free, 3);

	const struct Plist *responses = yaml_unmarshal_str(yaml, yaml_root_to_ipc_response_plist, "ipc response");

	struct Cfg *cfg_expected = cfg_init();
	cfg_expected->arrange = COL;

	assert_non_nul(responses);
	assert_int_equal(plist_size(responses), 3);

	// 0
	const struct IpcResponse *response = plist_at(responses, 0);
	assert_true(response->status.done);
	assert_int_equal(response->status.rc, 0);

	const struct Cfg *cfg_actual = response->cfg;
	assert_non_nul(cfg_actual);
	assert_cfg_equal(cfg_actual, cfg_expected);

	const struct Lid *lid = response->lid;
	assert_non_nul(lid);
	assert_str_equal(lid->device_path, "/path/to/lid");

	assert_int_equal(plist_size(response->heads), 2);

	const struct Head *head0 = plist_at(response->heads, 0);
	assert_non_nul(head0);
	assert_str_equal(head0->name, "name0");
	assert_int_equal(head0->overrided_enabled, NoOverride);

	const struct Head *head1 = plist_at(response->heads, 1);
	assert_non_nul(head1);
	assert_str_equal(head1->name, "name1");
	assert_int_equal(head1->overrided_enabled, NoOverride);

	assert_int_equal(plist_size(response->log_cap_lines), 4);
	const struct LogCapLine *line = plist_at(response->log_cap_lines, 0);
	assert_int_equal(line->threshold, DEBUG);
	assert_str_equal(line->line, "dbg0");

	line = plist_at(response->log_cap_lines, 1);
	assert_int_equal(line->threshold, INFO);
	assert_str_equal(line->line, "inf0");

	line = plist_at(response->log_cap_lines, 2);
	assert_int_equal(line->threshold, WARNING);
	assert_str_equal(line->line, "war0");

	line = plist_at(response->log_cap_lines, 3);
	assert_int_equal(line->threshold, ERROR);
	assert_str_equal(line->line, "err0");

	// 1
	response = plist_at(responses, 1);
	assert_false(response->status.done);
	assert_int_equal(response->status.rc, 1);
	assert_nul(response->cfg);
	assert_nul(response->lid);
	assert_int_equal(plist_size(response->heads), 0);

	assert_int_equal(plist_size(response->log_cap_lines), 4);
	line = plist_at(response->log_cap_lines, 0);
	assert_int_equal(line->threshold, DEBUG);
	assert_str_equal(line->line, "dbg1");

	line = plist_at(response->log_cap_lines, 1);
	assert_int_equal(line->threshold, INFO);
	assert_str_equal(line->line, "inf1");

	line = plist_at(response->log_cap_lines, 2);
	assert_int_equal(line->threshold, WARNING);
	assert_str_equal(line->line, "war1");

	line = plist_at(response->log_cap_lines, 3);
	assert_int_equal(line->threshold, ERROR);
	assert_str_equal(line->line, "err1");

	// 2
	response = plist_at(responses, 2);
	assert_true(response->status.done);
	assert_int_equal(response->status.rc, 2);
	assert_nul(response->cfg);
	assert_nul(response->lid);
	assert_int_equal(plist_size(response->heads), 0);
	assert_int_equal(plist_size(response->log_cap_lines), 0);

	plist_free_vals(responses);
	free(yaml);
	cfg_free(cfg_expected);

	assert_logs_empty();
}

static void yaml_unmarshal_str__yaml_parser_initialize_fail(void **state) {

	will_return_int(__wrap_yaml_parser_initialize, 0);

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

	free(err);

	assert_logs_empty();
}

static void yaml_unmarshal_file__yaml_parser_initialize_fail(void **state) {
	will_return_int(__wrap_yaml_parser_initialize, 0);

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
		TEST(yaml_root_to_cfg__ok),
		TEST(yaml_root_to_cfg__empty_arrays),
		TEST(yaml_root_to_cfg__unknown_fields_warn),
		TEST(yaml_root_to_cfg__missing),
		TEST(yaml_root_to_cfg__invalid),
		TEST(yaml_root_to_cfg__mistyped),
		TEST(yaml_root_to_cfg__root_mistyped),
		TEST(yaml_root_to_cfg__transform),
		TEST(yaml_root_to_cfg__scale),
		TEST(yaml_root_to_cfg__mode),
		TEST(yaml_root_to_cfg__disabled),
		TEST(yaml_root_to_cfg__disabled__disableds_empty),
		TEST(yaml_root_to_cfg__scale_round_to_invalid),
		TEST(yaml_root_to_cfg__scale_round_to_zero),

		TEST(yaml_root_to_ipc_request__empty),
		TEST(yaml_root_to_ipc_request__mistyped_root),
		TEST(yaml_root_to_ipc_request__invalid_op),
		TEST(yaml_root_to_ipc_request__mistyped_op),
		TEST(yaml_root_to_ipc_request__no_op),
		TEST(yaml_root_to_ipc_request__invalid_cfg),
		TEST(yaml_root_to_ipc_request__cfg_set),

		TEST(yaml_root_to_ipc_response_plist__empty),
		TEST(yaml_root_to_ipc_response_plist__mistyped_root),
		TEST(yaml_root_to_ipc_response_plist__seq_no_map),
		TEST(yaml_root_to_ipc_response_plist__seq_no_done),
		TEST(yaml_root_to_ipc_response_plist__seq_no_rc),
		TEST(yaml_root_to_ipc_response_plist__map),
		TEST(yaml_root_to_ipc_response_plist__seq),

		TEST(yaml_unmarshal_str__yaml_parser_initialize_fail),
		TEST(yaml_unmarshal_str__yaml_parser_load_fail),
		TEST(yaml_unmarshal_file__yaml_parser_initialize_fail),
		TEST(yaml_unmarshal_file__yaml_parser_load_fail),
	};

	return RUN(tests);
}


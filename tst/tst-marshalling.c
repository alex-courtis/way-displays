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
#include "global.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "slist.h"
#include "log.h"
#include "mode.h"
#include "wrap-libyaml.h"
#include "wlr-output-management-unstable-v1.h"

#include "marshalling.h"
#include "yaml-marshal-cfg.h"
#include "yaml-marshal-ipc-request.h"
#include "yaml-marshal-ipc-response.h"

#ifdef V2
#define V2 true
#define C_T_Y cfg_to_yaml
#define IREQ_T_Y ipc_request_to_yaml
#define IRES_T_Y ipc_response_to_yaml
#define YF_T_C yaml_file_into_cfg
#define Y_T_IREQ yaml_to_ipc_request
#define Y_T_IRES yaml_to_ipc_responses
#else
#define V2 false
#define C_T_Y marshal_cfg
#define IREQ_T_Y marshal_ipc_request
#define IRES_T_Y marshal_ipc_response
#define YF_T_C unmarshal_cfg_from_file
#define Y_T_IREQ unmarshal_ipc_request
#define Y_T_IRES unmarshal_ipc_responses
#endif

static void lcl(enum LogThreshold threshold, char *line, struct SList **log_cap_lines) {
	struct LogCapLine *lcl = calloc(1, sizeof(struct LogCapLine));

	lcl->threshold = threshold;
	lcl->line = strdup(line);

	slist_append(log_cap_lines, lcl);
}

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
	cfg_free(cfg);
	cfg = NULL;
	free(lid);
	lid = NULL;
	return 0;
}


// cfg-all.yaml
struct Cfg *cfg_all(void) {
	struct Cfg *cfg = cfg_default();

	cfg->arrange = COL;
	cfg->align = BOTTOM;
	cfg->scaling = OFF;
	cfg->auto_scale = OFF;
	cfg->log_threshold = ERROR;

	cfg->auto_scale_min = 0.5f;
	cfg->auto_scale_max = 2.5f;

	free(cfg->callback_cmd);
	cfg->callback_cmd = strdup("cmd");
	cfg->laptop_display_prefix = strdup("ldp");

	slist_append(&cfg->order_name_desc, strdup("one"));
	slist_append(&cfg->order_name_desc, strdup("ONE"));
	slist_append(&cfg->order_name_desc, strdup("!two"));

	slist_append(&cfg->user_scales, cfg_user_scale_init("three", 3));
	slist_append(&cfg->user_scales, cfg_user_scale_init("four", 4));

	slist_append(&cfg->user_modes, cfg_user_mode_init("five", false, 1920, 1080, 12340, false));
	slist_append(&cfg->user_modes, cfg_user_mode_init("six", false, 2560, 1440, -1, false));
	slist_append(&cfg->user_modes, cfg_user_mode_init("seven", true, -1, -1, -1, false));

	slist_append(&cfg->adaptive_sync_off_name_desc, strdup("ten"));
	slist_append(&cfg->adaptive_sync_off_name_desc, strdup("ELEVEN"));

	slist_append(&cfg->disabled, cfg_disabled_always("eight"));
	slist_append(&cfg->disabled, cfg_disabled_always("EIGHT"));
	slist_append(&cfg->disabled, cfg_disabled_always("nine"));

	struct Disabled *disabled = calloc(1, sizeof(struct Disabled));
	disabled->name_desc = strdup("twelve");

	struct Condition *cond = calloc(1, sizeof(struct Condition));
	slist_append(&cond->plugged, strdup("ONE"));
	slist_append(&cond->plugged, strdup("TWO"));
	slist_append(&disabled->conditions, cond);

	cond = calloc(1, sizeof(struct Condition));
	slist_append(&cond->unplugged, strdup("THREE"));
	slist_append(&disabled->conditions, cond);

	slist_append(&cfg->disabled, disabled);

	slist_append(&cfg->user_transforms, cfg_user_transform_init("twelve", WL_OUTPUT_TRANSFORM_FLIPPED));

	return cfg;
}

struct Cfg *cfg_non_default(void) {
	struct Cfg *def = cfg_init();

	def->arrange = COL;
	def->align = RIGHT;
	def->scaling = OFF;
	def->auto_scale = OFF;
	def->auto_scale_min = 88;
	def->auto_scale_max = 99;
	def->callback_cmd = strdup("FOOBARBAZ");

	return def;
}

// ipc-responses-map.yaml and ipc-responses-seq.yaml
struct IpcOperation *ipc_response(void) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));
	ipc_request->log_threshold = WARNING;
	ipc_request->command = GET;

	struct IpcOperation *ipc_operation = calloc(1, sizeof(struct IpcOperation));
	ipc_operation->request = ipc_request;
	ipc_operation->done = true;
	ipc_operation->rc = 1;
	ipc_operation->send_state = true;

	cfg = cfg_all();

	lid = calloc(1, sizeof(struct Lid));
	lid->closed = true;
	lid->device_path = "/path/to/lid";

	lcl(DEBUG, "dbg", &ipc_operation->log_cap_lines);
	lcl(INFO, "inf", &ipc_operation->log_cap_lines);
	lcl(WARNING, "war", &ipc_operation->log_cap_lines);
	lcl(ERROR, "err", &ipc_operation->log_cap_lines);
	lcl(FATAL, "fat", &ipc_operation->log_cap_lines);

	struct Head *head0 = calloc(1, sizeof(struct Head));

	head0->name = strdup("name");
	head0->description = strdup("desc");
	head0->width_mm = 1;
	head0->height_mm = 2;
	head0->make = strdup("make");
	head0->model = strdup("model");
	head0->serial_number = strdup("serial");
	head0->overrided_enabled = true;

	head0->current.scale = wl_fixed_from_double(4.0);
	head0->current.enabled = true;
	head0->current.x = 5;
	head0->current.y = 6;
	head0->current.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	head0->current.transform = WL_OUTPUT_TRANSFORM_270;

	head0->current.mode = mode_init(NULL, NULL, 10, 11, 12, true);
	slist_append(&head0->modes, head0->current.mode);

	head0->desired.scale = wl_fixed_from_double(7.0);
	head0->desired.enabled = true;
	head0->desired.x = 8;
	head0->desired.y = 9;
	head0->desired.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	head0->desired.transform = WL_OUTPUT_TRANSFORM_FLIPPED;

	head0->desired.mode = mode_init(NULL, NULL, 13, 14, 15, false);;
	slist_append(&head0->modes, head0->desired.mode);

	slist_append(&heads, head0);

	return ipc_operation;
}

static void yaml_file_to_cfg__ok(void **state) {

	struct Cfg *read = cfg_non_default();
	read->file_path = strdup("tst/marshalling/cfg-all.yaml");

	assert_true(YF_T_C(read));

	struct Cfg *expected = cfg_all();

	assert_cfg_equal(read, expected);

	cfg_free(read);
	cfg_free(expected);

	assert_logs_empty();
}

static void yaml_file_to_cfg__empty(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/marshalling/cfg-empty.yaml");

	assert_false(YF_T_C(read));

	if (V2)
		assert_log(ERROR, "\nparsing file tst/marshalling/cfg-empty.yaml no root node\n");
	else
		assert_log(ERROR, "\nparsing file tst/marshalling/cfg-empty.yaml empty cfg, expected map\n");
	assert_logs_empty();

	cfg_free(read);
}

static void yaml_file_to_cfg__missing(void **state) {
	struct Cfg *read = cfg_default();
	read->file_path = strdup("foo/bar/baz.yaml");

	assert_false(YF_T_C(read));

	if (V2)
		assert_log(ERROR, "\nparsing file foo/bar/baz.yaml: inexistent\n");
	else
		assert_log(ERROR, "\nparsing file foo/bar/baz.yaml bad file: foo/bar/baz.yaml\n");
	assert_logs_empty();

	cfg_free(read);
}

static void yaml_file_to_cfg__invalid(void **state) {

	struct Cfg *read = cfg_non_default();
	read->file_path = strdup("tst/marshalling/cfg-invalid.yaml");

	assert_true(YF_T_C(read));

	struct Cfg *expected = cfg_default();
	slist_append(&expected->disabled, cfg_disabled_always("BAD_DISABLED_IFS"));

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/marshalling/cfg-invalid.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_file_to_cfg__mistyped(void **state) {
	if (!V2)
		return;

	struct Cfg *read = cfg_non_default();
	read->file_path = strdup("tst/marshalling/cfg-mistyped.yaml");

	assert_true(YF_T_C(read));

	struct Cfg *expected = cfg_default();

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/marshalling/cfg-mistyped.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_file_to_cfg__transform(void **state) {
	if (!V2)
		return;

	struct Cfg *read = cfg_init();
	read->file_path = strdup("tst/marshalling/cfg-transform.yaml");

	assert_true(YF_T_C(read));

	struct Cfg *expected = cfg_init();
	slist_append(&expected->user_transforms, cfg_user_transform_init("one", WL_OUTPUT_TRANSFORM_FLIPPED));

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/marshalling/cfg-transform.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_file_to_cfg__scale(void **state) {
	if (!V2)
		return;

	struct Cfg *read = cfg_init();
	read->file_path = strdup("tst/marshalling/cfg-scale.yaml");

	assert_true(YF_T_C(read));

	struct Cfg *expected = cfg_init();
	slist_append(&expected->user_scales, cfg_user_scale_init("three", 3));

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/marshalling/cfg-scale.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_file_to_cfg__mode(void **state) {
	if (!V2)
		return;

	struct Cfg *read = cfg_init();
	read->file_path = strdup("tst/marshalling/cfg-mode.yaml");

	assert_true(YF_T_C(read));

	struct Cfg *expected = cfg_init();
	slist_append(&expected->user_modes, cfg_user_mode_init("max_override", true, 1920, 1080, 12340, false));
	slist_append(&expected->user_modes, cfg_user_mode_init("five", false, 1920, 1080, 12340, false));
	slist_append(&expected->user_modes, cfg_user_mode_init("seven", true, -1, -1, -1, false));

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/marshalling/cfg-mode.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_file_to_cfg__disabled(void **state) {
	if (!V2)
		return;

	struct Cfg *read = cfg_init();
	read->file_path = strdup("tst/marshalling/cfg-disabled.yaml");

	assert_true(YF_T_C(read));

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

	char *expected_log = read_file("tst/marshalling/cfg-disabled.log");
	assert_log(WARNING, expected_log);
	assert_logs_empty();

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

static void yaml_file_to_cfg__callback_cmd_empty(void **state) {
	struct Cfg *read = cfg_init();
	read->file_path = strdup("tst/marshalling/cfg-callback-cmd-empty.yaml");
	read->callback_cmd = strdup("FOOBAR");

	assert_true(YF_T_C(read));

	assert_nul(read->callback_cmd);

	struct Cfg *expected = cfg_init();

	assert_cfg_equal(read, expected);

	cfg_free(read);
	cfg_free(expected);

	assert_logs_empty();
}

static void yaml_file_to_cfg__yaml_parser_initialize_fail(void **state) {
	if (!V2)
		return;

	struct Cfg *read = cfg_init();

	read->file_path = strdup("tst/marshalling/cfg-all.yaml");

	yaml_parser_initialize__fail = true;

	assert_false(YF_T_C(read));

	assert_log(ERROR, "\nparsing file tst/marshalling/cfg-all.yaml: yaml_parser_initialize failed\n");
	assert_logs_empty();

	cfg_free(read);
}

static void yaml_file_to_cfg__yaml_parser_load_fail(void **state) {
	if (!V2)
		return;

	struct Cfg *read = cfg_init();

	read->file_path = strdup("tst/marshalling/cfg-all.yaml");

	yaml_parser_load__fail = true;

	assert_false(YF_T_C(read));

	assert_log(ERROR, "\nparsing file tst/marshalling/cfg-all.yaml: yaml_parser_load failed\n");
	assert_logs_empty();

	cfg_free(read);
}

static void yaml_file_to_cfg__legacy(void **state) {
	struct Cfg *read = cfg_init();
	read->file_path = strdup("tst/marshalling/cfg-legacy.yaml");

	assert_true(YF_T_C(read));

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

static void cfg_to_yaml__ok(void **state) {
	struct Cfg *cfg = cfg_all();

	char *actual = C_T_Y(cfg);

	assert_non_nul(actual);

	char *expected = V2 ? read_file("tst/marshalling/cfg-all.yaml") : read_file("tst/marshalling/cfg-all-v1.yaml");

	if (strcmp(actual, expected) != 0) {
		write_file("actual.yaml", actual);
		write_file("expected.yaml", expected);
	}

	assert_str_equal(actual, expected);

	cfg_free(cfg);
	free(actual);
	free(expected);

	assert_logs_empty();
}

static void cfg_to_yaml__default(void **state) {
	struct Cfg *cfg = cfg_default();

	char *actual = C_T_Y(cfg);

	assert_non_nul(actual);

	char *expected = read_file("tst/marshalling/cfg-default.yaml");

	if (strcmp(actual, expected) != 0) {
		write_file("actual.yaml", actual);
		write_file("expected.yaml", expected);
	}

	assert_str_equal(actual, expected);

	cfg_free(cfg);
	free(actual);
	free(expected);

	assert_logs_empty();
}

static void cfg_to_yaml__yaml_document_initialize_fail(void **state) {
	if (!V2)
		return;

	struct Cfg *cfg = cfg_all();

	yaml_document_initialize__fail = true;

	char *actual = C_T_Y(cfg);

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_document_initialize failed\n");
	assert_logs_empty();
}

static void cfg_to_yaml__yaml_document_add_mapping_fail(void **state) {
	if (!V2)
		return;

	struct Cfg *cfg = cfg_all();

	yaml_document_add_mapping__fail = true;

	char *actual = C_T_Y(cfg);

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_document_add_mapping for root failed\n");
	assert_logs_empty();
}

static void cfg_to_yaml__yaml_emitter_initialize_fail(void **state) {
	if (!V2)
		return;

	struct Cfg *cfg = cfg_all();

	yaml_emitter_initialize__fail = true;

	char *actual = C_T_Y(cfg);

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_initialize failed\n");
	assert_logs_empty();
}

static void cfg_to_yaml__yaml_emitter_open_fail(void **state) {
	if (!V2)
		return;

	struct Cfg *cfg = cfg_all();

	yaml_emitter_dump__fail = true;

	char *actual = C_T_Y(cfg);

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_dump failed\n");
	assert_logs_empty();
}

// also covers case of write_handler fail
static void cfg_to_yaml__yaml_emitter_dump_fail(void **state) {
	if (!V2)
		return;

	struct Cfg *cfg = cfg_all();

	yaml_emitter_dump__fail = true;

	char *actual = C_T_Y(cfg);

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_dump failed\n");
	assert_logs_empty();
}

static void cfg_to_yaml__yaml_emitter_close_fail(void **state) {
	if (!V2)
		return;

	struct Cfg *cfg = cfg_all();

	yaml_emitter_close__fail = true;

	char *actual = C_T_Y(cfg);

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(WARNING, "unable to marshal cfg: yaml_emitter_close failed\n");
	assert_logs_empty();
}

static void ipc_request_to_yaml__no_op(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));

	assert_nul(IREQ_T_Y(ipc_request));

	if (V2) {
		assert_log(ERROR, "unable to marshal ipc request: missing OP\n");
	} else {
		assert_log(ERROR, "marshalling ipc request: missing OP\n");
	}
	assert_logs_empty();

	ipc_request_free(ipc_request);
}

static void ipc_request_to_yaml__cfg_set(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));
	ipc_request->command = CFG_SET;
	ipc_request->log_threshold = ERROR;

	ipc_request->cfg = cfg_all();

	char *actual = IREQ_T_Y(ipc_request);

	assert_non_nul(actual);

	char *expected;
	if (V2)
		expected = read_file("tst/marshalling/ipc-request-cfg-set.yaml");
	else
		expected = read_file("tst/marshalling/ipc-request-cfg-set-v1.yaml");

	if (strcmp(actual, expected) != 0) {
		write_file("actual.yaml", actual);
		write_file("expected.yaml", expected);
	}

	assert_str_equal(actual, expected);

	ipc_request_free(ipc_request);
	free(actual);
	free(expected);

	assert_logs_empty();
}

static void ipc_response_to_yaml__map(void **state) {
	struct IpcOperation *ipc_operation = ipc_response();

	char *actual = IRES_T_Y(ipc_operation);

	assert_non_nul(actual);

	char *expected;
	if (V2)
		expected = read_file("tst/marshalling/ipc-responses-map.yaml");
	else
		expected = read_file("tst/marshalling/ipc-responses-map-v1.yaml");

	if (strcmp(actual, expected) != 0) {
		write_file("actual.yaml", actual);
		write_file("expected.yaml", expected);
	}

	assert_str_equal(actual, expected);

	ipc_operation_free(ipc_operation);

	free(actual);
	free(expected);

	slist_free_vals(&heads, head_free);

	assert_logs_empty();
}

static void ipc_response_to_yaml__seq(void **state) {
	struct IpcOperation *ipc_operation = ipc_response();
	ipc_operation->request->command = LIST;

	char *actual = IRES_T_Y(ipc_operation);

	assert_non_nul(actual);

	char *expected;
	if (V2)
		expected = read_file("tst/marshalling/ipc-responses-seq.yaml");
	else
		expected = read_file("tst/marshalling/ipc-responses-seq-v1.yaml");

	if (strcmp(actual, expected) != 0) {
		write_file("actual.yaml", actual);
		write_file("expected.yaml", expected);
	}

	assert_str_equal(actual, expected);

	ipc_operation_free(ipc_operation);

	free(actual);
	free(expected);

	slist_free_vals(&heads, head_free);

	assert_logs_empty();
}

static void unmarshal_ipc_request__empty(void **state) {
	struct IpcRequest *actual = Y_T_IREQ("");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc request: empty request\n"
			"========================================\n"
			"\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void unmarshal_ipc_request__mistyped_root(void **state) {
	if (!V2)
		return;

	struct IpcRequest *actual = Y_T_IREQ("- FOO");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc request: expected map, got sequence\n"
			"========================================\n"
			"- FOO\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void unmarshal_ipc_request__invalid_op(void **state) {
	struct IpcRequest *actual = Y_T_IREQ("OP: aoeu");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc request: invalid OP aoeu\n"
			"========================================\n"
			"OP: aoeu\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void unmarshal_ipc_request__mistyped_op(void **state) {
	if (!V2)
		return;

	struct IpcRequest *actual = Y_T_IREQ("OP:\n  FOO: BAR");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc request: invalid OP expected scalar, got map\n"
			"========================================\n"
			"OP:\n"
			"  FOO: BAR\n"
			"----------------------------------------\n");
	assert_logs_empty();
}


static void unmarshal_ipc_request__no_op(void **state) {
	struct IpcRequest *actual = Y_T_IREQ("FOO: BAR");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc request: missing OP\n"
			"========================================\n"
			"FOO: BAR\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void unmarshal_ipc_request__cfg_set(void **state) {
	char *yaml = read_file("tst/marshalling/ipc-request-cfg-set.yaml");

	struct IpcRequest *actual = Y_T_IREQ(yaml);

	assert_non_nul(actual);
	assert_int_equal(actual->command, CFG_SET);

	struct Cfg *expected_cfg = cfg_all();

	assert_cfg_equal(actual->cfg, expected_cfg);

	assert_int_equal(actual->log_threshold, ERROR);

	ipc_request_free(actual);
	cfg_free(expected_cfg);
	free(yaml);

	assert_logs_empty();
}

static void unmarshal_ipc_responses__empty(void **state) {
	struct SList *actual = Y_T_IRES("");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc response: empty request\n"
			"========================================\n"
			"\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void unmarshal_ipc_responses__mistyped_root(void **state) {
	if (!V2)
		return;

	struct SList *actual = Y_T_IRES("foo");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc response: expected map or sequence, got scalar\n"
			"========================================\n"
			"foo\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void unmarshal_ipc_responses__seq_no_map(void **state) {
	struct SList *actual = Y_T_IRES("-");

	assert_nul(actual);

	if (V2) {
		assert_log(ERROR, "\n"
				"unmarshalling ipc response: expected map, got scalar\n"
				"========================================\n"
				"-\n"
				"----------------------------------------\n");
	} else {
		assert_log(ERROR, "\n"
				"unmarshalling ipc response: expected map\n"
				"========================================\n"
				"-\n"
				"----------------------------------------\n");
	}
	assert_logs_empty();
}

static void unmarshal_ipc_responses__seq_no_done(void **state) {
	if (V2)
		expect_function_call(__wrap_lid_free);

	struct SList *actual = Y_T_IRES("- FOO: BAR");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc response: missing DONE\n"
			"========================================\n"
			"- FOO: BAR\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void unmarshal_ipc_responses__seq_no_rc(void **state) {
	if (V2)
		expect_function_call(__wrap_lid_free);

	struct SList *actual = Y_T_IRES("- DONE: TRUE");

	assert_nul(actual);

	assert_log(ERROR, "\n"
			"unmarshalling ipc response: missing RC\n"
			"========================================\n"
			"- DONE: TRUE\n"
			"----------------------------------------\n");
	assert_logs_empty();
}

static void unmarshal_ipc_responses__map(void **state) {
	char *yaml = read_file("tst/marshalling/ipc-responses-map.yaml");

	expect_function_call(__wrap_lid_free);

	struct SList *responses = Y_T_IRES(yaml);

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

	// TODO log message validation
	//
	// assert_int_equal(slist_length(response->log_cap_lines), 3);
	//
	// struct LogCapLine *line = slist_at(response->log_cap_lines, 0);
	// assert_non_nul(line);
	// assert_int_equal(line->threshold, WARNING);
	// assert_str_equal(line->line, "war");
	//
	// line = slist_at(response->log_cap_lines, 1);
	// assert_non_nul(line);
	// assert_int_equal(line->threshold, ERROR);
	// assert_str_equal(line->line, "err");

	assert_int_equal(head->overrided_enabled, OverrideFalse);

	slist_free_vals(&responses, ipc_response_free);
	cfg_free(expected_cfg);
	free(yaml);

	assert_logs_empty();
}

static void unmarshal_ipc_responses__seq(void **state) {
	char *yaml = read_file("tst/marshalling/ipc-responses-seq-brief.yaml");

	expect_function_calls(__wrap_lid_free, 3);

	struct SList *responses = Y_T_IRES(yaml);

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

	// 1
	response = slist_at(responses, 1);
	assert_non_nul(response);
	assert_false(response->status.done);
	assert_int_equal(response->status.rc, 1);
	assert_nul(response->cfg);
	assert_nul(response->lid);
	assert_nul(response->heads);

	// 2
	response = slist_at(responses, 2);
	assert_non_nul(response);
	assert_true(response->status.done);
	assert_int_equal(response->status.rc, 2);
	assert_nul(response->cfg);
	assert_nul(response->lid);
	assert_nul(response->heads);

	// TODO log message validation

	slist_free_vals(&responses, ipc_response_free);
	free(yaml);

	assert_logs_empty();
}

int main(void) {

	// fools cppcheck
	// TODO remove
	if (false) {
		cfg_to_yaml(NULL);
		ipc_request_to_yaml(NULL);
		ipc_response_to_yaml(NULL);
		yaml_file_into_cfg(NULL);
		yaml_to_ipc_request(NULL);
		yaml_to_ipc_responses(NULL);

		unmarshal_ipc_request__empty(NULL);
		unmarshal_ipc_request__invalid_op(NULL);
		unmarshal_ipc_request__no_op(NULL);
		unmarshal_ipc_request__cfg_set(NULL);

		unmarshal_ipc_responses__map(NULL);
	}

	const struct CMUnitTest tests[] = {
		TEST(yaml_file_to_cfg__ok),

		TEST(yaml_file_to_cfg__empty),
		TEST(yaml_file_to_cfg__missing),

		TEST(yaml_file_to_cfg__invalid),
		TEST(yaml_file_to_cfg__legacy),
		TEST(yaml_file_to_cfg__mistyped),
		TEST(yaml_file_to_cfg__transform),
		TEST(yaml_file_to_cfg__scale),
		TEST(yaml_file_to_cfg__mode),
		TEST(yaml_file_to_cfg__disabled),
		TEST(yaml_file_to_cfg__callback_cmd_empty),

		TEST(yaml_file_to_cfg__yaml_parser_initialize_fail),
		TEST(yaml_file_to_cfg__yaml_parser_load_fail),

		TEST(cfg_to_yaml__ok),
		TEST(cfg_to_yaml__default),

		TEST(cfg_to_yaml__yaml_document_initialize_fail),
		TEST(cfg_to_yaml__yaml_document_add_mapping_fail),
		TEST(cfg_to_yaml__yaml_emitter_initialize_fail),
		TEST(cfg_to_yaml__yaml_emitter_open_fail),
		TEST(cfg_to_yaml__yaml_emitter_dump_fail),
		TEST(cfg_to_yaml__yaml_emitter_close_fail),

		TEST(ipc_request_to_yaml__no_op),
		TEST(ipc_request_to_yaml__cfg_set),

		TEST(ipc_response_to_yaml__map),
		TEST(ipc_response_to_yaml__seq),

		TEST(unmarshal_ipc_request__empty),
		TEST(unmarshal_ipc_request__mistyped_root),
		TEST(unmarshal_ipc_request__invalid_op),
		TEST(unmarshal_ipc_request__mistyped_op),
		TEST(unmarshal_ipc_request__no_op),
		TEST(unmarshal_ipc_request__cfg_set),

		TEST(unmarshal_ipc_responses__empty),
		TEST(unmarshal_ipc_responses__mistyped_root),
		TEST(unmarshal_ipc_responses__seq_no_map),
		TEST(unmarshal_ipc_responses__seq_no_done),
		TEST(unmarshal_ipc_responses__seq_no_rc),
		TEST(unmarshal_ipc_responses__map),
		TEST(unmarshal_ipc_responses__seq),
	};

	return RUN(tests);
}


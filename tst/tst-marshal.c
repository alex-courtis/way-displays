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
#include "log.h"
#include "mode.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"
#include "wrap-libyaml.h"

#include "yaml/marshal.h"
#include "yaml/marshal-types.h"

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

static void cfg_to_yaml__ok(void **state) {
	struct Cfg *cfg = cfg_all();

	char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_non_nul(actual);

	char *expected = read_file("tst/marshalling/cfg-all.yaml");

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

	char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

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

static void cfg_to_yaml__empty(void **state) {
	struct Cfg *cfg = cfg_init();

	char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_non_nul(actual);

	char *expected = read_file("tst/marshalling/empty.yaml");

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

	struct Cfg *cfg = cfg_all();

	yaml_document_initialize__fail = true;

	char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_document_initialize failed\n");
	assert_logs_empty();
}

static void cfg_to_yaml__yaml_emitter_initialize_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_emitter_initialize__fail = true;

	char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_initialize failed\n");
	assert_logs_empty();
}

static void cfg_to_yaml__yaml_emitter_open_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_emitter_dump__fail = true;

	char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_dump failed\n");
	assert_logs_empty();
}

// also covers case of write_handler fail
static void cfg_to_yaml__yaml_emitter_dump_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_emitter_dump__fail = true;

	char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_dump failed\n");
	assert_logs_empty();
}

static void cfg_to_yaml__yaml_emitter_close_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_emitter_close__fail = true;

	char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(WARNING, "unable to marshal cfg: yaml_emitter_close failed\n");
	assert_logs_empty();
}

static void ipc_request_to_yaml__no_op(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));

	assert_nul(yaml_marshal(ipc_request, yaml_doc_ipc_request, "ipc request"));

	assert_log(ERROR, "unable to marshal ipc request: missing OP\n");

	assert_logs_empty();

	ipc_request_free(ipc_request);
}

static void ipc_request_to_yaml__cfg_set(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));
	ipc_request->command = CFG_SET;
	ipc_request->log_threshold = ERROR;

	ipc_request->cfg = cfg_all();

	char *actual = yaml_marshal(ipc_request, yaml_doc_ipc_request, "ipc request");

	char *expected = read_file("tst/marshalling/ipc-request-cfg-set.yaml");

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

	char *actual = actual = yaml_marshal(ipc_operation, yaml_doc_ipc_operation, "ipc response");

	assert_non_nul(actual);

	char *expected = read_file("tst/marshalling/ipc-responses-map.yaml");

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

	char *actual = yaml_marshal(ipc_operation, yaml_doc_ipc_operation, "ipc response");

	assert_non_nul(actual);

	char *expected = read_file("tst/marshalling/ipc-responses-seq.yaml");

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

int main(void) {

	const struct CMUnitTest tests[] = {
		TEST(cfg_to_yaml__ok),
		TEST(cfg_to_yaml__default),
		TEST(cfg_to_yaml__empty),

		TEST(cfg_to_yaml__yaml_document_initialize_fail),
		TEST(cfg_to_yaml__yaml_emitter_initialize_fail),
		TEST(cfg_to_yaml__yaml_emitter_open_fail),
		TEST(cfg_to_yaml__yaml_emitter_dump_fail),
		TEST(cfg_to_yaml__yaml_emitter_close_fail),

		TEST(ipc_request_to_yaml__no_op),
		TEST(ipc_request_to_yaml__cfg_set),

		TEST(ipc_response_to_yaml__map),
		TEST(ipc_response_to_yaml__seq),
	};

	return RUN(tests);
}


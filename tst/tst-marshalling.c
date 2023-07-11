#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "global.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "slist.h"
#include "log.h"
#include "mode.h"
#include "wlr-output-management-unstable-v1.h"

#include "marshalling.h"

void lcl(enum LogThreshold threshold, char *line) {
	struct LogCapLine *lcl = calloc(1, sizeof(struct LogCapLine));

	lcl->threshold = threshold;
	lcl->line = strdup(line);

	slist_append(&log_cap_lines, lcl);
}

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	return 0;
}

int after_each(void **state) {
	log_capture_clear();
	assert_logs_empty();
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

	slist_append(&cfg->order_name_desc, strdup("one"));
	slist_append(&cfg->order_name_desc, strdup("ONE"));
	slist_append(&cfg->order_name_desc, strdup("!two"));

	slist_append(&cfg->user_scales, cfg_user_scale_init("three", 3));
	slist_append(&cfg->user_scales, cfg_user_scale_init("four", 4));

	slist_append(&cfg->user_modes, cfg_user_mode_init("five", false, 1920, 1080, 60, false));
	slist_append(&cfg->user_modes, cfg_user_mode_init("six", false, 2560, 1440, -1, false));
	slist_append(&cfg->user_modes, cfg_user_mode_init("seven", true, -1, -1, -1, false));

	slist_append(&cfg->adaptive_sync_off_name_desc, strdup("ten"));
	slist_append(&cfg->adaptive_sync_off_name_desc, strdup("ELEVEN"));

	slist_append(&cfg->disabled_name_desc, strdup("eight"));
	slist_append(&cfg->disabled_name_desc, strdup("EIGHT"));
	slist_append(&cfg->disabled_name_desc, strdup("nine"));

	return cfg;
}

// ipc-request-get.yaml
struct IpcRequest *ipc_request_get(void) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));
	ipc_request->op = GET;

	return ipc_request;
}

void unmarshal_cfg_from_file__ok(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/marshalling/cfg-all.yaml");

	assert_true(unmarshal_cfg_from_file(read));

	struct Cfg *expected = cfg_all();

	assert_cfg_equal(read, expected);

	cfg_free(read);
	cfg_free(expected);
}

void unmarshal_cfg_from_file__empty(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/marshalling/cfg-empty.yaml");

	assert_false(unmarshal_cfg_from_file(read));

	assert_log(ERROR, "\nparsing file tst/marshalling/cfg-empty.yaml empty CFG\n");

	cfg_free(read);
}

void unmarshal_cfg_from_file__bad(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/marshalling/cfg-bad.yaml");

	assert_true(unmarshal_cfg_from_file(read));

	struct Cfg *expected = cfg_default();

	assert_cfg_equal(read, expected);

	char *expected_log = read_file("tst/marshalling/cfg-bad.log");
	assert_log(WARNING, expected_log);

	cfg_free(read);
	cfg_free(expected);
	free(expected_log);
}

void marshal_cfg__ok(void **state) {
	struct Cfg *cfg_actual = cfg_all();

	char *actual = marshal_cfg(cfg_actual);

	char *expected = read_file("tst/marshalling/cfg-all.yaml");

	assert_string_equal(actual, expected);

	cfg_free(cfg_actual);
	free(actual);
	free(expected);
}

void marshal_ipc_request__no_op(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));

	assert_null(marshal_ipc_request(ipc_request));

	assert_log(ERROR, "marshalling ipc request: missing OP\n");

	ipc_request_free(ipc_request);
}

void marshal_ipc_request__get(void **state) {
	struct IpcRequest *ipc_request = ipc_request_get();

	char *actual = marshal_ipc_request(ipc_request);

	char *expected = read_file("tst/marshalling/ipc-request-get.yaml");

	assert_string_equal(actual, expected);

	ipc_request_free(ipc_request);
	free(actual);
	free(expected);
}

void marshal_ipc_request__cfg_set(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));
	ipc_request->op = CFG_SET;

	ipc_request->cfg = cfg_all();

	char *actual = marshal_ipc_request(ipc_request);

	char *expected = read_file("tst/marshalling/ipc-request-cfg-set.yaml");

	assert_string_equal(actual, expected);

	ipc_request_free(ipc_request);
	free(actual);
	free(expected);
}

void marshal_ipc_response__ok(void **state) {
	struct IpcOperation *ipc_operation = calloc(1, sizeof(struct IpcOperation));
	ipc_operation->done = true;
	ipc_operation->rc = 1;
	ipc_operation->send_logs = true;
	ipc_operation->send_state = true;

	cfg = cfg_all();

	lid = calloc(1, sizeof(struct Lid));
	lid->closed = true;
	lid->device_path = "/path/to/lid";

	lcl(DEBUG, "dbg");
	lcl(INFO, "inf");
	lcl(WARNING, "war");
	lcl(ERROR, "err");

	struct Mode mode1 = {
		.width = 10,
		.height = 11,
		.refresh_mhz = 12,
		.preferred = true,
	};
	struct Mode mode2 = {
		.width = 13,
		.height = 14,
		.refresh_mhz = 15,
		.preferred = false,
	};
	struct Head head = {
		.name = "name",
		.description = "desc",
		.width_mm = 1,
		.height_mm = 2,
		.transform = WL_OUTPUT_TRANSFORM_270, // 3
		.make = "make",
		.model = "model",
		.serial_number = "serial",
		.current = {
			.scale = wl_fixed_from_double(4.0),
			.enabled = true,
			.x = 5,
			.y = 6,
			.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED,
			.mode = &mode1,
		},
		.desired = {
			.scale = wl_fixed_from_double(7.0),
			.enabled = true,
			.x = 8,
			.y = 9,
			.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED,
			.mode = &mode2,
		},
	};

	slist_append(&head.modes, &mode1);
	slist_append(&head.modes, &mode2);

	slist_append(&heads, &head);

	char *actual = marshal_ipc_response(ipc_operation);

	assert_non_null(actual);

	char *expected = read_file("tst/marshalling/ipc-response-ok.yaml");

	assert_string_equal(actual, expected);

	ipc_operation_free(ipc_operation);
	free(actual);
	free(expected);
	slist_free(&head.modes);
	slist_free(&heads);
}

void unmarshal_ipc_request__empty(void **state) {
	char *yaml = "";

	struct IpcRequest *actual = unmarshal_ipc_request(yaml);

	char *expected_log = read_file("tst/marshalling/ipc-request-empty.log");
	assert_log(ERROR, expected_log);

	assert_null(actual);

	free(expected_log);
}

void unmarshal_ipc_request__bad_op(void **state) {
	char *yaml = "OP: aoeu";

	struct IpcRequest *actual = unmarshal_ipc_request(yaml);

	char *expected_log = read_file("tst/marshalling/ipc-request-bad-op.log");
	assert_log(ERROR, expected_log);

	assert_null(actual);

	free(expected_log);
}

void unmarshal_ipc_request__no_op(void **state) {
	char *yaml = "FOO: BAR";

	struct IpcRequest *actual = unmarshal_ipc_request(yaml);

	char *expected_log = read_file("tst/marshalling/ipc-request-no-op.log");
	assert_log(ERROR, expected_log);

	assert_null(actual);

	free(expected_log);
}

void unmarshal_ipc_request__get(void **state) {
	char *yaml = read_file("tst/marshalling/ipc-request-get.yaml");

	struct IpcRequest *actual = unmarshal_ipc_request(yaml);

	assert_non_null(actual);
	assert_int_equal(actual->op, GET);
	assert_null(actual->cfg);

	ipc_request_free(actual);
	free(yaml);
}

void unmarshal_ipc_request__cfg_set(void **state) {
	char *yaml = read_file("tst/marshalling/ipc-request-cfg-set.yaml");

	struct IpcRequest *actual = unmarshal_ipc_request(yaml);

	assert_non_null(actual);
	assert_int_equal(actual->op, CFG_SET);

	struct Cfg *expected_cfg = cfg_all();

	assert_cfg_equal(actual->cfg, expected_cfg);

	ipc_request_free(actual);
	cfg_free(expected_cfg);
	free(yaml);
}

void unmarshal_ipc_response__empty(void **state) {
	char *yaml = "";

	struct IpcResponse *actual = unmarshal_ipc_response(yaml);

	char *expected_log = read_file("tst/marshalling/ipc-response-empty.log");
	assert_log(ERROR, expected_log);

	assert_null(actual);

	free(expected_log);
}

void unmarshal_ipc_response__no_done(void **state) {
	char *yaml = "- RC: 0";

	struct IpcResponse *actual = unmarshal_ipc_response(yaml);

	char *expected_log = read_file("tst/marshalling/ipc-response-no-done.log");
	assert_log(ERROR, expected_log);

	assert_null(actual);

	free(expected_log);
}

void unmarshal_ipc_response__no_rc(void **state) {
	char *yaml = "- DONE: TRUE";

	struct IpcResponse *actual = unmarshal_ipc_response(yaml);

	char *expected_log = read_file("tst/marshalling/ipc-response-no-rc.log");
	assert_log(ERROR, expected_log);

	assert_null(actual);

	free(expected_log);
}

void unmarshal_ipc_response__ok(void **state) {
	char *yaml = read_file("tst/marshalling/ipc-response-ok.yaml");

	struct IpcResponse *actual = unmarshal_ipc_response(yaml);

	assert_non_null(actual);
	assert_true(actual->done);
	assert_int_equal(actual->rc, 2);

	assert_non_null(actual->lid);
	assert_true(actual->lid->closed);
	assert_string_equal(actual->lid->device_path, "/path/to/lid");

	assert_non_null(actual->cfg);
	struct Cfg *expected_cfg = cfg_all();
	assert_cfg_equal(actual->cfg, expected_cfg);

	assert_int_equal(slist_length(actual->heads), 1);
	struct Head *head = slist_at(actual->heads, 0);

	assert_string_equal_nn(head->name, "name");
	assert_string_equal_nn(head->description, "desc");
	assert_int_equal(head->width_mm, 1);
	assert_int_equal(head->height_mm, 2);
	assert_int_equal(head->transform, WL_OUTPUT_TRANSFORM_270); // 3
	assert_string_equal_nn(head->make, "make");
	assert_string_equal_nn(head->model, "model");
	assert_string_equal_nn(head->serial_number, "serial");

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
	assert_non_null(mode_current);
	assert_int_equal(mode_current->width, 10);
	assert_int_equal(mode_current->height, 11);
	assert_int_equal(mode_current->refresh_mhz, 12);
	assert_true(mode_current->preferred);

	struct Mode *mode_desired = head->desired.mode;
	assert_non_null(mode_desired);
	assert_int_equal(mode_desired->width, 13);
	assert_int_equal(mode_desired->height, 14);
	assert_int_equal(mode_desired->refresh_mhz, 15);
	assert_false(mode_desired->preferred);

	assert_int_equal(slist_length(head->modes), 2);
	struct Mode *mode1 = slist_at(head->modes, 0);
	assert_non_null(mode1);
	assert_int_equal(mode1->width, 10);
	assert_int_equal(mode1->height, 11);
	assert_int_equal(mode1->refresh_mhz, 12);
	assert_true(mode1->preferred);

	struct Mode *mode2 = slist_at(head->modes, 1);
	assert_non_null(mode2);
	assert_int_equal(mode2->width, 13);
	assert_int_equal(mode2->height, 14);
	assert_int_equal(mode2->refresh_mhz, 15);
	assert_false(mode2->preferred);

	assert_log(DEBUG, "dbg\n");
	assert_log(INFO, "inf\n");
	assert_log(WARNING, "war\n");
	assert_log(ERROR, "err\n");

	ipc_response_free(actual);
	cfg_free(expected_cfg);
	free(yaml);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(unmarshal_cfg_from_file__ok),
		TEST(unmarshal_cfg_from_file__empty),
		TEST(unmarshal_cfg_from_file__bad),

		// YAML::Node equality operator is deprecated and not functional.
		// All we can do is read files with the same format that will be emitted.
		TEST(marshal_cfg__ok),

		TEST(marshal_ipc_request__no_op),
		TEST(marshal_ipc_request__get),
		TEST(marshal_ipc_request__cfg_set),

		TEST(marshal_ipc_response__ok),

		TEST(unmarshal_ipc_request__empty),
		TEST(unmarshal_ipc_request__bad_op),
		TEST(unmarshal_ipc_request__no_op),
		TEST(unmarshal_ipc_request__get),
		TEST(unmarshal_ipc_request__cfg_set),

		TEST(unmarshal_ipc_response__empty),
		TEST(unmarshal_ipc_response__no_done),
		TEST(unmarshal_ipc_response__no_rc),
		TEST(unmarshal_ipc_response__ok),
	};

	return RUN(tests);
}


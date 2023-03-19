#include "tst.h"
#include "asserts.h"
#include "expects.h"

#include <cmocka.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "list.h"
#include "log.h"
#include "mode.h"
#include "server.h"

#include "marshalling.h"

void lcl(enum LogThreshold threshold, char *line) {
	struct LogCapLine *lcl = calloc(1, sizeof(struct LogCapLine));

	lcl->threshold = threshold;
	lcl->line = strdup(line);

	slist_append(&log_cap_lines, lcl);
}

char *read_file(const char *path) {
	int fd = open(path, O_RDONLY);
	int len = lseek(fd, 0, SEEK_END);

	char *out = calloc(len, sizeof(char));

	memcpy(out, mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0), sizeof(char) * len);

	close(fd);

	return out;
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
	cfg_free(cfg);
	cfg = NULL;
	free(lid);
	lid = NULL;
	slist_free(&heads);
	return 0;
}


// cfg-all.yaml
struct Cfg *cfg_all(void) {
	struct Cfg *cfg = cfg_default();

	cfg->arrange = COL;
	cfg->align = BOTTOM;
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

	slist_append(&cfg->disabled_name_desc, strdup("eight"));
	slist_append(&cfg->disabled_name_desc, strdup("EIGHT"));
	slist_append(&cfg->disabled_name_desc, strdup("nine"));

	slist_append(&cfg->max_preferred_refresh_name_desc, strdup("!ten"));

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

	assert_equal_cfg(read, expected);

	cfg_free(read);
	cfg_free(expected);
}

void unmarshal_cfg_from_file__empty(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/marshalling/cfg-empty.yaml");

	expect_log_error("\nparsing file %s %s", "tst/marshalling/cfg-empty.yaml", "empty CFG", NULL, NULL);

	assert_false(unmarshal_cfg_from_file(read));

	cfg_free(read);
}

void unmarshal_cfg_from_file__bad(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/marshalling/cfg-bad.yaml");

	expect_log_warn("Ignoring invalid LOG_THRESHOLD %s, using default %s", "BAD_LOG_THRESHOLD", "INFO", NULL, NULL);

	expect_log_warn("Ignoring bad %s regex '%s':  %s", "ORDER", "(order", NULL, NULL);

	expect_log_warn("Ignoring invalid ARRANGE %s, using default %s", "BAD_ARRANGE", "ROW", NULL, NULL);

	expect_log_warn("Ignoring invalid ALIGN %s, using default %s", "BAD_ALIGN", "TOP", NULL, NULL);

	expect_log_warn("Ignoring invalid %s %s %s %s", "", "", "AUTO_SCALE", "BAD_AUTO_SCALE");

	expect_log_warn("Ignoring missing %s %s %s", "SCALE", "", "NAME_DESC", NULL);

	expect_log_warn("Ignoring invalid %s %s %s %s", "SCALE", "BAD_SCALE_NAME", "SCALE", "BAD_SCALE_VAL");

	expect_log_warn("Ignoring missing %s %s %s", "SCALE", "MISSING_SCALE_VALUE", "SCALE", NULL);

	expect_log_warn("Ignoring bad %s regex '%s':  %s", "SCALE", "(scale", NULL, NULL);

	expect_log_warn("Ignoring missing %s %s %s", "MODE", "", "NAME_DESC", NULL);

	expect_log_warn("Ignoring invalid %s %s %s %s", "MODE", "BAD_MODE_MAX", "MAX", "BAD_MAX");

	expect_log_warn("Ignoring invalid %s %s %s %s", "MODE", "BAD_MODE_WIDTH", "WIDTH", "BAD_WIDTH");

	expect_log_warn("Ignoring invalid %s %s %s %s", "MODE", "BAD_MODE_HEIGHT", "HEIGHT", "BAD_HEIGHT");

	expect_log_warn("Ignoring invalid %s %s %s %s", "MODE", "BAD_MODE_HZ", "HZ", "BAD_HZ");

	expect_log_warn("Ignoring bad %s regex '%s':  %s", "MODE", "(mode", NULL, NULL);

	expect_log_warn("Ignoring bad %s regex '%s':  %s", "MAX_PREFERRED_REFRESH", "(max", NULL, NULL);

	expect_log_warn("Ignoring bad %s regex '%s':  %s", "DISABLED", "(disabled", NULL, NULL);

	assert_true(unmarshal_cfg_from_file(read));

	struct Cfg *expected = cfg_default();

	assert_equal_cfg(read, expected);

	cfg_free(read);
	cfg_free(expected);
}

void marshal_cfg__ok(void **state) {
	struct Cfg *cfg = cfg_all();

	char *actual = marshal_cfg(cfg);

	char *expected = read_file("tst/marshalling/cfg-all.yaml");

	assert_string_equal(actual, expected);

	cfg_free(cfg);
	free(actual);
	free(expected);
}

void marshal_ipc_request__no_op(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));

	expect_log_error("marshalling ipc request: missing OP", NULL, NULL, NULL, NULL);

	assert_null(marshal_ipc_request(ipc_request));

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

	struct Cfg *cfg = cfg_all();
	ipc_request->cfg = cfg;

	char *actual = marshal_ipc_request(ipc_request);

	char *expected = read_file("tst/marshalling/ipc-request-cfg-set.yaml");

	assert_string_equal(actual, expected);

	ipc_request_free(ipc_request);
	free(actual);
	free(expected);
}

void marshal_ipc_response__ok(void **state) {
	struct IpcResponse *ipc_response = calloc(1, sizeof(struct IpcResponse));
	ipc_response->done = true;
	ipc_response->rc = 1;
	ipc_response->messages = true;
	ipc_response->status = true;

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
			.mode = &mode1,
		},
		.desired = {
			.scale = wl_fixed_from_double(7.0),
			.enabled = true,
			.x = 8,
			.y = 9,
		},
	};

	slist_append(&head.modes, &mode1);
	slist_append(&head.modes, &mode2);

	slist_append(&heads, &head);

	char *actual = marshal_ipc_response(ipc_response);

	assert_non_null(actual);

	char *expected = read_file("tst/marshalling/ipc-response-ok.yaml");

	assert_string_equal(actual, expected);

	ipc_response_free(ipc_response);
	free(actual);
	free(expected);
	slist_free(&head.modes);
}

void unmarshal_ipc_request__empty(void **state) {
	char yaml[] = "";

	expect_log_error(NULL, "empty request", NULL, NULL, NULL);
	expect_log_error_nocap(NULL, yaml, NULL, NULL, NULL);

	struct IpcRequest *actual = unmarshal_ipc_request(yaml);

	assert_null(actual);
}

void unmarshal_ipc_request__bad_op(void **state) {
	char yaml[] = "OP: aoeu";

	expect_log_error(NULL, "invalid OP 'aoeu'", NULL, NULL, NULL);
	expect_log_error_nocap(NULL, yaml, NULL, NULL, NULL);

	struct IpcRequest *actual = unmarshal_ipc_request(yaml);

	assert_null(actual);
}

void unmarshal_ipc_request__no_op(void **state) {
	char yaml[] = "FOO: BAR";

	expect_log_error(NULL, "missing OP", NULL, NULL, NULL);
	expect_log_error_nocap(NULL, yaml, NULL, NULL, NULL);

	struct IpcRequest *actual = unmarshal_ipc_request(yaml);

	assert_null(actual);
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

	assert_equal_cfg(actual->cfg, expected_cfg);

	ipc_request_free(actual);
	cfg_free(expected_cfg);
	free(yaml);
}

void unmarshal_ipc_response__empty(void **state) {
	char yaml[] = "";

	expect_log_error(NULL, "invalid response", NULL, NULL, NULL);
	expect_log_error_nocap(NULL, yaml, NULL, NULL, NULL);

	struct IpcResponse *actual = unmarshal_ipc_response(yaml);

	assert_null(actual);
}

void unmarshal_ipc_response__no_done(void **state) {
	char yaml[] = "RC: 0";

	expect_log_error(NULL, "DONE missing", NULL, NULL, NULL);
	expect_log_error_nocap(NULL, yaml, NULL, NULL, NULL);

	struct IpcResponse *actual = unmarshal_ipc_response(yaml);

	assert_null(actual);
}

void unmarshal_ipc_response__no_rc(void **state) {
	char yaml[] = "DONE: TRUE";

	expect_log_error(NULL, "RC missing", NULL, NULL, NULL);
	expect_log_error_nocap(NULL, yaml, NULL, NULL, NULL);

	struct IpcResponse *actual = unmarshal_ipc_response(yaml);

	assert_null(actual);
}

void unmarshal_ipc_response__ok(void **state) {
	char *yaml = read_file("tst/marshalling/ipc-response-ok.yaml");

	expect_log_(DEBUG, NULL, "dbg", NULL, NULL, NULL);
	expect_log_(INFO, NULL, "inf", NULL, NULL, NULL);
	expect_log_(WARNING, NULL, "war", NULL, NULL, NULL);
	expect_log_(ERROR, NULL, "err", NULL, NULL, NULL);

	struct IpcResponse *actual = unmarshal_ipc_response(yaml);

	assert_non_null(actual);
	assert_true(actual->done);
	assert_int_equal(actual->rc, 2);

	ipc_response_free(actual);
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


#include "tst.h"
#include "asserts.h"
#include "wraps.h"

#include <cmocka.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cfg.h"
#include "ipc.h"
#include "list.h"
#include "log.h"
#include "marshalling.h"

struct UserScale *us(const char *name_desc, const float scale) {
	struct UserScale *us = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	us->name_desc = strdup(name_desc);
	us->scale = scale;

	return us;
}

struct UserMode *um(const char *name_desc, const bool max, const int32_t width, const int32_t height, const int32_t refresh_hz, const bool warned_no_mode) {
	struct UserMode *um = (struct UserMode*)calloc(1, sizeof(struct UserMode));

	um->name_desc = strdup(name_desc);
	um->max = max;
	um->width = width;
	um->height = height;
	um->refresh_hz = refresh_hz;
	um->warned_no_mode = warned_no_mode;

	return um;
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
	return 0;
}

// cfg-ok.yaml
struct Cfg *cfg_ok(void) {
	struct Cfg *cfg = cfg_default();

	cfg->arrange = COL;
	cfg->align = BOTTOM;
	cfg->auto_scale = OFF;
	cfg->log_threshold = ERROR;

	slist_append(&cfg->order_name_desc, strdup("one"));
	slist_append(&cfg->order_name_desc, strdup("ONE"));
	slist_append(&cfg->order_name_desc, strdup("!two"));

	slist_append(&cfg->user_scales, us("three", 3));
	slist_append(&cfg->user_scales, us("four", 4));

	slist_append(&cfg->user_modes, um("five", false, 1920, 1080, 60, false));
	slist_append(&cfg->user_modes, um("six", false, 2560, 1440, -1, false));
	slist_append(&cfg->user_modes, um("seven", true, -1, -1, -1, false));

	slist_append(&cfg->disabled_name_desc, strdup("eight"));
	slist_append(&cfg->disabled_name_desc, strdup("EIGHT"));
	slist_append(&cfg->disabled_name_desc, strdup("nine"));

	return cfg;
}

// ipc-request-get.yaml
struct IpcRequest *ipc_request_get(void) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));
	ipc_request->command = GET;

	return ipc_request;
}

void unmarshal_cfg_from_file__ok(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/marshalling/cfg-ok.yaml");

	assert_true(unmarshal_cfg_from_file(read));

	struct Cfg *expected = cfg_ok();

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

	expect_log_warn("\nCould not compile ORDER regex '%s':  %s", "(", NULL, NULL, NULL);

	expect_log_warn("Ignoring invalid ARRANGE %s, using default %s", "BAD_ARRANGE", "ROW", NULL, NULL);

	expect_log_warn("Ignoring invalid ALIGN %s, using default %s", "BAD_ALIGN", "TOP", NULL, NULL);

	expect_log_warn("Ignoring invalid %s %s %s %s", "", "", "AUTO_SCALE", "BAD_AUTO_SCALE");

	expect_log_warn("Ignoring missing %s %s %s", "SCALE", "", "NAME_DESC", NULL);

	expect_log_warn("Ignoring invalid %s %s %s %s", "SCALE", "BAD_SCALE_NAME", "SCALE", "BAD_SCALE_VAL");

	expect_log_warn("Ignoring missing %s %s %s", "SCALE", "MISSING_SCALE_VALUE", "SCALE", NULL);

	expect_log_warn("Ignoring missing %s %s %s", "MODE", "", "NAME_DESC", NULL);

	expect_log_warn("Ignoring invalid %s %s %s %s", "MODE", "BAD_MODE_MAX", "MAX", "BAD_MAX");

	expect_log_warn("Ignoring invalid %s %s %s %s", "MODE", "BAD_MODE_WIDTH", "WIDTH", "BAD_WIDTH");

	expect_log_warn("Ignoring invalid %s %s %s %s", "MODE", "BAD_MODE_HEIGHT", "HEIGHT", "BAD_HEIGHT");

	expect_log_warn("Ignoring invalid %s %s %s %s", "MODE", "BAD_MODE_HZ", "HZ", "BAD_HZ");

	assert_true(unmarshal_cfg_from_file(read));

	struct Cfg *expected = cfg_default();

	assert_equal_cfg(read, expected);

	cfg_free(read);
	cfg_free(expected);
}

void marshal_cfg__ok(void **state) {
	struct Cfg *cfg = cfg_ok();

	char *actual = marshal_cfg(cfg);

	char *expected = read_file("tst/marshalling/cfg-ok.yaml");

	assert_string_equal(actual, expected);

	cfg_free(cfg);
	free(actual);
	free(expected);
}

void marshal_ipc_request__no_op(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));

	expect_log_error("marshalling ipc request: missing OP", NULL, NULL, NULL, NULL);

	assert_null(marshal_ipc_request(ipc_request));

	free_ipc_request(ipc_request);
}

void marshal_ipc_request__get(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));
	ipc_request->command = GET;

	char *actual = marshal_ipc_request(ipc_request);

	char *expected = read_file("tst/marshalling/ipc-request-get.yaml");

	assert_string_equal(actual, expected);

	free_ipc_request(ipc_request);
	free(actual);
	free(expected);
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
	};

	return RUN(tests);
}


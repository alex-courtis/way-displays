#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "list.h"
#include "log.h"
#include "marshalling.h"

struct UserScale *us(const char *name_desc, const float scale) {
	struct UserScale *us = calloc(1, sizeof(struct UserScale));

	us->name_desc = strdup(name_desc);
	us->scale = scale;

	return us;
}

struct UserMode *um(const char *name_desc, const bool max, const int32_t width, const int32_t height, const int32_t refresh_hz, const bool warned_no_mode) {
	struct UserMode *um = calloc(1, sizeof(struct UserMode));

	um->name_desc = strdup(name_desc);
	um->max = max;
	um->width = width;
	um->height = height;
	um->refresh_hz = refresh_hz;
	um->warned_no_mode = warned_no_mode;

	return um;
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

void __wrap_log_warn(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
	check_expected(__format);
	check_expected(arg1);
	check_expected(arg2);
	check_expected(arg3);
	check_expected(arg4);
}

void __wrap_log_error(const char *__restrict __format, const void *arg1, const void *arg2, const void *arg3, const void *arg4, ...) {
	check_expected(__format);
	check_expected(arg1);
	check_expected(arg2);
	check_expected(arg3);
	check_expected(arg4);
}

void unmarshal_cfg_from_file__ok(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/marshalling/cfg-ok.yaml");

	assert_true(unmarshal_cfg_from_file(read));

	struct Cfg *expected = cfg_default();

	expected->arrange = COL;
	expected->align = BOTTOM;
	expected->auto_scale = OFF;
	expected->log_threshold = ERROR;

	slist_append(&expected->order_name_desc, strdup("one"));
	slist_append(&expected->order_name_desc, strdup("ONE"));
	slist_append(&expected->order_name_desc, strdup("!two"));

	slist_append(&expected->user_scales, us("three", 3));
	slist_append(&expected->user_scales, us("four", 4));

	slist_append(&expected->user_modes, um("five", false, 1920, 1080, 60, false));
	slist_append(&expected->user_modes, um("six", false, 2560, 1440, -1, false));
	slist_append(&expected->user_modes, um("seven", true, -1, -1, -1, false));

	slist_append(&expected->disabled_name_desc, strdup("eight"));
	slist_append(&expected->disabled_name_desc, strdup("EIGHT"));
	slist_append(&expected->disabled_name_desc, strdup("nine"));

	assert_equal_cfg(read, expected);

	cfg_free(read);
	cfg_free(expected);
}

void unmarshal_cfg_from_file__empty(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/marshalling/cfg-empty.yaml");

	expect_string(__wrap_log_error, __format, "\nparsing file %s %s");
	expect_string(__wrap_log_error, arg1, "tst/marshalling/cfg-empty.yaml");
	expect_string(__wrap_log_error, arg2, "empty CFG");
	expect_any(__wrap_log_error, arg3);
	expect_any(__wrap_log_error, arg4);

	assert_false(unmarshal_cfg_from_file(read));

	cfg_free(read);
}

void unmarshal_cfg_from_file__bad(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/marshalling/cfg-bad.yaml");

	expect_string(__wrap_log_warn, __format, "Ignoring invalid LOG_THRESHOLD %s, using default %s");
	expect_string(__wrap_log_warn, arg1, "BAD_LOG_THRESHOLD");
	expect_string(__wrap_log_warn, arg2, "INFO");
	expect_any(__wrap_log_warn, arg3);
	expect_any(__wrap_log_warn, arg4);

	expect_string(__wrap_log_warn, __format, "\nCould not compile ORDER regex '%s':  %s");
	expect_string(__wrap_log_warn, arg1, "(");
	expect_any(__wrap_log_warn, arg2);
	expect_any(__wrap_log_warn, arg3);
	expect_any(__wrap_log_warn, arg4);

	expect_string(__wrap_log_warn, __format, "Ignoring invalid ARRANGE %s, using default %s");
	expect_string(__wrap_log_warn, arg1, "BAD_ARRANGE");
	expect_string(__wrap_log_warn, arg2, "ROW");
	expect_any(__wrap_log_warn, arg3);
	expect_any(__wrap_log_warn, arg4);

	expect_string(__wrap_log_warn, __format, "Ignoring invalid ALIGN %s, using default %s");
	expect_string(__wrap_log_warn, arg1, "BAD_ALIGN");
	expect_string(__wrap_log_warn, arg2, "TOP");
	expect_any(__wrap_log_warn, arg3);
	expect_any(__wrap_log_warn, arg4);

	expect_string(__wrap_log_warn, __format, "Ignoring invalid %s %s %s %s");
	expect_string(__wrap_log_warn, arg1, "");
	expect_string(__wrap_log_warn, arg2, "");
	expect_string(__wrap_log_warn, arg3, "AUTO_SCALE");
	expect_string(__wrap_log_warn, arg4, "BAD_AUTO_SCALE");

	expect_string(__wrap_log_warn, __format, "Ignoring missing %s %s %s");
	expect_string(__wrap_log_warn, arg1, "SCALE");
	expect_string(__wrap_log_warn, arg2, "");
	expect_string(__wrap_log_warn, arg3, "NAME_DESC");
	expect_any(__wrap_log_warn, arg4);

	expect_string(__wrap_log_warn, __format, "Ignoring invalid %s %s %s %s");
	expect_string(__wrap_log_warn, arg1, "SCALE");
	expect_string(__wrap_log_warn, arg2, "BAD_SCALE_NAME");
	expect_string(__wrap_log_warn, arg3, "SCALE");
	expect_string(__wrap_log_warn, arg4, "BAD_SCALE_VAL");

	expect_string(__wrap_log_warn, __format, "Ignoring missing %s %s %s");
	expect_string(__wrap_log_warn, arg1, "SCALE");
	expect_string(__wrap_log_warn, arg2, "MISSING_SCALE_VALUE");
	expect_string(__wrap_log_warn, arg3, "SCALE");
	expect_any(__wrap_log_warn, arg4);

	expect_string(__wrap_log_warn, __format, "Ignoring missing %s %s %s");
	expect_string(__wrap_log_warn, arg1, "MODE");
	expect_string(__wrap_log_warn, arg2, "");
	expect_string(__wrap_log_warn, arg3, "NAME_DESC");
	expect_any(__wrap_log_warn, arg4);

	expect_string(__wrap_log_warn, __format, "Ignoring invalid %s %s %s %s");
	expect_string(__wrap_log_warn, arg1, "MODE");
	expect_string(__wrap_log_warn, arg2, "BAD_MODE_MAX");
	expect_string(__wrap_log_warn, arg3, "MAX");
	expect_string(__wrap_log_warn, arg4, "BAD_MAX");

	expect_string(__wrap_log_warn, __format, "Ignoring invalid %s %s %s %s");
	expect_string(__wrap_log_warn, arg1, "MODE");
	expect_string(__wrap_log_warn, arg2, "BAD_MODE_WIDTH");
	expect_string(__wrap_log_warn, arg3, "WIDTH");
	expect_string(__wrap_log_warn, arg4, "BAD_WIDTH");

	expect_string(__wrap_log_warn, __format, "Ignoring invalid %s %s %s %s");
	expect_string(__wrap_log_warn, arg1, "MODE");
	expect_string(__wrap_log_warn, arg2, "BAD_MODE_HEIGHT");
	expect_string(__wrap_log_warn, arg3, "HEIGHT");
	expect_string(__wrap_log_warn, arg4, "BAD_HEIGHT");

	expect_string(__wrap_log_warn, __format, "Ignoring invalid %s %s %s %s");
	expect_string(__wrap_log_warn, arg1, "MODE");
	expect_string(__wrap_log_warn, arg2, "BAD_MODE_HZ");
	expect_string(__wrap_log_warn, arg3, "HZ");
	expect_string(__wrap_log_warn, arg4, "BAD_HZ");

	assert_true(unmarshal_cfg_from_file(read));

	struct Cfg *expected = cfg_default();

	assert_equal_cfg(read, expected);

	cfg_free(read);
	cfg_free(expected);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(unmarshal_cfg_from_file__ok),
		TEST(unmarshal_cfg_from_file__empty),
		TEST(unmarshal_cfg_from_file__bad),
	};

	return RUN(tests);
}


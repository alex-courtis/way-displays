#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "list.h"

// forward declarations
struct Cfg *merge_set(struct Cfg *to, struct Cfg *from);
struct Cfg *merge_del(struct Cfg *to, struct Cfg *from);
void validate_warn(struct Cfg *cfg);
void validate_fix(struct Cfg *cfg);

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

struct State {
	struct Cfg *from;
	struct Cfg *to;
	struct Cfg *expected;
};

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	s->from = cfg_default();
	s->to = cfg_default();
	s->expected = cfg_default();

	*state = s;
	return 0;
}

int after_each(void **state) {
	struct State *s = *state;

	cfg_free(s->from);
	cfg_free(s->to);
	cfg_free(s->expected);

	free(s);
	return 0;
}

void __wrap_log_warn(const char *__restrict __format, const void *arg1, const void *arg2, ...) {
	check_expected(__format);
	check_expected(arg1);
	check_expected(arg2);
}

void merge_set__arrange(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->expected->arrange = COL;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_equal_cfg(merged, s->expected);

	cfg_free(merged);
}

void merge_set__align(void **state) {
	struct State *s = *state;

	s->from->align = MIDDLE;
	s->expected->align = MIDDLE;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_equal_cfg(merged, s->expected);

	cfg_free(merged);
}

void merge_set__order(void **state) {
	struct State *s = *state;

	slist_append(&s->to->order_name_desc, strdup("A"));
	slist_append(&s->from->order_name_desc, strdup("X"));

	slist_append(&s->expected->order_name_desc, strdup("X"));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_equal_cfg(merged, s->expected);

	cfg_free(merged);
}

void merge_set__auto_scale(void **state) {
	struct State *s = *state;

	s->from->auto_scale = OFF;
	s->expected->auto_scale = OFF;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_equal_cfg(merged, s->expected);

	cfg_free(merged);
}

void merge_set__user_scale(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_scales, us("to", 1));
	slist_append(&s->to->user_scales, us("both", 2));

	slist_append(&s->from->user_scales, us("from", 3));
	slist_append(&s->from->user_scales, us("both", 4));

	slist_append(&s->expected->user_scales, us("to", 1));
	slist_append(&s->expected->user_scales, us("both", 4));
	slist_append(&s->expected->user_scales, us("from", 3));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_equal_cfg(merged, s->expected);

	cfg_free(merged);
}

void merge_set__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_modes, um("to", false, 1, 2, 3, false));
	slist_append(&s->to->user_modes, um("both", false, 4, 5, 6, false));

	slist_append(&s->from->user_modes, um("from", false, 7, 8, 9, true));
	slist_append(&s->from->user_modes, um("both", false, 10, 11, 12, true));

	slist_append(&s->expected->user_modes, um("to", false, 1, 2, 3, false));
	slist_append(&s->expected->user_modes, um("both", false, 10, 11, 12, true));
	slist_append(&s->expected->user_modes, um("from", false, 7, 8, 9, true));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_equal_cfg(merged, s->expected);

	cfg_free(merged);
}

void merge_set__disabled(void **state) {
	struct State *s = *state;

	slist_append(&s->to->disabled_name_desc, strdup("to"));
	slist_append(&s->to->disabled_name_desc, strdup("both"));

	slist_append(&s->from->disabled_name_desc, strdup("from"));
	slist_append(&s->from->disabled_name_desc, strdup("both"));

	slist_append(&s->expected->disabled_name_desc, strdup("to"));
	slist_append(&s->expected->disabled_name_desc, strdup("both"));
	slist_append(&s->expected->disabled_name_desc, strdup("from"));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_equal_cfg(merged, s->expected);

	cfg_free(merged);
}

void merge_del__scale(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_scales, us("1", 1));
	slist_append(&s->to->user_scales, us("2", 2));

	slist_append(&s->from->user_scales, us("2", 3));
	slist_append(&s->from->user_scales, us("3", 4));

	slist_append(&s->expected->user_scales, us("1", 1));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_equal_cfg(merged, s->expected);

	cfg_free(merged);
}

void merge_del__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_modes, um("1", false, 1, 1, 1, false));
	slist_append(&s->to->user_modes, um("2", false, 2, 2, 2, false));

	slist_append(&s->from->user_modes, um("2", false, 2, 2, 2, false));
	slist_append(&s->from->user_modes, um("3", false, 3, 3, 3, false));

	slist_append(&s->from->user_modes, um("1", false, 1, 1, 1, false));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_equal_cfg(merged, s->expected);

	cfg_free(merged);
}

void merge_del__disabled(void **state) {
	struct State *s = *state;

	slist_append(&s->to->disabled_name_desc, strdup("1"));
	slist_append(&s->to->disabled_name_desc, strdup("2"));

	slist_append(&s->from->disabled_name_desc, strdup("2"));
	slist_append(&s->from->disabled_name_desc, strdup("3"));

	slist_append(&s->expected->disabled_name_desc, strdup("1"));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_equal_cfg(merged, s->expected);

	cfg_free(merged);
}

void validate_fix__col(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->from->align = TOP;
	expect_string(__wrap_log_warn, __format, "\nIgnoring invalid ALIGN %s for %s arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.");
	expect_string(__wrap_log_warn, arg1, "TOP");
	expect_string(__wrap_log_warn, arg2, "COLUMN");

	s->expected->arrange = COL;
	s->expected->align = LEFT;

	validate_fix(s->from);

	assert_equal_cfg(s->from, s->expected);
}

void validate_fix__row(void **state) {
	struct State *s = *state;

	s->from->arrange = ROW;
	s->from->align = RIGHT;
	expect_string(__wrap_log_warn, __format, "\nIgnoring invalid ALIGN %s for %s arrange. Valid values are TOP, MIDDLE and BOTTOM. Using default TOP.");
	expect_string(__wrap_log_warn, arg1, "RIGHT");
	expect_string(__wrap_log_warn, arg2, "ROW");

	s->expected->arrange = ROW;
	s->expected->align = TOP;

	validate_fix(s->from);

	assert_equal_cfg(s->from, s->expected);
}

void validate_fix__scale(void **state) {
	struct State *s = *state;

	char FMT[] = "\nIgnoring non-positive SCALE %s %.3f";

	slist_append(&s->from->user_scales, us("ok", 1));

	slist_append(&s->from->user_scales, us("neg", -1));
	expect_string(__wrap_log_warn, __format, FMT);
	expect_string(__wrap_log_warn, arg1, "neg");
	expect_any(__wrap_log_warn, arg2);

	slist_append(&s->from->user_scales, us("zero", 0));
	expect_string(__wrap_log_warn, __format, FMT);
	expect_string(__wrap_log_warn, arg1, "zero");
	expect_any(__wrap_log_warn, arg2);

	validate_fix(s->from);

	slist_append(&s->expected->user_scales, us("ok", 1));

	assert_equal_cfg(s->from, s->expected);
}

void validate_fix__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->from->user_modes, um("ok", false, 1, 2, 3, false));
	slist_append(&s->from->user_modes, um("max", true, -1, -1, -1, false));

	slist_append(&s->from->user_modes, um("negative width", false, -99, 2, 3, false));
	expect_string(__wrap_log_warn, __format, "\nIgnoring non-positive MODE %s WIDTH %d");
	expect_string(__wrap_log_warn, arg1, "negative width");
	expect_any(__wrap_log_warn, arg2);

	slist_append(&s->from->user_modes, um("negative height", false, 1, -99, 3, false));
	expect_string(__wrap_log_warn, __format, "\nIgnoring non-positive MODE %s HEIGHT %d");
	expect_string(__wrap_log_warn, arg1, "negative height");
	expect_any(__wrap_log_warn, arg2);

	slist_append(&s->from->user_modes, um("negative hz", false, 1, 2, -99, false));
	expect_string(__wrap_log_warn, __format, "\nIgnoring non-positive MODE %s HZ %d");
	expect_string(__wrap_log_warn, arg1, "negative hz");
	expect_any(__wrap_log_warn, arg2);

	slist_append(&s->from->user_modes, um("missing width", false, -1, 2, 3, false));
	expect_string(__wrap_log_warn, __format, "\nIgnoring invalid MODE %s missing WIDTH");
	expect_string(__wrap_log_warn, arg1, "missing width");
	expect_any(__wrap_log_warn, arg2);

	slist_append(&s->from->user_modes, um("missing height", false, 1, -1, 3, false));
	expect_string(__wrap_log_warn, __format, "\nIgnoring invalid MODE %s missing HEIGHT");
	expect_string(__wrap_log_warn, arg1, "missing height");
	expect_any(__wrap_log_warn, arg2);

	validate_fix(s->from);

	slist_append(&s->expected->user_modes, um("ok", false, 1, 2, 3, false));
	slist_append(&s->expected->user_modes, um("max", true, -1, -1, -1, false));

	assert_equal_cfg(s->from, s->expected);
}

void validate_warn__(void **state) {
	struct State *s = *state;

	char FMT[] = "\n%s '%s' is less than 4 characters, which may result in some unwanted matches.";

	slist_append(&s->expected->user_scales, us("sss", 1));
	slist_append(&s->expected->user_scales, us("ssssssss", 2));
	expect_string(__wrap_log_warn, __format, FMT);
	expect_string(__wrap_log_warn, arg1, "SCALE");
	expect_string(__wrap_log_warn, arg2, "sss");

	slist_append(&s->expected->user_modes, um("mmm", false, 1, 1, 1, false));
	slist_append(&s->expected->user_modes, um("mmmmmmmm", false, 1, 1, 1, false));
	expect_string(__wrap_log_warn, __format, FMT);
	expect_string(__wrap_log_warn, arg1, "MODE");
	expect_string(__wrap_log_warn, arg2, "mmm");

	slist_append(&s->expected->order_name_desc, strdup("ooo"));
	slist_append(&s->expected->order_name_desc, strdup("oooooooooo"));
	expect_string(__wrap_log_warn, __format, FMT);
	expect_string(__wrap_log_warn, arg1, "ORDER");
	expect_string(__wrap_log_warn, arg2, "ooo");

	slist_append(&s->expected->max_preferred_refresh_name_desc, strdup("ppp"));
	slist_append(&s->expected->max_preferred_refresh_name_desc, strdup("pppppppppp"));
	expect_string(__wrap_log_warn, __format, FMT);
	expect_string(__wrap_log_warn, arg1, "MAX_PREFERRED_REFRESH");
	expect_string(__wrap_log_warn, arg2, "ppp");

	slist_append(&s->expected->disabled_name_desc, strdup("ddd"));
	slist_append(&s->expected->disabled_name_desc, strdup("dddddddddd"));
	expect_string(__wrap_log_warn, __format, FMT);
	expect_string(__wrap_log_warn, arg1, "DISABLED");
	expect_string(__wrap_log_warn, arg2, "ddd");

	validate_warn(s->expected);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(merge_set__arrange),
		TEST(merge_set__align),
		TEST(merge_set__order),
		TEST(merge_set__auto_scale),
		TEST(merge_set__user_scale),
		TEST(merge_set__mode),
		TEST(merge_set__disabled),

		TEST(merge_del__scale),
		TEST(merge_del__mode),
		TEST(merge_del__disabled),

		TEST(validate_fix__col),
		TEST(validate_fix__row),
		TEST(validate_fix__scale),
		TEST(validate_fix__mode),

		TEST(validate_warn__),
	};

	return RUN(tests);
}


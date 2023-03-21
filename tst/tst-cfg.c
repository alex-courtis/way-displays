#include "tst.h"
#include "asserts.h"
#include "expects.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

#include "cfg.h"

struct Cfg *merge_set(struct Cfg *to, struct Cfg *from);
struct Cfg *merge_del(struct Cfg *to, struct Cfg *from);
void validate_warn(struct Cfg *cfg);
void validate_fix(struct Cfg *cfg);


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


void merge_set__arrange(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->expected->arrange = COL;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__align(void **state) {
	struct State *s = *state;

	s->from->align = MIDDLE;
	s->expected->align = MIDDLE;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__order(void **state) {
	struct State *s = *state;

	slist_append(&s->to->order_name_desc, strdup("A"));
	slist_append(&s->from->order_name_desc, strdup("X"));

	slist_append(&s->expected->order_name_desc, strdup("X"));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__auto_scale(void **state) {
	struct State *s = *state;

	s->from->auto_scale = OFF;
	s->expected->auto_scale = OFF;

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__user_scale(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_scales, cfg_user_scale_init("to", 1));
	slist_append(&s->to->user_scales, cfg_user_scale_init("both", 2));

	slist_append(&s->from->user_scales, cfg_user_scale_init("from", 3));
	slist_append(&s->from->user_scales, cfg_user_scale_init("both", 4));

	slist_append(&s->expected->user_scales, cfg_user_scale_init("to", 1));
	slist_append(&s->expected->user_scales, cfg_user_scale_init("both", 4));
	slist_append(&s->expected->user_scales, cfg_user_scale_init("from", 3));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_modes, cfg_user_mode_init("to", false, 1, 2, 3, false));
	slist_append(&s->to->user_modes, cfg_user_mode_init("both", false, 4, 5, 6, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("from", false, 7, 8, 9, true));
	slist_append(&s->from->user_modes, cfg_user_mode_init("both", false, 10, 11, 12, true));

	slist_append(&s->expected->user_modes, cfg_user_mode_init("to", false, 1, 2, 3, false));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("both", false, 10, 11, 12, true));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("from", false, 7, 8, 9, true));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_set__vrr_off(void **state) {
	struct State *s = *state;

	slist_append(&s->to->vrr_off_name_desc, strdup("to"));
	slist_append(&s->to->vrr_off_name_desc, strdup("both"));

	slist_append(&s->from->vrr_off_name_desc, strdup("from"));
	slist_append(&s->from->vrr_off_name_desc, strdup("both"));

	slist_append(&s->expected->vrr_off_name_desc, strdup("to"));
	slist_append(&s->expected->vrr_off_name_desc, strdup("both"));
	slist_append(&s->expected->vrr_off_name_desc, strdup("from"));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

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

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__scale(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_scales, cfg_user_scale_init("1", 1));
	slist_append(&s->to->user_scales, cfg_user_scale_init("2", 2));

	slist_append(&s->from->user_scales, cfg_user_scale_init("2", 3));
	slist_append(&s->from->user_scales, cfg_user_scale_init("3", 4));

	slist_append(&s->expected->user_scales, cfg_user_scale_init("1", 1));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->to->user_modes, cfg_user_mode_init("1", false, 1, 1, 1, false));
	slist_append(&s->to->user_modes, cfg_user_mode_init("2", false, 2, 2, 2, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("2", false, 2, 2, 2, false));
	slist_append(&s->from->user_modes, cfg_user_mode_init("3", false, 3, 3, 3, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("1", false, 1, 1, 1, false));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void merge_del__vrr_off(void **state) {
	struct State *s = *state;

	slist_append(&s->to->vrr_off_name_desc, strdup("1"));
	slist_append(&s->to->vrr_off_name_desc, strdup("2"));

	slist_append(&s->from->vrr_off_name_desc, strdup("2"));
	slist_append(&s->from->vrr_off_name_desc, strdup("3"));

	slist_append(&s->expected->vrr_off_name_desc, strdup("1"));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

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

	assert_cfg_equal(merged, s->expected);

	cfg_free(merged);
}

void validate_fix__col(void **state) {
	struct State *s = *state;

	s->from->arrange = COL;
	s->from->align = TOP;
	expect_log_warn("\nIgnoring invalid ALIGN %s for %s arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.", "TOP", "COLUMN", NULL, NULL);

	s->expected->arrange = COL;
	s->expected->align = LEFT;

	validate_fix(s->from);

	assert_cfg_equal(s->from, s->expected);
}

void validate_fix__row(void **state) {
	struct State *s = *state;

	s->from->arrange = ROW;
	s->from->align = RIGHT;
	expect_log_warn("\nIgnoring invalid ALIGN %s for %s arrange. Valid values are TOP, MIDDLE and BOTTOM. Using default TOP.", "RIGHT", "ROW", NULL, NULL);

	s->expected->arrange = ROW;
	s->expected->align = TOP;

	validate_fix(s->from);

	assert_cfg_equal(s->from, s->expected);
}

void validate_fix__scale(void **state) {
	struct State *s = *state;

	char *fmt = "\nIgnoring non-positive SCALE %s %.3f";

	slist_append(&s->from->user_scales, cfg_user_scale_init("ok", 1));

	slist_append(&s->from->user_scales, cfg_user_scale_init("neg", -1));
	expect_log_warn(fmt, "neg", NULL, NULL, NULL);

	slist_append(&s->from->user_scales, cfg_user_scale_init("zero", 0));
	expect_log_warn(fmt, "zero", NULL, NULL, NULL);

	validate_fix(s->from);

	slist_append(&s->expected->user_scales, cfg_user_scale_init("ok", 1));

	assert_cfg_equal(s->from, s->expected);
}

void validate_fix__mode(void **state) {
	struct State *s = *state;

	slist_append(&s->from->user_modes, cfg_user_mode_init("ok", false, 1, 2, 3, false));
	slist_append(&s->from->user_modes, cfg_user_mode_init("max", true, -1, -1, -1, false));

	slist_append(&s->from->user_modes, cfg_user_mode_init("negative width", false, -99, 2, 3, false));
	expect_log_warn("\nIgnoring non-positive MODE %s WIDTH %d", "negative width", NULL, NULL, NULL);

	slist_append(&s->from->user_modes, cfg_user_mode_init("negative height", false, 1, -99, 3, false));
	expect_log_warn("\nIgnoring non-positive MODE %s HEIGHT %d", "negative height", NULL, NULL, NULL);

	slist_append(&s->from->user_modes, cfg_user_mode_init("negative hz", false, 1, 2, -99, false));
	expect_log_warn("\nIgnoring non-positive MODE %s HZ %d", "negative hz", NULL, NULL, NULL);

	slist_append(&s->from->user_modes, cfg_user_mode_init("missing width", false, -1, 2, 3, false));
	expect_log_warn("\nIgnoring invalid MODE %s missing WIDTH", "missing width", NULL, NULL, NULL);

	slist_append(&s->from->user_modes, cfg_user_mode_init("missing height", false, 1, -1, 3, false));
	expect_log_warn("\nIgnoring invalid MODE %s missing HEIGHT", "missing height", NULL, NULL, NULL);

	validate_fix(s->from);

	slist_append(&s->expected->user_modes, cfg_user_mode_init("ok", false, 1, 2, 3, false));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("max", true, -1, -1, -1, false));

	assert_cfg_equal(s->from, s->expected);
}

void validate_warn__(void **state) {
	struct State *s = *state;

	char *fmt = "\n%s '%s' is less than 4 characters, which may result in some unwanted matches.";

	slist_append(&s->expected->user_scales, cfg_user_scale_init("sss", 1));
	slist_append(&s->expected->user_scales, cfg_user_scale_init("ssssssss", 2));
	expect_log_warn(fmt, "SCALE", "sss", NULL, NULL);

	slist_append(&s->expected->user_modes, cfg_user_mode_init("mmm", false, 1, 1, 1, false));
	slist_append(&s->expected->user_modes, cfg_user_mode_init("mmmmmmmm", false, 1, 1, 1, false));
	expect_log_warn(fmt, "MODE", "mmm", NULL, NULL);

	slist_append(&s->expected->order_name_desc, strdup("ooo"));
	slist_append(&s->expected->order_name_desc, strdup("oooooooooo"));
	expect_log_warn(fmt, "ORDER", "ooo", NULL, NULL);

	slist_append(&s->expected->vrr_off_name_desc, strdup("vvv"));
	slist_append(&s->expected->vrr_off_name_desc, strdup("vvvvvvvvvv"));
	expect_log_warn(fmt, "VRR_OFF", "vvv", NULL, NULL);

	slist_append(&s->expected->max_preferred_refresh_name_desc, strdup("ppp"));
	slist_append(&s->expected->max_preferred_refresh_name_desc, strdup("pppppppppp"));
	expect_log_warn(fmt, "MAX_PREFERRED_REFRESH", "ppp", NULL, NULL);

	slist_append(&s->expected->disabled_name_desc, strdup("ddd"));
	slist_append(&s->expected->disabled_name_desc, strdup("dddddddddd"));
	expect_log_warn(fmt, "DISABLED", "ddd", NULL, NULL);

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
		TEST(merge_set__vrr_off),
		TEST(merge_set__disabled),

		TEST(merge_del__scale),
		TEST(merge_del__mode),
		TEST(merge_del__vrr_off),
		TEST(merge_del__disabled),

		TEST(validate_fix__col),
		TEST(validate_fix__row),
		TEST(validate_fix__scale),
		TEST(validate_fix__mode),

		TEST(validate_warn__),
	};

	return RUN(tests);
}


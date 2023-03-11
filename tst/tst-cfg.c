#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "list.h"

// forward declarations
struct Cfg *merge_set(struct Cfg *to, struct Cfg *from);
struct Cfg *merge_del(struct Cfg *to, struct Cfg *from);

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

	slist_append(&s->to->user_scales, us("to", 1));
	slist_append(&s->to->user_scales, us("both", 2));

	slist_append(&s->from->user_scales, us("from", 3));
	slist_append(&s->from->user_scales, us("both", 4));

	slist_append(&s->expected->user_scales, us("to", 1));
	slist_append(&s->expected->user_scales, us("both", 4));
	slist_append(&s->expected->user_scales, us("from", 3));

	struct Cfg *merged = merge_set(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

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

	slist_append(&s->to->user_scales, us("1", 1));
	slist_append(&s->to->user_scales, us("2", 2));

	slist_append(&s->from->user_scales, us("2", 3));
	slist_append(&s->from->user_scales, us("3", 4));

	slist_append(&s->expected->user_scales, us("1", 1));

	struct Cfg *merged = merge_del(s->to, s->from);

	assert_cfg_equal(merged, s->expected);

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
	};

	return RUN(tests);
}


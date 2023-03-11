#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
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

void unmarshal_cfg_from_file__ok(void **state) {

	struct Cfg *read = cfg_default();
	read->file_path = strdup("tst/cfg.yaml");

	assert_true(unmarshal_cfg_from_file(read));

	struct Cfg *expected = cfg_default();

	expected->arrange = COL;
	expected->align = BOTTOM;
	expected->auto_scale = OFF;
	expected->log_threshold = ERROR;

	slist_append(&expected->order_name_desc, strdup("one"));
	slist_append(&expected->order_name_desc, strdup("two"));

	slist_append(&expected->user_scales, us("three", 3));
	slist_append(&expected->user_scales, us("four", 4));

	slist_append(&expected->user_modes, um("five", false, 1920, 1080, 60, false));
	slist_append(&expected->user_modes, um("six", false, 2560, 1440, -1, false));
	slist_append(&expected->user_modes, um("seven", true, -1, -1, -1, false));

	slist_append(&expected->disabled_name_desc, strdup("eight"));
	slist_append(&expected->disabled_name_desc, strdup("nine"));

	assert_equal_cfg(read, expected);

	cfg_free(read);
	cfg_free(expected);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(unmarshal_cfg_from_file__ok),
	};

	return RUN(tests);
}


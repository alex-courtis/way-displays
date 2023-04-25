#include "tst.h"
#include "asserts.h"
#include "expects.h"

#include <cmocka.h>
// #include <fcntl.h>
// #include <stdbool.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/mman.h>
// #include <unistd.h>

#include "list.h"

#include "cfg.h"
#include "global.h"
#include "info.h"

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

void print_cfg_commands__empty(void **state) {
	struct Cfg *cfg = calloc(1, sizeof(struct Cfg));

	print_cfg_commands(INFO, cfg);

	free(cfg);
}

void print_cfg_commands__ok(void **state) {
	struct Cfg *cfg = cfg_default();

	cfg->arrange = COL;
	cfg->align = RIGHT;
	expect_log_(INFO, "way-displays -s ARRANGE_ALIGN %s %s", "COLUMN", "RIGHT", NULL, NULL);

	slist_append(&cfg->order_name_desc, strdup("one"));
	slist_append(&cfg->order_name_desc, strdup("two"));
	slist_append(&cfg->order_name_desc, strdup("three"));
	expect_log_(INFO, "way-displays -s ORDER %s", "'one' 'two' 'three' ", NULL, NULL, NULL);

	cfg->auto_scale = OFF;
	expect_log_(INFO, "way-displays -s AUTO_SCALE %s", "OFF", NULL, NULL, NULL);

	slist_append(&cfg->user_scales, cfg_user_scale_init("one", 1));
	slist_append(&cfg->user_scales, cfg_user_scale_init("two", 2.3456));
	expect_log_(INFO, "way-displays -s SCALE '%s' %s", "one", "1.000", NULL, NULL);
	expect_log_(INFO, "way-displays -s SCALE '%s' %s", "two", "2.346", NULL, NULL);

	slist_append(&cfg->user_modes, cfg_user_mode_init("all", false, 1, 2, 3, false));
	slist_append(&cfg->user_modes, cfg_user_mode_init("res", false, 4, 5, -1, false));
	slist_append(&cfg->user_modes, cfg_user_mode_init("max", true, 7, 8, 9, false));
	expect_log_(INFO, "way-displays -s MODE '%s' %s", "all", "1 2 3", NULL, NULL);
	expect_log_(INFO, "way-displays -s MODE '%s' %s", "res", "4 5", NULL, NULL);
	expect_log_(INFO, "way-displays -s MODE '%s' %s", "max", "MAX", NULL, NULL);

	slist_append(&cfg->disabled_name_desc, strdup("one"));
	slist_append(&cfg->disabled_name_desc, strdup("two"));
	expect_log_(INFO, "way-displays -s DISABLED '%s'", "one", NULL, NULL, NULL);
	expect_log_(INFO, "way-displays -s DISABLED '%s'", "two", NULL, NULL, NULL);

	print_cfg_commands(INFO, cfg);

	cfg_free(cfg);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(print_cfg_commands__empty),
		TEST(print_cfg_commands__ok),
	};

	return RUN(tests);
}


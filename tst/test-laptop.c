#include <sys/stat.h>
#include <unistd.h>

#include "laptop.h"

static int laptop_lid_closed_setup_teardown(void **state) {

	// always try and remove anything from create_state_file
	remove("/tmp/wld-lid-state-testing/LIDX/state");
	rmdir("/tmp/wld-lid-state-testing/LIDX");
	rmdir("/tmp/wld-lid-state-testing");

	*state = NULL;

	return 0;
}

void create_state_file(const char *contents) {
	assert_int_equal(0, mkdir("/tmp/wld-lid-state-testing", 0755));
	assert_int_equal(0, mkdir("/tmp/wld-lid-state-testing/LIDX", 0755));

	FILE *lidStateFile = fopen("/tmp/wld-lid-state-testing/LIDX/state", "w");
	assert_non_null(lidStateFile);
	assert_int_not_equal(0, fputs(contents, lidStateFile));
	assert_int_equal(0, fclose(lidStateFile));
}

static void laptop_lid_closed_missing_path(void **state) {
	assert_false(laptop_lid_closed_path("/tmp/nonexistent"));
}

static void laptop_lid_closed_missing_file(void **state) {
	assert_int_equal(0, mkdir("/tmp/wld-lid-state-testing", 0755));
	assert_int_equal(0, mkdir("/tmp/wld-lid-state-testing/LIDX", 0755));

	assert_false(laptop_lid_closed_path("/tmp/wld-lid-state-testing"));
}

static void laptop_lid_closed_open(void **state) {
	create_state_file("something OpEn something something\n");
	assert_false(laptop_lid_closed_path("/tmp/wld-lid-state-testing"));
}

static void laptop_lid_closed_closed(void **state) {
	create_state_file("something ClOsEd something something\n");
	assert_true(laptop_lid_closed_path("/tmp/wld-lid-state-testing"));
}

#define laptop_tests \
	cmocka_unit_test_setup_teardown(laptop_lid_closed_missing_path, laptop_lid_closed_setup_teardown, laptop_lid_closed_setup_teardown), \
	cmocka_unit_test_setup_teardown(laptop_lid_closed_missing_file, laptop_lid_closed_setup_teardown, laptop_lid_closed_setup_teardown), \
	cmocka_unit_test_setup_teardown(laptop_lid_closed_open, laptop_lid_closed_setup_teardown, laptop_lid_closed_setup_teardown), \
	cmocka_unit_test_setup_teardown(laptop_lid_closed_closed, laptop_lid_closed_setup_teardown, laptop_lid_closed_setup_teardown)


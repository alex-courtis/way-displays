#include "tst.h"

#include "assert-log.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "head.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "wlr-output-management-unstable-v1.h"

#include "listeners.h"

static int before_each(void **state) {
	assert_logs_empty_before();

	return 0;
}

static void preferred__first(void **state) {
	struct Head *head = head_init();
	struct Mode *mode_existing = mode_init_h_whr(head, 3840, 2160, 60000);
	struct Mode *mode_pref = mode_init_h_whr(head, 2560, 1440, 30000);

	pset_add(head->modes, mode_existing);
	pset_add(head->modes, mode_pref);

	zwlr_output_mode_listener()->preferred(mode_pref, NULL);

	assert_false(mode_existing->preferred);
	assert_true(mode_pref->preferred);

	assert_logs_empty();

	head_free(head);
}

static void preferred__subsequent(void **state) {
	struct Head *head = head_init_name("NAM");
	struct Mode *mode_existing = mode_init_h_whr(head, 3840, 2160, 60000);
	mode_existing->preferred = true;
	struct Mode *mode_subsequent = mode_init_h_whr(head, 2560, 1440, 30000);

	pset_add(head->modes, mode_existing);
	pset_add(head->modes, mode_subsequent);

	zwlr_output_mode_listener()->preferred(mode_subsequent, NULL);

	assert_log(INFO, "\nNAM: multiple preferred modes advertised: using initial 3840x2160@60Hz (60,000mHz) (preferred), ignoring 2560x1440@30Hz (30,000mHz)\n");
	assert_logs_empty();

	assert_true(mode_existing->preferred);
	assert_false(mode_subsequent->preferred);

	head_free(head);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_B(preferred__first),
		TEST_B(preferred__subsequent)
	};

	return RUN(tests);
}


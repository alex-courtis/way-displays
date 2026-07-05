#include "tst.h"

#include "assert-log.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdlib.h>

#include "head.h"
#include "log.h"
#include "pset.h"
#include "wlr-output-management-unstable-v1.h"

#include "listeners.h"

static int before_each(void **state) {
	assert_logs_empty_before();

	return 0;
}

static void preferred__first(void **state) {
	struct Head *head = head_init();
	const struct Mode *mode_existing = mode_init_h_whr(head, 3840, 2160, 60000);
	struct Mode *mode_pref = mode_init_h_whr(head, 2560, 1440, 30000);

	pset_add(head->modes, mode_existing);
	pset_add(head->modes, mode_pref);

	zwlr_output_mode_listener()->preferred(mode_pref, NULL);

	assert_ptr_equal(head->mode_preferred, mode_pref);

	assert_logs_empty();

	head_free(head);
}

static void preferred__subsequent(void **state) {
	struct Head *head = head_init_name("NAM");
	struct Mode *mode_existing = mode_init_h_whr(head, 3840, 2160, 60000);
	struct Mode *mode_subsequent = mode_init_h_whr(head, 2560, 1440, 30000);

	head->mode_preferred = mode_existing;

	pset_add(head->modes, mode_existing);
	pset_add(head->modes, mode_subsequent);

	zwlr_output_mode_listener()->preferred(mode_subsequent, NULL);

	assert_log(INFO, "\nNAM: multiple preferred modes advertised: using initial 3840x2160@60Hz (60,000mHz) (preferred), ignoring 2560x1440@30Hz (30,000mHz)\n");
	assert_logs_empty();

	assert_ptr_equal(head->mode_preferred, mode_existing);

	head_free(head);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_B(preferred__first),
		TEST_B(preferred__subsequent)
	};

	return RUN(tests);
}


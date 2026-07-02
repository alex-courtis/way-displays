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
	struct WlrMode *wlr_mode_existing = wlr_mode_init(head, NULL, 3840, 2160, 60000, false);
	struct WlrMode *wlr_mode_pref = wlr_mode_init(head, NULL, 2560, 1440, 30000, false);

	pset_add(head->wlr_modes, wlr_mode_existing);
	pset_add(head->wlr_modes, wlr_mode_pref);

	zwlr_output_mode_listener()->preferred(wlr_mode_pref, NULL);

	assert_false(wlr_mode_existing->preferred);
	assert_true(wlr_mode_pref->preferred);

	assert_logs_empty();

	head_free(head);
}

static void preferred__subsequent(void **state) {
	struct Head *head = head_init_name("NAM");
	struct WlrMode *wlr_mode_existing = wlr_mode_init(head, NULL, 3840, 2160, 60000, true);
	struct WlrMode *wlr_mode_subsequent = wlr_mode_init(head, NULL, 2560, 1440, 30000, false);

	pset_add(head->wlr_modes, wlr_mode_existing);
	pset_add(head->wlr_modes, wlr_mode_subsequent);

	zwlr_output_mode_listener()->preferred(wlr_mode_subsequent, NULL);

	assert_log(INFO, "\nNAM: multiple preferred modes advertised: using initial 3840x2160@60Hz (60,000mHz) (preferred), ignoring 2560x1440@30Hz (30,000mHz)\n");
	assert_logs_empty();

	assert_true(wlr_mode_existing->preferred);
	assert_false(wlr_mode_subsequent->preferred);

	head_free(head);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_B(preferred__first),
		TEST_B(preferred__subsequent)
	};

	return RUN(tests);
}


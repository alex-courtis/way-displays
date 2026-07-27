#include "tst.h"

#include "assert-pset.h"
#include "assert-log.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdlib.h>

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "enum.h"
#include "pset.h"

#include "cfg/disabled.h"

static int before_each(void **state) {
	g_cfg = cfg_default();

	return 0;
}

static int after_each(void **state) {
	g_cfg_destroy();

	return 0;
}
static void cfg_disabled_filter_conditional_clashes__null(void **state) {

	cfg_disabled_filter_conditional_clashes(NULL);

	assert_logs_empty();
}

static void cfg_disabled_filter_conditional_clashes__no_clash(void **state) {
	const struct Pset *ipc_disableds = cfg_disabled_pset_init();

	pset_add(ipc_disableds, disabled_nd("both"));
	pset_add(ipc_disableds, disabled_nd("ipc only"));

	pset_add(g_cfg->disableds, disabled_nd("both"));
	pset_add(g_cfg->disableds, disabled_nd("cfg only"));

	const struct Pset *expected = cfg_disabled_pset_init();
	pset_add(expected, disabled_nd("both"));
	pset_add(expected, disabled_nd("ipc only"));

	cfg_disabled_filter_conditional_clashes(ipc_disableds);

	assert_pset_equal(ipc_disableds, expected);

	pset_free_vals(ipc_disableds);
	pset_free_vals(expected);

	assert_logs_empty();
}

static void cfg_disabled_filter_conditional_clashes__clash(void **state) {
	const struct Pset *ipc_disableds = cfg_disabled_pset_init();

	pset_add(ipc_disableds, disabled_nd("not conditional"));
	pset_add(ipc_disableds, disabled_nd("conditional"));
	pset_add(ipc_disableds, disabled_nd("ipc only"));

	pset_add(g_cfg->disableds, disabled_nd("not conditional"));
	const struct CfgDisabled *conditional = disabled_nd("conditional");
	pset_add(conditional->conditions, cfg_condition_init());
	pset_add(g_cfg->disableds, conditional);
	pset_add(g_cfg->disableds, disabled_nd("cfg only"));

	const struct Pset *expected = cfg_disabled_pset_init();
	pset_add(expected, disabled_nd("not conditional"));
	pset_add(expected, disabled_nd("ipc only"));

	cfg_disabled_filter_conditional_clashes(ipc_disableds);

	assert_pset_equal(ipc_disableds, expected);

	pset_free_vals(ipc_disableds);
	pset_free_vals(expected);

	assert_log(INFO,
			"\nIgnoring DISABLED for conditional as it is DISABLED conditionally\n"
			);

	assert_logs_empty();
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(cfg_disabled_filter_conditional_clashes__null),
		TEST_BA(cfg_disabled_filter_conditional_clashes__no_clash),
		TEST_BA(cfg_disabled_filter_conditional_clashes__clash),
	};

	return RUN(tests);
}


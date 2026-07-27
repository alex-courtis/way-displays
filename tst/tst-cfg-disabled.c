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

static void cfg_disabled_filter_conditional_clashes__substring(void **state) {
	const struct Pset *ipc_disableds = cfg_disabled_pset_init();

	pset_add(ipc_disableds, disabled_nd("has conditions"));
	pset_add(ipc_disableds, disabled_nd("cond"));
	pset_add(ipc_disableds, disabled_nd("ipc only"));

	const struct CfgDisabled *conditional = disabled_nd("condit");
	pset_add(conditional->conditions, cfg_condition_init());
	pset_add(g_cfg->disableds, conditional);

	const struct Pset *expected = cfg_disabled_pset_init();
	pset_add(expected, disabled_nd("ipc only"));

	cfg_disabled_filter_conditional_clashes(ipc_disableds);

	assert_pset_equal(ipc_disableds, expected);

	pset_free_vals(ipc_disableds);
	pset_free_vals(expected);

	assert_log(INFO,
			"\nIgnoring DISABLED for 'has conditions' as it is conditionally DISABLED 'condit'\n"
			"\nIgnoring DISABLED for 'cond' as it is conditionally DISABLED 'condit'\n"
			);

	assert_logs_empty();
}

static void cfg_disabled_filter_conditional_clashes__regex_req(void **state) {
	const struct Pset *ipc_disableds = cfg_disabled_pset_init();

	pset_add(ipc_disableds, disabled_nd("!has"));
	pset_add(ipc_disableds, disabled_nd("!more"));
	pset_add(ipc_disableds, disabled_nd("!ipc only"));

	const struct CfgDisabled *conditional = disabled_nd("has conditions");
	pset_add(conditional->conditions, cfg_condition_init());
	pset_add(g_cfg->disableds, conditional);
	conditional = disabled_nd("more conditions");
	pset_add(conditional->conditions, cfg_condition_init());
	pset_add(g_cfg->disableds, conditional);

	const struct Pset *expected = cfg_disabled_pset_init();
	pset_add(expected, disabled_nd("!ipc only"));

	cfg_disabled_filter_conditional_clashes(ipc_disableds);

	assert_pset_equal(ipc_disableds, expected);

	pset_free_vals(ipc_disableds);
	pset_free_vals(expected);

	assert_log(INFO,
			"\nIgnoring DISABLED for '!has' as it is conditionally DISABLED 'has conditions'\n"
			"\nIgnoring DISABLED for '!more' as it is conditionally DISABLED 'more conditions'\n"
			);

	assert_logs_empty();
}

static void cfg_disabled_filter_conditional_clashes__regex_cfg(void **state) {
	const struct Pset *ipc_disableds = cfg_disabled_pset_init();

	pset_add(ipc_disableds, disabled_nd("cond"));
	pset_add(ipc_disableds, disabled_nd("conditional"));
	pset_add(ipc_disableds, disabled_nd("!cond"));
	pset_add(ipc_disableds, disabled_nd("!ipc only"));

	const struct CfgDisabled *conditional = disabled_nd("!cond");
	pset_add(conditional->conditions, cfg_condition_init());
	pset_add(g_cfg->disableds, conditional);

	const struct Pset *expected = cfg_disabled_pset_init();
	pset_add(expected, disabled_nd("!ipc only"));

	cfg_disabled_filter_conditional_clashes(ipc_disableds);

	assert_pset_equal(ipc_disableds, expected);

	pset_free_vals(ipc_disableds);
	pset_free_vals(expected);

	assert_log(INFO,
			"\nIgnoring DISABLED for 'cond' as it is conditionally DISABLED '!cond'\n"
			"\nIgnoring DISABLED for 'conditional' as it is conditionally DISABLED '!cond'\n"
			"\nIgnoring DISABLED for '!cond' as it is conditionally DISABLED '!cond'\n"
			);

	assert_logs_empty();
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(cfg_disabled_filter_conditional_clashes__null),
		TEST_BA(cfg_disabled_filter_conditional_clashes__no_clash),
		TEST_BA(cfg_disabled_filter_conditional_clashes__substring),
		TEST_BA(cfg_disabled_filter_conditional_clashes__regex_req),
		TEST_BA(cfg_disabled_filter_conditional_clashes__regex_cfg),
	};

	return RUN(tests);
}


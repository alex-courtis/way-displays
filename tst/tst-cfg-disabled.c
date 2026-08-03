#include "tst.h"

#include "assert-spmap.h"
#include "assert-log.h"

#include <cmocka.h>
#include <stdlib.h>

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "enum.h"
#include "pset.h"
#include "spmap.h"

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
	const struct SPmap *ipc_disableds = cfg_disabled_spmap_init();

	spmap_put(ipc_disableds, "both", cfg_disabled_init());
	spmap_put(ipc_disableds, "ipc only", cfg_disabled_init());

	spmap_put(g_cfg->disableds, "both", cfg_disabled_init());
	spmap_put(g_cfg->disableds, "cfg only", cfg_disabled_init());

	const struct SPmap *expected = cfg_disabled_spmap_init();
	spmap_put(expected, "both", cfg_disabled_init());
	spmap_put(expected, "ipc only", cfg_disabled_init());

	cfg_disabled_filter_conditional_clashes(ipc_disableds);

	assert_spmap_equal(ipc_disableds, expected);

	spmap_free_vals(ipc_disableds);
	spmap_free_vals(expected);

	assert_logs_empty();
}

static void cfg_disabled_filter_conditional_clashes__substring(void **state) {
	const struct SPmap *ipc_disableds = cfg_disabled_spmap_init();

	spmap_put(ipc_disableds, "has conditions", cfg_disabled_init());
	spmap_put(ipc_disableds, "cond", cfg_disabled_init());
	spmap_put(ipc_disableds, "ipc only", cfg_disabled_init());

	const struct CfgDisabled *conditional = cfg_disabled_init();
	pset_add(conditional->conditions, cfg_condition_init());
	spmap_put(g_cfg->disableds, "condit", conditional);

	const struct SPmap *expected = cfg_disabled_spmap_init();
	spmap_put(expected, "ipc only", cfg_disabled_init());

	cfg_disabled_filter_conditional_clashes(ipc_disableds);

	assert_spmap_equal(ipc_disableds, expected);

	spmap_free_vals(ipc_disableds);
	spmap_free_vals(expected);

	assert_log(INFO,
			"\nIgnoring DISABLED for 'has conditions' as it is conditionally DISABLED 'condit'\n"
			"\nIgnoring DISABLED for 'cond' as it is conditionally DISABLED 'condit'\n"
			);

	assert_logs_empty();
}

static void cfg_disabled_filter_conditional_clashes__regex_req(void **state) {
	const struct SPmap *ipc_disableds = cfg_disabled_spmap_init();

	spmap_put(ipc_disableds, "!has", cfg_disabled_init());
	spmap_put(ipc_disableds, "!more", cfg_disabled_init());
	spmap_put(ipc_disableds, "!ipc only", cfg_disabled_init());

	const struct CfgDisabled *conditional = cfg_disabled_init();
	pset_add(conditional->conditions, cfg_condition_init());
	spmap_put(g_cfg->disableds, "has conditions", conditional);

	conditional = cfg_disabled_init();
	pset_add(conditional->conditions, cfg_condition_init());
	spmap_put(g_cfg->disableds, "more conditions", conditional);

	const struct SPmap *expected = cfg_disabled_spmap_init();
	spmap_put(expected, "!ipc only", cfg_disabled_init());

	cfg_disabled_filter_conditional_clashes(ipc_disableds);

	assert_spmap_equal(ipc_disableds, expected);

	spmap_free_vals(ipc_disableds);
	spmap_free_vals(expected);

	assert_log(INFO,
			"\nIgnoring DISABLED for '!has' as it is conditionally DISABLED 'has conditions'\n"
			"\nIgnoring DISABLED for '!more' as it is conditionally DISABLED 'more conditions'\n"
			);

	assert_logs_empty();
}

static void cfg_disabled_filter_conditional_clashes__regex_cfg(void **state) {
	const struct SPmap *ipc_disableds = cfg_disabled_spmap_init();

	spmap_put(ipc_disableds, "cond", cfg_disabled_init());
	spmap_put(ipc_disableds, "conditional", cfg_disabled_init());
	spmap_put(ipc_disableds, "!cond", cfg_disabled_init());
	spmap_put(ipc_disableds, "!ipc only", cfg_disabled_init());

	const struct CfgDisabled *conditional = cfg_disabled_init();
	pset_add(conditional->conditions, cfg_condition_init());
	spmap_put(g_cfg->disableds, "!cond", conditional);

	const struct SPmap *expected = cfg_disabled_spmap_init();
	spmap_put(expected, "!ipc only", cfg_disabled_init());

	cfg_disabled_filter_conditional_clashes(ipc_disableds);

	assert_spmap_equal(ipc_disableds, expected);

	spmap_free_vals(ipc_disableds);
	spmap_free_vals(expected);

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


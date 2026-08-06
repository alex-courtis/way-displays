#include "tst.h"

#include "assert-log.h"
#include "assert-spmap.h"
#include "asserts.h"
#include "data.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "displ.h"
#include "enum.h"
#include "head.h"
#include "lid.h"
#include "ppmap.h"
#include "pset.h"
#include "spmap.h"
#include "sset.h"

#include "cfg/disabled.h"

static int before_each(void **state) {
	g_cfg = cfg_default();

	g_displ = displ_init();

	g_lid = NULL;

	return 0;
}

static int after_each(void **state) {
	g_cfg_destroy();

	displ_free(g_displ);

	free(g_lid);
	g_lid = NULL;

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

static void cfg_disabled_applies_to_head__name_desc_conditions(void **state) {
	struct Head *head = head_n("DP-1");
	ppmap_put(g_displ->heads, H1, head);

	const struct CfgDisabled *disabled = cfg_disabled_init();

	const struct SPmap *disableds = cfg_disabled_spmap_init();
	spmap_put(disableds, "!DP-[1-5]", disabled);

	// unconditional met
	assert_true(cfg_disabled_applies_to_head(disableds, head, false));
	assert_nul(head->disabled_condition_desc);

	const struct CfgCondition *condition_unplugged = cfg_condition_init();
	sset_add(condition_unplugged->unplugged, "DP-99");
	pset_add(disabled->conditions, condition_unplugged);

	// unplugged met
	assert_true(cfg_disabled_applies_to_head(disableds, head, false));
	assert_str_equal(head->disabled_condition_desc, "DP-99 unplugged");

	struct CfgCondition *condition_lid = cfg_condition_init();
	condition_lid->lid = LID_NOT_PRESENT;
	pset_add(disabled->conditions, condition_lid);

	// unplugged still met
	assert_true(cfg_disabled_applies_to_head(disableds, head, false));
	assert_str_equal(head->disabled_condition_desc, "DP-99 unplugged");

	sset_add(condition_unplugged->unplugged, "DP-1");

	// lid now met
	assert_true(cfg_disabled_applies_to_head(disableds, head, false));
	assert_str_equal(head->disabled_condition_desc, "lid not present");

	g_lid = calloc(1, sizeof(struct Lid));
	g_lid->closed = true;

	// none met
	assert_false(cfg_disabled_applies_to_head(disableds, head, false));
	assert_nul(head->disabled_condition_desc);

	spmap_free_vals(disableds);
}

static void cfg_disabled_applies_to_head__name_desc_only(void **state) {
	const struct CfgDisabled *disabled = cfg_disabled_init();

	struct Head *head_disabled = head_n("DP-1");

	const struct SPmap *disableds = cfg_disabled_spmap_init();
	spmap_put(disableds, "DP-1", disabled);

	assert_true(cfg_disabled_applies_to_head(disableds, head_disabled, false));

	assert_nul(head_disabled->disabled_condition_desc);

	struct Head *head_enabled = head_n("DP-2");

	assert_false(cfg_disabled_applies_to_head(disableds, head_enabled, false));

	assert_nul(head_disabled->disabled_condition_desc);

	spmap_free_vals(disableds);

	head_free(head_enabled);
	head_free(head_disabled);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(cfg_disabled_filter_conditional_clashes__null),
		TEST_BA(cfg_disabled_filter_conditional_clashes__no_clash),
		TEST_BA(cfg_disabled_filter_conditional_clashes__substring),
		TEST_BA(cfg_disabled_filter_conditional_clashes__regex_req),
		TEST_BA(cfg_disabled_filter_conditional_clashes__regex_cfg),

		TEST_BA(cfg_disabled_applies_to_head__name_desc_conditions),
		TEST_BA(cfg_disabled_applies_to_head__name_desc_only),
	};

	return RUN(tests);
}


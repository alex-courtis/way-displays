#include "tst.h"

#include "assert-cfg.h"
#include "assert-log.h"
#include "asserts.h"
#include "expects.h"
#include "data.h"
#include "util-col.h"
#include "util-file.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cfg/cfg.h"
#include "cfg/condition.h"
#include "cfg/disabled.h"
#include "enum.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "plist.h"
#include "ppmap.h"
#include "pset.h"
#include "simap.h"
#include "sset.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml/unmarshal-types.h"

#include "yaml/unmarshal.h"

// expected will be free'd, log_path is optional WARNING
static void _check_unmarshalled_cfg(const char *yaml_path, struct Cfg *expected, const char *log_path, const char * const file, const int line) {
	struct Cfg *actual = yaml_unmarshal_file(yaml_path, yaml_root_to_cfg);
	_assert_non_nul(actual, "actual", file, line);

	_assert_cfg(actual, expected, true, "assert_cfg_equal", file, line);

	if (log_path) {
		char *expected_log = read_file(log_path);
		_assert_log(WARNING, expected_log, file, line);
		free(expected_log);
	}

	_assert_logs_empty(file, line);

	cfg_free(actual);
	cfg_free(expected);
}
#define check_unmarshalled_cfg_v1(yaml_path, expected, log_path) _check_unmarshalled_cfg(yaml_path, expected, log_path, __FILE__, __LINE__)

static void yaml_root_to_cfg__ok(void **state) {
	// only once
	expect_any(__wrap_cfg_migrate_v1, cfg);
	expect_str(__wrap_cfg_migrate_v1, v1_laptop_display_prefix, "ldp");

	check_unmarshalled_cfg_v1("tst/yaml/v1/cfg-all.yaml", cfg_all(), NULL);

	assert_logs_empty();
}

static void yaml_root_to_cfg__transform(void **state) {
	struct Cfg *expected = cfg_init();
	simap_put(expected->transforms, "one", WL_OUTPUT_TRANSFORM_FLIPPED);

	// only once
	expect_any(__wrap_cfg_migrate_v1, cfg);
	expect_str(__wrap_cfg_migrate_v1, v1_laptop_display_prefix, NULL);

	check_unmarshalled_cfg_v1("tst/yaml/v1/cfg-transform.yaml", expected, "tst/yaml/v1/cfg-transform.log");

	assert_logs_empty();
}

static void yaml_root_to_cfg__scale(void **state) {
	struct Cfg *expected = cfg_init();
	simap_put(expected->scales, "three", 3000);

	// only once
	expect_any(__wrap_cfg_migrate_v1, cfg);
	expect_str(__wrap_cfg_migrate_v1, v1_laptop_display_prefix, NULL);

	check_unmarshalled_cfg_v1("tst/yaml/v1/cfg-scale.yaml", expected, "tst/yaml/v1/cfg-scale.log");

	assert_logs_empty();
}

static void yaml_root_to_cfg__mode(void **state) {
	struct Cfg *expected = cfg_init();

	spmap_put_many(expected->modes,
			"max_override", mode_whr_max(1920, 1080, 12340),
			"five", mode_whr(1920, 1080, 12340),
			"seven", mode_whr_max(-1, -1, -1),
			NULL);

	// only once
	expect_any(__wrap_cfg_migrate_v1, cfg);
	expect_str(__wrap_cfg_migrate_v1, v1_laptop_display_prefix, NULL);

	check_unmarshalled_cfg_v1("tst/yaml/v1/cfg-mode.yaml", expected, "tst/yaml/v1/cfg-mode.log");

	assert_logs_empty();
}

static void yaml_root_to_cfg__disabled(void **state) {
	struct Cfg *expected = cfg_init();

	struct CfgDisabled *disabled_consolidated = cfg_disabled_init();

	struct CfgCondition *cond = cfg_condition_init();
	sset_add_many(cond->plugged, "ONE", "TWO", NULL);
	pset_add(disabled_consolidated->conditions, cond);

	cond = cfg_condition_init();
	sset_add_many(cond->plugged, "ONE", "TWO", NULL);
	cond->lid = LID_OPEN;
	pset_add(disabled_consolidated->conditions, cond);

	cond = cfg_condition_init();
	sset_add_many(cond->unplugged, "THREE", NULL);
	cond->lid = LID_CLOSED;
	pset_add(disabled_consolidated->conditions, cond);

	cond = cfg_condition_init();
	sset_add(cond->plugged, "FOUR");
	pset_add(disabled_consolidated->conditions, cond);
	sset_add_many(cond->unplugged, "FIVE", "SIX", NULL);
	cond->lid = LID_NOT_PRESENT;
	pset_add(disabled_consolidated->conditions, cond);

	spmap_put_many(expected->disableds,
			"eight",                      cfg_disabled_init(),
			"EIGHT",                      cfg_disabled_init(),
			"nine",                       cfg_disabled_init(),
			"twelve",                     disabled_consolidated,
			"BAD_DISABLED_IFS",           cfg_disabled_init(),
			"MISTYPED_IF_SCALAR",         cfg_disabled_init(),
			"MISTYPED_IF_MAP",            cfg_disabled_init(),
			"MISTYPED_UN_PLUGGED_MAP",    cfg_disabled_init(),
			"MISTYPED_LID_MAP",           cfg_disabled_init(),
			"BAD_LID_ENUM",               cfg_disabled_init(),
			"NO_VALID_CONDITIONS",        cfg_disabled_init(),
			NULL);

	// only once
	expect_any(__wrap_cfg_migrate_v1, cfg);
	expect_str(__wrap_cfg_migrate_v1, v1_laptop_display_prefix, NULL);

	check_unmarshalled_cfg_v1("tst/yaml/v1/cfg-disabled.yaml", expected, "tst/yaml/v1/cfg-disabled.log");

	assert_logs_empty();
}

static void yaml_root_to_ipc_request__cfg_set(void **state) {
	struct Cfg *expected = cfg_all();

	char *yaml = read_file("tst/yaml/v1/ipc-request-cfg-set.yaml");

	struct IpcRequest *actual = yaml_unmarshal_str(yaml, yaml_root_to_ipc_request, "ipc request");

	assert_non_nul(actual);
	assert_int_equal(actual->command, CFG_SET);
	assert_int_equal(actual->log_threshold, ERROR);

	assert_cfg_equal(actual->cfg, expected);

	ipc_request_free(actual);
	cfg_free(expected);
	free(yaml);

	assert_logs_empty();
}

static void yaml_root_to_ipc_response_plist__map(void **state) {
	char *yaml = read_file("tst/yaml/v1/ipc-responses-map.yaml");

	expect_function_call(__wrap_lid_free);

	const struct Plist *responses = yaml_unmarshal_str(yaml, yaml_root_to_ipc_response_plist, "ipc response");

	assert_non_nul(responses);
	assert_int_equal(plist_size(responses), 1);

	const struct IpcResponse *response = plist_at(responses, 0);

	assert_true(response->status.done);
	assert_int_equal(response->status.rc, 2);

	assert_non_nul(response->lid);
	assert_true(response->lid->closed);
	assert_str_equal(response->lid->device_path, "/path/to/lid");

	assert_non_nul(response->cfg);
	struct Cfg *expected_cfg = cfg_all();
	assert_cfg_equal(response->cfg, expected_cfg);

	assert_int_equal(plist_size(response->heads), 1);
	const struct Head *head = plist_at(response->heads, 0);

	assert_str_equal(head->name, "name");
	assert_str_equal(head->description, "desc");
	assert_int_equal(head->width_mm, 1);
	assert_int_equal(head->height_mm, 2);
	assert_str_equal(head->make, "make");
	assert_str_equal(head->model, "model");
	assert_str_equal(head->serial_number, "serial");

	assert_int_equal(head->cur.scale, wl_fixed_from_double(4));
	assert_true(head->cur.enabled);
	assert_int_equal(head->cur.x, 5);
	assert_int_equal(head->cur.y, 6);
	assert_int_equal(head->cur.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED);

	assert_int_equal(head->des.scale, wl_fixed_from_double(7.0));
	assert_true(head->des.enabled);
	assert_int_equal(head->des.x, 8);
	assert_int_equal(head->des.y, 9);
	assert_int_equal(head->des.adaptive_sync, ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);

	const struct Mode *mode_current = ppmap_get(head->modes, head->cur.zmode);
	assert_non_nul(mode_current);
	assert_int_equal(mode_current->width, 10);
	assert_int_equal(mode_current->height, 11);
	assert_int_equal(mode_current->refresh_mhz, 12);

	const struct Mode *mode_desired = ppmap_get(head->modes, head->des.zmode);
	assert_non_nul(mode_desired);
	assert_int_equal(mode_desired->width, 13);
	assert_int_equal(mode_desired->height, 14);
	assert_int_equal(mode_desired->refresh_mhz, 15);

	assert_int_equal(ppmap_size(head->modes), 2);
	const struct Mode *mode1 = ppmap_at(head->modes, 0).val;
	assert_non_nul(mode1);
	assert_int_equal(mode1->width, 10);
	assert_int_equal(mode1->height, 11);
	assert_int_equal(mode1->refresh_mhz, 12);

	const struct Mode *mode2 = ppmap_at(head->modes, 1).val;
	assert_non_nul(mode2);
	assert_int_equal(mode2->width, 13);
	assert_int_equal(mode2->height, 14);
	assert_int_equal(mode2->refresh_mhz, 15);

	const struct Mode *mode_pref = ppmap_get(head->modes, head->zmode_pref);
	assert_non_nul(mode_pref);
	assert_int_equal(mode_pref->width, 10);
	assert_int_equal(mode_pref->height, 11);
	assert_int_equal(mode_pref->refresh_mhz, 12);

	assert_int_equal(ppmap_size(head->modes_failed), 1);

	const struct Mode *mode_failed = ppmap_at(head->modes_failed, 0).val;
	assert_non_nul(mode_failed);
	assert_int_equal(mode_failed->width, 16);
	assert_int_equal(mode_failed->height, 17);
	assert_int_equal(mode_failed->refresh_mhz, 18);

	assert_int_equal(head->cur.transform, 3);
	assert_int_equal(head->des.transform, 4);

	assert_int_equal(plist_size(response->log_cap_lines), 3);
	const struct LogCapLine *line = plist_at(response->log_cap_lines, 0);
	assert_int_equal(line->threshold, WARNING);
	assert_str_equal(line->line, "war");

	line = plist_at(response->log_cap_lines, 1);
	assert_int_equal(line->threshold, ERROR);
	assert_str_equal(line->line, "err");

	line = plist_at(response->log_cap_lines, 2);
	assert_non_nul(line);
	assert_int_equal(line->threshold, FATAL);
	assert_str_equal(line->line, "fat");

	assert_int_equal(head->overrided_enabled, OverrideFalse);

	plist_free_vals(responses);
	cfg_free(expected_cfg);
	free(yaml);

	assert_logs_empty();
}

int main(void) {

	const struct CMUnitTest tests[] = {
		TEST(yaml_root_to_cfg__ok),
		TEST(yaml_root_to_cfg__transform),
		TEST(yaml_root_to_cfg__scale),
		TEST(yaml_root_to_cfg__mode),
		TEST(yaml_root_to_cfg__disabled),

		TEST(yaml_root_to_ipc_request__cfg_set),

		TEST(yaml_root_to_ipc_response_plist__map),
	};

	return RUN(tests);
}


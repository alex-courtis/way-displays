#include <cmocka.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <wayland-util.h>

#include "displ.h"
#include "enum.h"
#include "head.h"
#include "mode.h"
#include "ppmap.h"
#include "pset.h"
#include "ssmap.h"
#include "wlr-output-management-unstable-v1.h"
#include "yaml/marshal.h"
#include "yaml/unmarshal.h"

// any mocks that require __real should be in the test c itself, as they require the --wrap= to be explicitly specified

/*
 * fs
 */

bool __wrap_fs_mkdir_p(char *path, mode_t mode) {
	check_expected_ptr(path);
	check_expected_int(mode);
	return mock_type(bool);
}

char *__wrap_fs_canonical_path(char *path) {
	check_expected_ptr(path);
	return mock_ptr_type_checked(char*);
}

/*
 * fds
 */

void __wrap_fd_wd_cfg_dir_create(void) {
	function_called();
}

void __wrap_fd_wd_cfg_dir_destroy(void) {
	function_called();
}

/*
 * head
 */

const struct zwlr_output_mode_v1 *__wrap_head_find_mode(struct Head * const head) {
	check_expected_ptr(head);
	return mock_ptr_type_checked(struct zwlr_output_mode_v1*);
}

wl_fixed_t __wrap_head_auto_scale(struct Head *head) {
	check_expected_ptr(head);
	return mock_type(wl_fixed_t);
}

/*
 * print
 */

void __wrap_print_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct zwlr_output_mode_v1* const zmode) {
	check_expected_int(t);
	check_expected_ptr(head);
	check_expected_ptr(zmode);
}

void __wrap_print_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head) {
	check_expected_int(t);
	check_expected_ptr(head);
}

void __wrap_print_head(const enum LogThreshold t, const enum InfoEvent event, const struct Head * const head) {
	check_expected_int(t);
	check_expected_int(event);
	check_expected_ptr(head);
}

void __wrap_print_head_map(const enum LogThreshold t, const enum InfoEvent event, const struct PPmap * const heads) {
	check_expected_int(t);
	check_expected_int(event);
	check_expected_ptr(heads);
}

void __wrap_print_head_set(const enum LogThreshold t, const enum InfoEvent event, const struct Pset * const heads) {
	check_expected_int(t);
	check_expected_int(event);
	check_expected_ptr(heads);
}

/*
 * callback
 */

void __wrap_callback(const enum LogThreshold t, const char * const msg1, const char * const msg2) {
	check_expected_int(t);
	check_expected_ptr(msg1);
	check_expected_ptr(msg2);
}

void __wrap_callback_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct zwlr_output_mode_v1* const zmode) {
	check_expected_int(t);
	check_expected_ptr(head);
	check_expected_ptr(zmode);
}

void __wrap_callback_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head) {
	check_expected_int(t);
	check_expected_ptr(head);
}

/*
 * mode
 */

double __wrap_mode_dpi(const struct Mode* const mode, int32_t width_mm, int32_t height_mm) {
	check_expected_ptr(mode);
	check_expected_int(width_mm);
	check_expected_int(height_mm);
	return mock_type(double);
}

const struct Mode *__wrap_mode_best_satisfying(const struct Mode * const mode_target, const struct PPmap* const modes) {
	check_expected_ptr(mode_target);
	check_expected_ptr(modes);
	return mock_ptr_type_checked(struct Mode*);
}

const struct Mode *__wrap_mode_max_refresh(const struct Mode* const mode_target, const struct PPmap* modes) {
	check_expected_ptr(mode_target);
	check_expected_ptr(modes);
	return mock_ptr_type_checked(struct Mode*);
}

/*
 * process
 */

void __wrap_spawn_sh_cmd(const char * const command, const struct SSmap * const env) {
	check_expected_ptr(command);
	check_expected_ptr(env);
}

void __wrap_wd_exit(const int __status) {
	check_expected_int(__status);
}

void __wrap_wd_exit_message(const int __status) {
	check_expected_int(__status);
}

/*
 * wl_wrappers
 */

struct zwlr_output_configuration_v1 *__wrap_create_zwlr_output_config_listener(struct Displ *displ) {
	check_expected_ptr(displ);
	return mock_ptr_type_checked(struct zwlr_output_configuration_v1*);
}

struct zwlr_output_configuration_head_v1 * __wrap__zwlr_output_configuration_v1_enable_head(struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1, struct zwlr_output_head_v1 *head) {
	check_expected_ptr(zwlr_output_configuration_v1);
	check_expected_ptr(head);
	return mock_ptr_type_checked(struct zwlr_output_configuration_head_v1*);
}

void __wrap__zwlr_output_configuration_v1_disable_head(struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1, struct zwlr_output_head_v1 *head) {
	check_expected_ptr(zwlr_output_configuration_v1);
	check_expected_ptr(head);
}

void __wrap__zwlr_output_configuration_v1_apply(struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	check_expected_ptr(zwlr_output_configuration_v1);
}

void __wrap__zwlr_output_configuration_head_v1_set_mode(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, struct zwlr_output_mode_v1 *mode) {
	check_expected_ptr(zwlr_output_configuration_head_v1);
	check_expected_ptr(mode);
}

void __wrap__zwlr_output_configuration_head_v1_set_transform(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, int32_t transform) {
	check_expected_ptr(zwlr_output_configuration_head_v1);
	check_expected_int(transform);
}

void __wrap__zwlr_output_configuration_head_v1_set_scale(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, int32_t scale) {
	check_expected_ptr(zwlr_output_configuration_head_v1);
	check_expected_int(scale);
}

void __wrap__zwlr_output_configuration_head_v1_set_position(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, int32_t x, int32_t y) {
	check_expected_ptr(zwlr_output_configuration_head_v1);
	check_expected_int(x);
	check_expected_int(y);
}

void __wrap__zwlr_output_configuration_head_v1_set_adaptive_sync(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, uint32_t state) {
	check_expected_ptr(zwlr_output_configuration_head_v1);
	check_expected_int(state);
}

/*
 * yaml/
 */
char *__wrap_yaml_marshal(const void *data, fn_yaml_root_from_type fn, const char *human) {
	check_expected_ptr(data);
	check_expected_ptr(human);

	return mock_ptr_type_checked(char*);
}

void *__wrap_yaml_unmarshal_file(const char *path, fn_yaml_root_to_type fn) {
	check_expected_ptr(path);

	return mock_ptr_type_checked(struct Cfg*);
}

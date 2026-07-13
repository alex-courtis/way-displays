#include <cmocka.h>
#include <stdbool.h>
#include <sys/types.h>
#include <wayland-util.h>
#include <yaml.h>

#include "head.h"
#include "log.h"
#include "mode.h"
#include "pset.h"
#include "ssmap.h"
#include "yaml/marshal.h"
#include "yaml/unmarshal.h"

/*
 * fds
 */

bool __wrap_file_write(const char *path, const char *contents, const char *mode) {
	check_expected_ptr(path);
	check_expected_ptr(contents);
	check_expected_ptr(mode);
	return mock_type(bool);
}

bool __wrap_mkdir_p(char *path, mode_t mode) {
	check_expected_ptr(path);
	check_expected_int(mode);
	return mock_type(bool);
}

void __wrap_fd_wd_cfg_dir_create(void) {
	function_called();
}

void __wrap_fd_wd_cfg_dir_destroy(void) {
	function_called();
}

/*
 * head
 */

const struct Mode *__wrap_head_find_mode(struct Head * const head) {
	check_expected_ptr(head);
	return mock_ptr_type_checked(struct Mode*);
}

wl_fixed_t __wrap_head_auto_scale(struct Head *head) {
	check_expected_ptr(head);
	return mock_type(wl_fixed_t);
}

/*
 * print
 */

void __wrap_print_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct Mode * const mode) {
	check_expected_int(t);
	check_expected_ptr(head);
	check_expected_ptr(mode);
}

void __wrap_print_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head) {
	check_expected_int(t);
	check_expected_ptr(head);
}

/*
 * callback
 */

void __wrap_callback(const enum LogThreshold t, const char * const msg1, const char * const msg2) {
	check_expected_int(t);
	check_expected_ptr(msg1);
	check_expected_ptr(msg2);
}

void __wrap_callback_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct Mode * const mode) {
	check_expected_int(t);
	check_expected_ptr(head);
	check_expected_ptr(mode);
}

void __wrap_callback_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head) {
	check_expected_int(t);
	check_expected_ptr(head);
}

/*
 * mode
 */

double __wrap_mode_dpi(const struct Mode* const mode) {
	check_expected_ptr(mode);
	return mock_type(double);
}

const struct Mode *__wrap_mode_best_satisfying(const struct Mode * const mode_target, const struct Pset* const modes) {
	check_expected_ptr(mode_target);
	check_expected_ptr(modes);
	return mock_ptr_type_checked(struct Mode*);
}

const struct Mode *__wrap_mode_max_refresh(const struct Mode* const mode_target, const struct Pset* modes) {
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
 * fs
 */

char *__wrap_resolve_canonical_path(char *path) {
	check_expected_ptr(path);
	return mock_ptr_type_checked(char*);
}

/*
 * yaml-(un)marshal
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

/*
 * libyaml
 */

int __real_yaml_document_initialize(yaml_document_t *document, yaml_version_directive_t *version_directive, yaml_tag_directive_t *tag_directives_start, yaml_tag_directive_t *tag_directives_end, int start_implicit, int end_implicit);
int __wrap_yaml_document_initialize(yaml_document_t *document, yaml_version_directive_t *version_directive, yaml_tag_directive_t *tag_directives_start, yaml_tag_directive_t *tag_directives_end, int start_implicit, int end_implicit) {
	if (has_mock())
		return mock_int();
	else
		return __real_yaml_document_initialize(document, version_directive, tag_directives_start, tag_directives_end, start_implicit, end_implicit);
}

int __real_yaml_emitter_initialize(yaml_emitter_t *emitter);
int __wrap_yaml_emitter_initialize(yaml_emitter_t *emitter) {
	if (has_mock())
		return mock_int();
	else
		return __real_yaml_emitter_initialize(emitter);
}

int __real_yaml_emitter_open(yaml_emitter_t *emitter);
int __wrap_yaml_emitter_open(yaml_emitter_t *emitter) {
	if (has_mock())
		return mock_int();
	else
		return __real_yaml_emitter_open(emitter);
}

int __real_yaml_emitter_dump(yaml_emitter_t *emitter, yaml_document_t *document);
int __wrap_yaml_emitter_dump(yaml_emitter_t *emitter, yaml_document_t *document) {
	if (has_mock())
		return mock_int();
	else
		return __real_yaml_emitter_dump(emitter, document);
}

int __real_yaml_emitter_close(yaml_emitter_t *emitter);
int __wrap_yaml_emitter_close(yaml_emitter_t *emitter) {
	if (has_mock())
		return mock_int();
	else

		return __real_yaml_emitter_close(emitter);
}

int __real_yaml_parser_initialize(yaml_parser_t *parser);
int __wrap_yaml_parser_initialize(yaml_parser_t *parser) {
	if (has_mock())
		return mock_int();
	else
		return __real_yaml_parser_initialize(parser);
}

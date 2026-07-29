#include "tst.h"

#include "assert-log.h"
#include "asserts.h"
#include "data.h"
#include "util-file.h"

#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "cfg/cfg.h"
#include "displ.h"
#include "enum.h"
#include "fs.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "plist.h"
#include "str.h"

#include "yaml/marshal-types.h"
#include "yaml/marshal.h"

// these mocks are local to this test as they specifically require __real to be present i.e. explicitly wrapped

int __real_yaml_emitter_initialize(yaml_emitter_t *emitter);
int __wrap_yaml_emitter_initialize(yaml_emitter_t *emitter) { // cppcheck-suppress staticFunction
	return has_mock() ? mock_int() : __real_yaml_emitter_initialize(emitter);
}

int __real_yaml_emitter_open(yaml_emitter_t *emitter);
int __wrap_yaml_emitter_open(yaml_emitter_t *emitter) { // cppcheck-suppress staticFunction
	return has_mock() ? mock_int() : __real_yaml_emitter_open(emitter);
}

int __real_yaml_emitter_dump(yaml_emitter_t *emitter, yaml_document_t *document);
int __wrap_yaml_emitter_dump(yaml_emitter_t *emitter, yaml_document_t *document) { // cppcheck-suppress staticFunction
	return has_mock() ? mock_int() : __real_yaml_emitter_dump(emitter, document);
}

int __real_yaml_emitter_close(yaml_emitter_t *emitter);
int __wrap_yaml_emitter_close(yaml_emitter_t *emitter) { // cppcheck-suppress staticFunction
	return has_mock() ? mock_int() : __real_yaml_emitter_close(emitter);
}

int __real_yaml_document_initialize(yaml_document_t *document, yaml_version_directive_t *version_directive, yaml_tag_directive_t *tag_directives_start, yaml_tag_directive_t *tag_directives_end, int start_implicit, int end_implicit);
int __wrap_yaml_document_initialize(yaml_document_t *document, yaml_version_directive_t *version_directive, yaml_tag_directive_t *tag_directives_start, yaml_tag_directive_t *tag_directives_end, int start_implicit, int end_implicit) { // cppcheck-suppress staticFunction
	return has_mock() ? mock_int() : __real_yaml_document_initialize(document, version_directive, tag_directives_start, tag_directives_end, start_implicit, end_implicit);
}

static int before_each(void **state) {
	g_displ = displ_init();
	return 0;
}

static int after_each(void **state) {
	displ_free(g_displ);
	cfg_free(g_cfg);
	g_cfg = NULL;
	free(g_lid);
	g_lid = NULL;
	return 0;
}

static void _check_marshalled(char *actual, const char *expected_path, const char * const file, const int line) {
	_assert_non_nul(actual, "actual", file, line);

	char *expected = read_file_filter(expected_path, "# $schema:");

	if (strcmp(actual, expected) != 0) {
		const char *err = sprintf_alloc("check_marshalled\nactual.yaml:\n%s !=\nexpected.yaml:\n%s\n", actual, expected);
		fs_file_write("actual.yaml", actual, "w");
		fs_file_write("expected.yaml", expected, "w");
		fprintf(stderr, "%s:%d: %s", file, line, err);
		exit(1);
	}

	free(actual);
	free(expected);
}
#define check_marshalled(actual, expected_path) _check_marshalled(actual, expected_path, __FILE__, __LINE__)

static void yaml_root_from_cfg__ok(void **state) {
	struct Cfg *cfg = cfg_all();

	check_marshalled(yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg"), "tst/yaml/cfg-all.yaml");

	cfg_free(cfg);

	assert_logs_empty();
}

static void yaml_root_from_cfg__default(void **state) {
	struct Cfg *cfg = cfg_default();

	check_marshalled(yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg"), "tst/yaml/cfg-default.yaml");

	cfg_free(cfg);

	assert_logs_empty();
}

static void yaml_root_from_cfg__empty(void **state) {
	struct Cfg *cfg = cfg_init();

	check_marshalled(yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg"), "tst/yaml/empty.yaml");

	cfg_free(cfg);

	assert_logs_empty();
}


static void yaml_root_from_ipc_request__no_op(void **state) {
	struct IpcRequest *ipc_request = ipc_request_init(0);

	assert_nul(yaml_marshal(ipc_request, (fn_yaml_root_from_type)yaml_root_from_ipc_request, "ipc request"));

	assert_log(ERROR, "unable to marshal ipc request: missing OP\n");

	ipc_request_free(ipc_request);

	assert_logs_empty();
}

static void yaml_root_from_ipc_request__cfg_set(void **state) {
	struct IpcRequest *ipc_request = ipc_request_init(CFG_SET);
	ipc_request->log_threshold = ERROR;

	ipc_request->cfg = cfg_all();

	check_marshalled(yaml_marshal(ipc_request, (fn_yaml_root_from_type)yaml_root_from_ipc_request, "ipc request"), "tst/yaml/ipc-request-cfg-set.yaml");

	ipc_request_free(ipc_request);

	assert_logs_empty();
}

static void yaml_root_from_ipc_operation__map(void **state) {
	struct IpcOperation *ipc_operation = ipc_response();

	plist_append(ipc_operation->log_cap_lines, log_cap_line_init(ERROR, "err"));
	plist_append(ipc_operation->log_cap_lines, log_cap_line_init(FATAL, "fat"));
	ipc_operation_update_rc(ipc_operation);

	check_marshalled(yaml_marshal(ipc_operation, (fn_yaml_root_from_type)yaml_root_from_ipc_operation, "ipc response"), "tst/yaml/ipc-responses-map.yaml");

	ipc_operation_destroy(ipc_operation);

	assert_logs_empty();
}

static void yaml_root_from_ipc_operation__seq(void **state) {
	struct IpcOperation *ipc_operation = ipc_response();
	ipc_operation->request->command = LIST;

	check_marshalled(yaml_marshal(ipc_operation, (fn_yaml_root_from_type)yaml_root_from_ipc_operation, "ipc response"), "tst/yaml/ipc-responses-seq.yaml");

	ipc_operation_destroy(ipc_operation);

	assert_logs_empty();
}

static void yaml_marshal__yaml_document_initialize_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	will_return_int(__wrap_yaml_document_initialize, 0);

	const char *actual = yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_document_initialize failed\n");

	assert_logs_empty();
}

static void yaml_marshal__yaml_emitter_initialize_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	will_return_int(__wrap_yaml_emitter_initialize, 0);

	const char *actual = yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_initialize failed\n");

	assert_logs_empty();
}

static void yaml_marshal__yaml_emitter_open_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	will_return_int(__wrap_yaml_emitter_open, 0);

	const char *actual = yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_open failed\n");

	assert_logs_empty();
}

// also covers case of write_handler fail
static void yaml_marshal__yaml_emitter_dump_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	will_return_int(__wrap_yaml_emitter_dump, 0);

	const char *actual = yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_dump failed\n");

	assert_logs_empty();
}

static void yaml_marshal__yaml_emitter_close_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	will_return_int(__wrap_yaml_emitter_close, 0);

	const char *actual = yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(WARNING, "unable to marshal cfg: yaml_emitter_close failed\n");

	assert_logs_empty();
}

static void yaml_write_handler__empty(void **state) {
	char *data = NULL;

	char *buffer = strdup("1234");
	size_t size = 2;

	assert_int_equal(yaml_write_handler(&data, (unsigned char *)buffer, size), 1);

	assert_str_equal(data, "12");

	free(buffer);
	free(data);

	assert_logs_empty();
}

static void yaml_write_handler__append(void **state) {
	char *data = strdup("foo");

	char *buffer = strdup("1234");
	size_t size = 2;

	assert_int_equal(yaml_write_handler(&data, (unsigned char *)buffer, size), 1);

	assert_str_equal(data, "foo12");

	free(buffer);
	free(data);

	assert_logs_empty();
}

static void yaml_write_handler__no_data(void **state) {
	char *buffer = strdup("1234");
	size_t size = 2;

	assert_int_equal(yaml_write_handler(NULL, (unsigned char *)buffer, size), 0);

	free(buffer);

	assert_logs_empty();
}

int main(void) {

	const struct CMUnitTest tests[] = {
		TEST_BA(yaml_root_from_cfg__ok),
		TEST_BA(yaml_root_from_cfg__default),
		TEST_BA(yaml_root_from_cfg__empty),

		TEST_BA(yaml_root_from_ipc_request__no_op),
		TEST_BA(yaml_root_from_ipc_request__cfg_set),

		TEST_BA(yaml_root_from_ipc_operation__map),
		TEST_BA(yaml_root_from_ipc_operation__seq),

		TEST_BA(yaml_marshal__yaml_document_initialize_fail),
		TEST_BA(yaml_marshal__yaml_emitter_initialize_fail),
		TEST_BA(yaml_marshal__yaml_emitter_open_fail),
		TEST_BA(yaml_marshal__yaml_emitter_dump_fail),
		TEST_BA(yaml_marshal__yaml_emitter_close_fail),

		TEST_BA(yaml_write_handler__empty),
		TEST_BA(yaml_write_handler__append),
		TEST_BA(yaml_write_handler__no_data),
	};

	return RUN(tests);
}


#include "tst.h"

#include "assert-log.h"
#include "asserts.h"
#include "util-data.h"
#include "util-file.h"

#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "enum.h"
#include "fn.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "pslist.h"
#include "str.h"

#include "yaml/marshal-types.h"
#include "yaml/marshal.h"

static int after_each(void **state) {
	assert_logs_empty();

	cfg_free(g_cfg);
	g_cfg = NULL;
	free(g_lid);
	g_lid = NULL;
	return 0;
}

static void _check_marshalled(char *actual, const char *expected_path, const char * const file, const int line) {
	_assert_non_nul(actual, "actual", file, line);

	char *expected = read_file(expected_path);

	if (strcmp(actual, expected) != 0) {
		const char *err = sprintf_alloc("check_marshalled\nactual.yaml:\n%s !=\nexpected.yaml:\n%s\n", actual, expected);
		write_file("actual.yaml", actual);
		write_file("expected.yaml", expected);
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

}

static void yaml_root_from_cfg__default(void **state) {
	struct Cfg *cfg = cfg_default();

	check_marshalled(yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg"), "tst/yaml/cfg-default.yaml");

	cfg_free(cfg);

}

static void yaml_root_from_cfg__empty(void **state) {
	struct Cfg *cfg = cfg_init();

	check_marshalled(yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg"), "tst/yaml/empty.yaml");

	cfg_free(cfg);
}


static void yaml_root_from_ipc_request__no_op(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));

	assert_nul(yaml_marshal(ipc_request, (fn_yaml_root_from_type)yaml_root_from_ipc_request, "ipc request"));

	assert_log(ERROR, "unable to marshal ipc request: missing OP\n");

	ipc_request_free(ipc_request);
}

static void yaml_root_from_ipc_request__cfg_set(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));
	ipc_request->command = CFG_SET;
	ipc_request->log_threshold = ERROR;

	ipc_request->cfg = cfg_all();

	check_marshalled(yaml_marshal(ipc_request, (fn_yaml_root_from_type)yaml_root_from_ipc_request, "ipc request"), "tst/yaml/ipc-request-cfg-set.yaml");

	ipc_request_free(ipc_request);
}

static void yaml_root_from_ipc_operation__map(void **state) {
	struct IpcOperation *ipc_operation = ipc_response();

	log_cap_line_append(ERROR, "err", &ipc_operation->log_cap_lines);
	log_cap_line_append(FATAL, "fat", &ipc_operation->log_cap_lines);
	ipc_operation_update_rc(ipc_operation);

	check_marshalled(yaml_marshal(ipc_operation, (fn_yaml_root_from_type)yaml_root_from_ipc_operation, "ipc response"), "tst/yaml/ipc-responses-map.yaml");

	ipc_operation_free(ipc_operation);

	pslist_free_vals(&g_heads, (fn_free)head_free);
}

static void yaml_root_from_ipc_operation__seq(void **state) {
	struct IpcOperation *ipc_operation = ipc_response();
	ipc_operation->request->command = LIST;

	check_marshalled(yaml_marshal(ipc_operation, (fn_yaml_root_from_type)yaml_root_from_ipc_operation, "ipc response"), "tst/yaml/ipc-responses-seq.yaml");

	ipc_operation_free(ipc_operation);

	pslist_free_vals(&g_heads, (fn_free)head_free);
}

static void yaml_marshal__yaml_document_initialize_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	will_return_int(__wrap_yaml_document_initialize, 0);

	const char *actual = yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_document_initialize failed\n");
}

static void yaml_marshal__yaml_emitter_initialize_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	will_return_int(__wrap_yaml_emitter_initialize, 0);

	const char *actual = yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_initialize failed\n");
}

static void yaml_marshal__yaml_emitter_open_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	will_return_int(__wrap_yaml_emitter_open, 0);

	const char *actual = yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_open failed\n");
}

// also covers case of write_handler fail
static void yaml_marshal__yaml_emitter_dump_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	will_return_int(__wrap_yaml_emitter_dump, 0);

	const char *actual = yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_dump failed\n");
}

static void yaml_marshal__yaml_emitter_close_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	will_return_int(__wrap_yaml_emitter_close, 0);

	const char *actual = yaml_marshal(cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(WARNING, "unable to marshal cfg: yaml_emitter_close failed\n");
}

static void yaml_write_handler__empty(void **state) {
	char *data = NULL;

	char *buffer = strdup("1234");
	size_t size = 2;

	assert_int_equal(yaml_write_handler(&data, (unsigned char *)buffer, size), 1);

	assert_str_equal(data, "12");

	free(buffer);
	free(data);
}

static void yaml_write_handler__append(void **state) {
	char *data = strdup("foo");

	char *buffer = strdup("1234");
	size_t size = 2;

	assert_int_equal(yaml_write_handler(&data, (unsigned char *)buffer, size), 1);

	assert_str_equal(data, "foo12");

	free(buffer);
	free(data);
}

static void yaml_write_handler__no_data(void **state) {
	char *buffer = strdup("1234");
	size_t size = 2;

	assert_int_equal(yaml_write_handler(NULL, (unsigned char *)buffer, size), 0);

	free(buffer);
}

int main(void) {

	const struct CMUnitTest tests[] = {
		TEST_A(yaml_root_from_cfg__ok),
		TEST_A(yaml_root_from_cfg__default),
		TEST_A(yaml_root_from_cfg__empty),

		TEST_A(yaml_root_from_ipc_request__no_op),
		TEST_A(yaml_root_from_ipc_request__cfg_set),

		TEST_A(yaml_root_from_ipc_operation__map),
		TEST_A(yaml_root_from_ipc_operation__seq),

		TEST_A(yaml_marshal__yaml_document_initialize_fail),
		TEST_A(yaml_marshal__yaml_emitter_initialize_fail),
		TEST_A(yaml_marshal__yaml_emitter_open_fail),
		TEST_A(yaml_marshal__yaml_emitter_dump_fail),
		TEST_A(yaml_marshal__yaml_emitter_close_fail),

		TEST_A(yaml_write_handler__empty),
		TEST_A(yaml_write_handler__append),
		TEST_A(yaml_write_handler__no_data),
	};

	return RUN(tests);
}


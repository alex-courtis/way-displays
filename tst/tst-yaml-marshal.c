#include "tst.h"

#include "asserts-log.h"
#include "asserts.h"
#include "util-file.h"
#include "wrap-libyaml.h"
#include "wrap-log.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "slist.h"

#include "yaml/marshal-types.h"
#include "yaml/marshal.h"

#include "yaml/data.c"


int write_handler(void *data, unsigned char *buffer, size_t size);


static int before_all(void **state) {
	return 0;
}

static int after_all(void **state) {
	return 0;
}

static int before_each(void **state) {
	logs_clear();

	reset_yaml_fails();

	return 0;
}

static int after_each(void **state) {
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
		write_file("actual.yaml", actual);
		write_file("expected.yaml", expected);
	}

	_assert_str_equal(actual, "actual", expected, "expected", file, line);

	free(actual);
	free(expected);
}
#define check_marshalled(actual, expected_path) _check_marshalled(actual, expected_path, __FILE__, __LINE__)

static void yaml_doc_cfg__ok(void **state) {
	struct Cfg *cfg = cfg_all();

	check_marshalled(yaml_marshal(cfg, yaml_doc_cfg, "cfg"), "tst/yaml/cfg-all.yaml");

	cfg_free(cfg);

	assert_logs_empty();
}

static void yaml_doc_cfg__default(void **state) {
	struct Cfg *cfg = cfg_default();

	check_marshalled(yaml_marshal(cfg, yaml_doc_cfg, "cfg"), "tst/yaml/cfg-default.yaml");

	cfg_free(cfg);

	assert_logs_empty();
}

static void yaml_doc_cfg__empty(void **state) {
	struct Cfg *cfg = cfg_init();

	check_marshalled(yaml_marshal(cfg, yaml_doc_cfg, "cfg"), "tst/yaml/empty.yaml");

	cfg_free(cfg);

	assert_logs_empty();
}


static void yaml_doc_ipc_request__no_op(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));

	assert_nul(yaml_marshal(ipc_request, yaml_doc_ipc_request, "ipc request"));

	assert_log(ERROR, "unable to marshal ipc request: missing OP\n");

	assert_logs_empty();

	ipc_request_free(ipc_request);
}

static void yaml_doc_ipc_request__cfg_set(void **state) {
	struct IpcRequest *ipc_request = calloc(1, sizeof(struct IpcRequest));
	ipc_request->command = CFG_SET;
	ipc_request->log_threshold = ERROR;

	ipc_request->cfg = cfg_all();

	check_marshalled(yaml_marshal(ipc_request, yaml_doc_ipc_request, "ipc request"), "tst/yaml/ipc-request-cfg-set.yaml");

	ipc_request_free(ipc_request);

	assert_logs_empty();
}

static void yaml_doc_ipc_operation__map(void **state) {
	struct IpcOperation *ipc_operation = ipc_response();

	check_marshalled(yaml_marshal(ipc_operation, yaml_doc_ipc_operation, "ipc response"), "tst/yaml/ipc-responses-map.yaml");

	ipc_operation_free(ipc_operation);

	slist_free_vals(&g_heads, head_free);

	assert_logs_empty();
}

static void yaml_doc_ipc_operation__seq(void **state) {
	struct IpcOperation *ipc_operation = ipc_response();
	ipc_operation->request->command = LIST;

	check_marshalled(yaml_marshal(ipc_operation, yaml_doc_ipc_operation, "ipc response"), "tst/yaml/ipc-responses-seq.yaml");

	ipc_operation_free(ipc_operation);

	slist_free_vals(&g_heads, head_free);

	assert_logs_empty();
}

static void yaml_marshal__yaml_document_initialize_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_document_initialize__fail = true;

	const char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_document_initialize failed\n");
	assert_logs_empty();
}

static void yaml_marshal__yaml_emitter_initialize_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_emitter_initialize__fail = true;

	const char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_initialize failed\n");
	assert_logs_empty();
}

static void yaml_marshal__yaml_emitter_open_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_emitter_open__fail = true;

	const char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_open failed\n");
	assert_logs_empty();
}

// also covers case of write_handler fail
static void yaml_marshal__yaml_emitter_dump_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_emitter_dump__fail = true;

	const char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(ERROR, "unable to marshal cfg: yaml_emitter_dump failed\n");
	assert_logs_empty();
}

static void yaml_marshal__yaml_emitter_close_fail(void **state) {

	struct Cfg *cfg = cfg_all();

	yaml_emitter_close__fail = true;

	const char *actual = yaml_marshal(cfg, yaml_doc_cfg, "cfg");

	assert_nul(actual);

	cfg_free(cfg);

	assert_log(WARNING, "unable to marshal cfg: yaml_emitter_close failed\n");
	assert_logs_empty();
}

static void write_handler__empty(void **state) {
	char *data = NULL;

	char *buffer = strdup("1234");
	size_t size = 2;

	write_handler(&data, (unsigned char *)buffer, size);

	assert_str_equal(data, "12");

	free(buffer);
	free(data);

	assert_logs_empty();
}

static void write_handler__append(void **state) {
	char *data = strdup("foo");

	char *buffer = strdup("1234");
	size_t size = 2;

	write_handler(&data, (unsigned char *)buffer, size);

	assert_str_equal(data, "foo12");

	free(buffer);
	free(data);

	assert_logs_empty();
}

int main(void) {

	const struct CMUnitTest tests[] = {
		TEST(yaml_doc_cfg__ok),
		TEST(yaml_doc_cfg__default),
		TEST(yaml_doc_cfg__empty),

		TEST(yaml_doc_ipc_request__no_op),
		TEST(yaml_doc_ipc_request__cfg_set),

		TEST(yaml_doc_ipc_operation__map),
		TEST(yaml_doc_ipc_operation__seq),

		TEST(yaml_marshal__yaml_document_initialize_fail),
		TEST(yaml_marshal__yaml_emitter_initialize_fail),
		TEST(yaml_marshal__yaml_emitter_open_fail),
		TEST(yaml_marshal__yaml_emitter_dump_fail),
		TEST(yaml_marshal__yaml_emitter_close_fail),

		TEST(write_handler__empty),
		TEST(write_handler__append),
	};

	return RUN(tests);
}


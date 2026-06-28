#ifndef ASSERT_CFG_H
#define ASSERT_CFG_H

#include <cmocka.h>
#include <stdlib.h>

#include "cfg.h"
#include "util-file.h"
#include "yaml/marshal-types.h"
#include "yaml/marshal.h"

void _assert_cfg_equal(const struct Cfg *a, const struct Cfg *b, const char * const file, const int line) {
	if (!cfg_equal(a, b)) {
		char *yaml_a = yaml_marshal(a, (fn_yaml_type_to_root)yaml_cfg_to_root, "cfg a");
		char *yaml_b = yaml_marshal(b, (fn_yaml_type_to_root)yaml_cfg_to_root, "cfg b");
		cmocka_print_error("assert_cfg_equal\nactual.cfg:\n%s\nexpected.cfg:\n%s\n", yaml_a, yaml_b);
		write_file("actual.cfg", yaml_a);
		write_file("expected.cfg", yaml_b);
		free(yaml_a);
		free(yaml_b);
		_fail(file, line);
	}
}

#define assert_cfg_equal(a, b) _assert_cfg_equal(a, b, __FILE__, __LINE__)

#endif // ASSERT_CFG_H


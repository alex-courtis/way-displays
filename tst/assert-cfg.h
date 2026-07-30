#ifndef ASSERT_CFG_H
#define ASSERT_CFG_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cfg/cfg.h"
#include "fs.h"
#include "str.h"
#include "yaml/marshal-types-cfg.h"
#include "yaml/marshal.h"

void _assert_cfg(const struct Cfg *a, const struct Cfg *b, bool equal, const char * const name, const char * const file, const int line) {
	if (equal ? !cfg_equal(a, b) : cfg_equal(a, b)) {
		char *yaml_a = yaml_marshal(a, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg a");
		char *yaml_b = yaml_marshal(b, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg b");
		const char *err = sprintf_alloc("%s\nactual.cfg:\n%s %s\nexpected.cfg:\n%s\n", name, yaml_a, equal ? "!=" : "==", yaml_b);
		fs_file_write("actual.cfg", yaml_a, "w");
		fs_file_write("expected.cfg", yaml_b, "w");
		fprintf(stderr, "%s:%d: %s", file, line, err);
		exit(1);
	}
}

#define assert_cfg_equal(a, b) _assert_cfg(a, b, true, "assert_cfg_equal", __FILE__, __LINE__)

#define assert_cfg_not_equal(a, b) _assert_cfg(a, b, false, "assert_cfg_equal", __FILE__, __LINE__)

#endif // ASSERT_CFG_H


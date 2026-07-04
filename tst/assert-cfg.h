#ifndef ASSERT_CFG_H
#define ASSERT_CFG_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cfg.h"
#include "str.h"
#include "util-file.h"
#include "yaml/marshal-types.h"
#include "yaml/marshal.h"

void _assert_cfg(const struct Cfg *a, const struct Cfg *b, bool equal, const char * const name, const char * const file, const int line) {
	if (equal ? !cfg_equal(a, b) : cfg_equal(a, b)) {
		char *yaml_a = yaml_marshal(a, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg a");
		char *yaml_b = yaml_marshal(b, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg b");
		const char *err = sprintf_alloc("%s\nactual.cfg:\n%s %s\nexpected.cfg:\n%s\n", name, yaml_a, equal ? "!=" : "==", yaml_b);
		write_file("actual.cfg", yaml_a);
		write_file("expected.cfg", yaml_b);
		fprintf(stderr, "%s:%d: %s", file, line, err);
		exit(1);
	}
}

#define assert_cfg_equal(a, b) _assert_cfg(a, b, true, "assert_cfg_equal", __FILE__, __LINE__)

#define assert_cfg_not_equal(a, b) _assert_cfg(a, b, false, "assert_cfg_equal", __FILE__, __LINE__)

#endif // ASSERT_CFG_H


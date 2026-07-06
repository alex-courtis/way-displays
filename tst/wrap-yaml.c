#include <cmocka.h>

#include "yaml/marshal.h"
#include "yaml/unmarshal.h"

char *__wrap_yaml_marshal(const void *data, fn_yaml_root_from_type fn, const char *human) {
	check_expected_ptr(data);
	check_expected_ptr(human);

	return mock_ptr_type_checked(char*);
}

void *__wrap_yaml_unmarshal_file(const char *path, fn_yaml_root_to_type fn) {
	check_expected_ptr(path);

	return mock_ptr_type_checked(struct Cfg*);
}


#include "tst.h" // IWYU pragma: keep

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "lid.h"

void __wrap_lid_init(void) {
	function_called();
}

void __wrap_lid_update(void) {
	function_called();
}

bool __wrap_lid_is_closed(char *name) {
	check_expected(name);
	return mock_type(bool);
}

void __wrap_lid_destroy(void) {
	function_called();
}

void __wrap_lid_free(void *data) {
	function_called();

	if (!data)
		return;

	struct Lid *lid = data;

	free(lid->device_path);

	free(lid);
}


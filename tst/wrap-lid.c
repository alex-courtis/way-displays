#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "lid.h"

// complete replacement for lid

struct Lid *g_lid = NULL;

void __wrap_g_lid_init(void) {
	function_called();
}

void __wrap_g_lid_update(void) {
	function_called();
}

bool __wrap_g_lid_is_closed(char *name) {
	check_expected_ptr(name);
	return mock_type(bool);
}

void __wrap_g_lid_destroy(void) {
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


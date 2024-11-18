#include <stdbool.h>
#include <string.h>

#include "fn.h"

bool fn_comp_equals_strcmp(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strcmp(a, b) == 0;
}

bool fn_comp_equals_strstr(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strstr(a, b);
}

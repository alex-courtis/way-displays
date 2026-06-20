#include <stdbool.h>
#include <string.h>
#include <strings.h>

#include "fn.h"
#include "str.h"

bool fn_equal_ptr(const void* const a, const void* const b) {
	return a == b;
}

bool fn_equal_strcmp(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strcmp(a, b) == 0;
}

bool fn_equal_strcasecmp(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strcasecmp(a, b) == 0;
}

bool fn_equal_strstr(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strstr(a, b);
}

bool fn_less_than_strcmp(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strcmp(a, b) < 0;
}

bool fn_less_than_strcasecmp(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strcasecmp(a, b) < 0;
}

void *fn_clone_strdup(const void* const val) {
	if (val == NULL)
		return NULL;

	return strdup(val);
}

char *fn_str_or_null(const void* const val) {
	return sprintf_alloc("%s", val ? (char*)val : "(null)");
}


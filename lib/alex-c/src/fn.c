#include <stdbool.h>
#include <string.h>
#include <strings.h>

#include "fn.h"
#include "str.h"

bool equal_ptr(const void* const a, const void* const b) {
	return a == b;
}

bool equal_strcmp(const char* const a, const char* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strcmp(a, b) == 0;
}

bool equal_strcasecmp(const char* const a, const char* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strcasecmp(a, b) == 0;
}

bool equal_strstr(const char* const a, const char* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strstr(a, b);
}

bool less_than_strcmp(const char* const a, const char* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strcmp(a, b) < 0;
}

bool less_than_strcasecmp(const char* const a, const char* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strcasecmp(a, b) < 0;
}

void *clone_strdup(const void* const val) {
	if (val == NULL)
		return NULL;

	return strdup(val);
}

char *str_or_null(const char* const val) {
	return sprintf_alloc("%s", val ? val : "(null)");
}


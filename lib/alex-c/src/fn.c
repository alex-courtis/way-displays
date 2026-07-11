#include <stdbool.h>
#include <stdlib.h>
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

bool equal_stp(const size_t* const a, const size_t* const b) {
	if (!a || !b)
		return false;

	return *a == *b;
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

void *clone_strdup(const char* const str) {
	if (!str)
		return NULL;

	return strdup(str);
}

void *clone_size_t_ptr(const size_t* const np) {
	if (!np)
		return NULL;

	size_t *new = calloc(1, sizeof(size_t));
	*new = *np;

	return new;
}

char *str_or_null(const char* const str) {
	return sprintf_alloc("%s", str ? str : "(null)");
}

char *str_size_t_ptr(const size_t* const pi) {
	if (pi)
		return sprintf_alloc("%zu", *pi);
	else
		return strdup("(null)");
}


#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "str.h"

char *vsnprintf_alloc(size_t __maxlen, const char *__restrict __format, va_list __args) {

	va_list args;
	va_copy(args, __args);
	size_t raw = vsnprintf(NULL, 0, __format, args);
	va_end(args);

	size_t len = MIN(raw, __maxlen);

	char *str = calloc(len + 1, sizeof(char));

	va_copy(args, __args);
	vsnprintf(str, len + 1, __format, args);
	va_end(args);

	return str;
}

char *vsprintf_alloc(const char *__restrict __format, va_list __args) {
	return vsnprintf_alloc(SIZE_MAX, __format, __args);
}

char *sprintf_alloc(const char *__restrict __format, ...) {

	va_list args;
	va_start(args, __format);

	char *str = vsnprintf_alloc(SIZE_MAX, __format, args);

	va_end(args);

	return str;
}

char *snprintf_alloc(size_t __maxlen, const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);

	char *str = vsnprintf_alloc(__maxlen, __format, args);

	va_end(args);

	return str;
}

char *vsnprintf_append(char *__restrict s, size_t __maxlen, const char *__restrict __format, va_list __args) {

	size_t l_left = MIN(s ? strlen(s) : 0, __maxlen);
	size_t l_right = 0;

	if (l_left < __maxlen) {
		va_list args;
		va_copy(args, __args);
		l_right = vsnprintf(NULL, 0, __format, args);
		l_right = MIN(l_right, __maxlen - l_left);
		va_end(args);
	}

	char *str = calloc(l_left + l_right + 1, sizeof(char));

	char *right = stpncpy(str, s, l_left);

	if (l_right) {
		va_list args;
		va_copy(args, __args);
		vsnprintf(right, l_right + 1, __format, args);
		va_end(args);
	}

	if (s)
		free(s);

	return str;
}

char *vsprintf_append(char *__restrict s, const char *__restrict __format, va_list __args) {
	return vsnprintf_append(s, SIZE_MAX, __format, __args);
}

char *sprintf_append(char *__restrict s, const char *__restrict __format, ...) {

	va_list args;
	va_start(args, __format);

	char *str = vsnprintf_append(s, SIZE_MAX, __format, args);

	va_end(args);

	return str;
}

char *snprintf_append(char *__restrict s, size_t __maxlen, const char *__restrict __format, ...) {

	va_list args;
	va_start(args, __format);

	char *str = vsnprintf_append(s, __maxlen, __format, args);

	va_end(args);

	return str;
}

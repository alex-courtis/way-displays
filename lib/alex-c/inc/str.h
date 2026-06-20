#ifndef STR_H
#define STR_H

#include <stdarg.h>
#include <stddef.h>

// __args never mutated
// __maxlen is for the total returned string

// printf to a malloc'd buffer
char *sprintf_alloc(const char *__restrict __format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
char *snprintf_alloc(size_t __maxlen, const char *__restrict __format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
char *vsprintf_alloc(const char *__restrict __format, va_list __args);
char *vsnprintf_alloc(size_t __maxlen, const char *__restrict __format, va_list __args);

// append to nullable s, returning a malloc'd buffer, freeing s
char *sprintf_append(char *__restrict s, const char *__restrict __format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
char *snprintf_append(char *__restrict s, size_t __maxlen, const char *__restrict __format, ...) __attribute__ ((__format__ (__printf__, 3, 4)));
char *vsprintf_append(char *__restrict s, const char *__restrict __format, va_list __args);
char *vsnprintf_append(char *__restrict s, size_t __maxlen, const char *__restrict __format, va_list __args);

#endif // STR_H

#ifndef REGX_H
#define REGX_H

#include <stdbool.h>

// executes regcomp, regexec and regfree, fails silently on bad pattern
bool regex_matches(const char * const string, const char * const pattern);

// executes regcomp and return regerror on failure, user frees
char *regex_compiles(const char * const pattern);

#endif // REGX_H

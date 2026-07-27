#include <regex.h>
#include <stdbool.h>
#include <stdlib.h>

#include "regx.h"

bool regex_matches(const char * const string, const char * const pattern) {
	if (!string || !pattern)
		return false;

	regex_t regex;

	if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
		return false;

	bool match = regexec(&regex, string, 0, NULL, 0) == 0;

	regfree(&regex);

	return match;
}

char *regex_compiles(const char * const pattern) {
	if (!pattern)
		return NULL;

	char *err = NULL;

	regex_t regex;

	int result = regcomp(&regex, pattern, REG_EXTENDED);

	if (result) {
		size_t err_len = regerror(result, &regex, NULL, 0);

		err = calloc(err_len, sizeof(char));

		regerror(result, &regex, err, err_len);
	}

	regfree(&regex);

	return err;
}


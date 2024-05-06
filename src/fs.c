#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "log.h"

bool mkdir_p(char *path, mode_t mode) {
	bool rc = false;
	char *dir_path = NULL;

	if (!path) {
		goto end;
	}

	struct stat sb;
	if (stat(path, &sb) == 0) {
		rc = true;
		goto end;
	}

	dir_path = strdup(path);
	if (!mkdir_p(dirname(dir_path), mode)) {
		goto end;
	}

	if (mkdir(path, mode) != 0) {
		log_error_errno("\nCannot create directory %s", path);
		goto end;
	}

	rc = true;

end:
	free(dir_path);

	return rc;
}

bool file_write(const char *path, const char *contents, const char *mode) {
	if (!path) {
		return false;
	}

	FILE *f = fopen(path, mode);

	if (!f) {
		log_error_errno("\nUnable to write to %s", path);
		return false;
	}

	if (contents) {
		fprintf(f, "%s\n", contents);
	}

	fflush(f);

	if (fclose(f) != 0) {
		log_error_errno("\nUnable to write to %s", path);
		return false;
	}

	return true;
}


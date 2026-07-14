#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fs.h"

// TODO move to lib

bool fs_mkdir_p(char *path, mode_t mode) {
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
	if (!fs_mkdir_p(dirname(dir_path), mode)) {
		goto end;
	}

	if (mkdir(path, mode) != 0) {
		goto end;
	}

	rc = true;

end:
	free(dir_path);

	return rc;
}

bool fs_file_write(const char *path, const char *contents, const char *mode) {
	if (!path || !mode) {
		return false;
	}

	FILE *f = fopen(path, mode);

	if (!f) {
		return false;
	}

	if (contents) {
		fprintf(f, "%s\n", contents);
	}

	fflush(f);

	if (fclose(f) != 0) {
		return false;
	}

	return true;
}

char *fs_canonical_path(char *path) {
	if (!path)
		return NULL;

	char *real_path = realpath(path, NULL);

	if (!real_path)
		return NULL;

	if (access(real_path, R_OK) != 0) {
		free(real_path);
		return NULL;
	}

	return real_path;
}


#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cfg/file.h"
#include "log.h"
#include "pslist.h"

#include "fs.h"

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
		log_error(NULL);
		log_error_errno("Cannot create directory %s", path);
		goto end;
	}

	rc = true;

end:
	free(dir_path);

	return rc;
}

bool file_write(const char *path, const char *contents, const char *mode) {
	if (!path || !mode) {
		return false;
	}

	FILE *f = fopen(path, mode);

	if (!f) {
		log_error(NULL);
		log_error_errno("Unable to write to %s", path);
		return false;
	}

	if (contents) {
		fprintf(f, "%s\n", contents);
	}

	fflush(f);

	if (fclose(f) != 0) {
		log_error(NULL);
		log_error_errno("Unable to write to %s", path);
		return false;
	}

	return true;
}

char *canonical_path(char *path) {
	if (!path)
		return NULL;

	if (access(path, R_OK) != 0)
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

bool g_cfg_file_resolve(void) {
	if (!g_cfg_file)
		return false;

	g_cfg_file_init();

	for (struct Pslist *i = g_cfg_file_paths; i; i = i->nex) {
		if (access(i->val, R_OK) == 0) {

			char *file_path = realpath(i->val, NULL);

			if (!file_path) {
				continue;
			}
			if (access(file_path, R_OK) != 0) {
				free(file_path);
				continue;
			}

			set_paths(g_cfg_file, i->val, file_path);

			free(file_path);

			return true;
		}
	}

	return false;
}
bool g_cfg_file_resolve1(void) {
	if (!g_cfg_file)
		return false;

	g_cfg_file_init();

	for (struct Pslist *i = g_cfg_file_paths; i; i = i->nex) {
		char *path = canonical_path(i->val);
		if (path) {
			set_paths(g_cfg_file, i->val, path);
			free(path);
			return true;
		}
	}

	return false;
}


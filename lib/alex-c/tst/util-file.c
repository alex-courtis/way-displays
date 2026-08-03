#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "util-file.h"

#include "str.h"

char *read_file(const char *path) {
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr,"%s : %s\n", path, strerror(errno));
		exit(1);
	}

	int len = lseek(fd, 0, SEEK_END);

	char *out = calloc(len + 1, sizeof(char));

	if (len > 0) {
		memcpy(out, mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0), sizeof(char) * len);
	}

	close(fd);

	return out;
}

char *read_file_filter(const char *path, const char *starts_with) {
	char *line = NULL;
	size_t size = 0;

	FILE* file = fopen(path, "r");
	if (!file) {
		fprintf(stderr,"%s : %s\n", path, strerror(errno));
		exit(1);
	}

	char *out = strdup("");
	while (getline(&line, &size, file) != -1) {
		if (strstr(line, starts_with) != line) {
			out = sprintf_append(out, "%s", line);
		}
	}

	free(line);

	fclose(file);

	return out;
}


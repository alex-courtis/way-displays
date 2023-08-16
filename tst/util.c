#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stdio.h>

char *read_file(const char *path) {
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		char *out = calloc(PATH_MAX, sizeof(char));
		sprintf(out, "file not found: %s\n", path);
		return out;
	}

	int len = lseek(fd, 0, SEEK_END);

	if (len == 0) {
		return NULL;
	}

	char *out = calloc(len, sizeof(char));

	if (len > 0) {
		memcpy(out, mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0), sizeof(char) * len - 1);
	}

	close(fd);

	return out;
}

void write_file(const char *path, const char *content) {
	FILE *f = fopen(path, "w");
	if (!f) {
		fprintf(stderr, "could not write to %s\n", path);
		exit(1);
	}

	fprintf(f, "%s", content);

	fclose(f);
}


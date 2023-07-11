#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

char *read_file(const char *path) {
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		char *out = calloc(PATH_MAX + 64, sizeof(char));
		snprintf(out, PATH_MAX + 64, "file not found: %s\n", path);
		return out;
	}

	int len = lseek(fd, 0, SEEK_END);

	char *out = calloc(len, sizeof(char));

	memcpy(out, mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0), sizeof(char) * len - 1);

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


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "util-file.h"

char *read_file(const char *path) {
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr,"file not found: %s\n", path);
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


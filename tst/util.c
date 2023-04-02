#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>


#include <stdio.h>

char *read_file(const char *path) {
	int fd = open(path, O_RDONLY);
	int len = lseek(fd, 0, SEEK_END);

	char *out = calloc(len, sizeof(char));

	memcpy(out, mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0), sizeof(char) * len - 1);

	close(fd);

	return out;
}

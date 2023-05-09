#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <sys/stat.h>

bool mkdir_p(char *path, mode_t mode);

bool file_write(const char *path, const char *contents);

#endif // FS_H


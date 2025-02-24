#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <sys/types.h>

bool mkdir_p(char *path, mode_t mode);

bool file_write(const char *path, const char *contents, const char *mode);

#endif // FS_H


#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <sys/types.h>

// mkdir -p for path with mode applied to all created
bool fs_mkdir_p(char *path, mode_t mode);

// write a file to path with mode
bool fs_file_write(const char *path, const char *contents, const char *mode);

// return realpath if it exists and is readable, following links, user frees
char *fs_canonical_path(char *path);

#endif // FS_H


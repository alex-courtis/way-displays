#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <sys/types.h>

bool mkdir_p(char *path, mode_t mode);

bool file_write(const char *path, const char *contents, const char *mode);

// return real path (delink) if it exists and is readable, user frees
char *resolve_canonical_path(char *path);

#endif // FS_H


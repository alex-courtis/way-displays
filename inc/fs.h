#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <sys/types.h>

bool mkdir_p(char *path, mode_t mode);

bool file_write(const char *path, const char *contents, const char *mode);

// TODO move CfgFile bits into file.c
// if a file is found in g_cfg_file_paths, return true and set them
bool g_cfg_file_resolve(void);
bool g_cfg_file_resolve1(void);

// return real path (delink) if it exists and is readable, user frees
char *canonical_path(char *path);

#endif // FS_H


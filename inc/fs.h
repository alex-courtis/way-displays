#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <sys/types.h>

bool mkdir_p(char *path, mode_t mode);

bool file_write(const char *path, const char *contents, const char *mode);

// TODO move CfgFile bits into file.c
// if a file is found in g_cfg_file_paths, return true and set them
bool g_cfg_file_resolve(void);


#endif // FS_H


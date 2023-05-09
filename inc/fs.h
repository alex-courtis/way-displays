#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <sys/stat.h>

bool mkdir_p(char *path, mode_t mode);

#endif // FS_H


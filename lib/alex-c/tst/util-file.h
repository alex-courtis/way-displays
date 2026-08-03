#ifndef UTIL_FILE_H
#define UTIL_FILE_H

char *read_file(const char *path);

char *read_file_filter(const char *path, const char *starts_with);

#endif // UTIL_FILE_H

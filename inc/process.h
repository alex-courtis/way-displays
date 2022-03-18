#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

__pid_t pid_active_server(void);

void pid_file_create(void);

void exit_fail(void);

#endif // PROCESS_H


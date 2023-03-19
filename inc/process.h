#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

char *pid_path(void);

pid_t pid_active_server(void);

void pid_file_create(void);

// exit; caller should return afterwards
void wd_exit(int __status);

// exit, logging standard message to error; caller should return after
void wd_exit_message(int __status);

#endif // PROCESS_H


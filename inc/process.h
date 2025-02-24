#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

#include "stable.h"

char *pid_path(void);

pid_t pid_active_server(void);

void pid_file_create(void);

void spawn_sh_cmd(const char * const command, const struct STable * const env);

// exit no message
void wd_exit(const int __status);

// exit, logging fatal standard message
void wd_exit_message(const int __status);

#endif // PROCESS_H


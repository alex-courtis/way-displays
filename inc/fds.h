#ifndef FDS_H
#define FDS_H

#include "types.h"

extern int fd_signal;
extern int fd_cfg_dir;

extern int npfds;
extern struct pollfd *pfds;

extern struct pollfd *pfd_signal;
extern struct pollfd *pfd_wayland;
extern struct pollfd *pfd_lid;
extern struct pollfd *pfd_cfg_dir;

void create_pfds(struct Displ *displ);

void destroy_pfds();

bool cfg_file_written(char *file_name);

#endif // FDS_H


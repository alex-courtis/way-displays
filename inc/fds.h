#ifndef FDS_H
#define FDS_H

#include <poll.h>
#include <stdbool.h>

#include "cfg.h"
#include "types.h"

extern int fd_signal;
extern int fd_ipc;
extern int fd_cfg_dir;

extern nfds_t npfds;
extern struct pollfd pfds[5];

extern struct pollfd *pfd_signal;
extern struct pollfd *pfd_ipc;
extern struct pollfd *pfd_wayland;
extern struct pollfd *pfd_lid;
extern struct pollfd *pfd_cfg_dir;

void init_fds(struct Cfg *cfg);

void create_pfds(struct Displ *displ);

void destroy_pfds();

bool cfg_file_modified(char *file_name);

#endif // FDS_H


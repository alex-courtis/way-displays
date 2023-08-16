#ifndef FDS_H
#define FDS_H

#include <poll.h>
#include <stdbool.h>

extern int fd_signal;
extern int fd_socket_server;
extern int fd_cfg_dir;
extern int wd_cfg_dir;

extern nfds_t npfds;
extern struct pollfd pfds[5];

extern struct pollfd *pfd_signal;
extern struct pollfd *pfd_ipc;
extern struct pollfd *pfd_wayland;
extern struct pollfd *pfd_lid;
extern struct pollfd *pfd_cfg_dir;

void pfds_init(void);

void pfds_destroy(void);

void fd_wd_cfg_dir_create(void);

void fd_wd_cfg_dir_destroy(void);

bool fd_cfg_dir_modified(char *file_name);

#endif // FDS_H


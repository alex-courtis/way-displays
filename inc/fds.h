#ifndef FDS_H
#define FDS_H

#include "types.h"

extern int fd_signal;

extern int npfds;
extern struct pollfd *pfds;

extern struct pollfd *pfd_signal;
extern struct pollfd *pfd_wayland;
extern struct pollfd *pfd_lid;

void create_pfds(struct Displ *displ);

void destroy_pfds();

#endif // FDS_H


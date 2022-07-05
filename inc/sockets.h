#ifndef SOCKETS_H
#define SOCKETS_H

#include <sys/types.h>
#include <sys/un.h>

void socket_path(struct sockaddr_un *addr);

int create_fd_ipc_server(void);

int create_fd_ipc_client(void);

int socket_accept(int fd_sock);

char *socket_read(int fd);

ssize_t socket_write(int fd, char *data, size_t len);

#endif // SOCKETS_H


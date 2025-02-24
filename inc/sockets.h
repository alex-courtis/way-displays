#ifndef SOCKETS_H
#define SOCKETS_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/un.h>

void socket_path(struct sockaddr_un *addr);

int create_socket_server(void);

int create_socket_client(void);

int socket_accept(int socket_server);

char *socket_read(int socket_client);

ssize_t socket_write(int socket_client, char *data, size_t len);

#endif // SOCKETS_H


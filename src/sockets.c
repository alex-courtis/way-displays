#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "log.h"

#define SERVER_TIMEOUT_SEC 2
#define CLIENT_TIMEOUT_SEC 10

bool set_socket_timeout(int socket, struct timeval timeout) {

	if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		log_error_errno("\nSocket set timeout failed");
		return false;
	}

	return true;
}

int socket_accept(int socket_server) {

	int socket_client = accept(socket_server, NULL, NULL);

	if (socket_client == -1) {
		log_error_errno("\nSocket accept failed");
		return -1;
	}

	struct timeval timeout = { .tv_sec = SERVER_TIMEOUT_SEC, .tv_usec = 0, };
	if (!set_socket_timeout(socket_client, timeout)) {
		return -1;
	}

	return socket_client;
}

char *socket_read(int socket_client) {

	// peek, as the sender may experience delay between connecting and sending
	if (recv(socket_client, NULL, 0, MSG_PEEK) == -1) {
		if (errno == EAGAIN) {
			log_error("\nSocket read timeout");
		} else {
			log_error_errno("\nSocket recv failed");
		}
		return NULL;
	}

	// total message size right now; further data will be disregarded
	int n = 0;
	if (ioctl(socket_client, FIONREAD, &n) == -1) {
		log_error_errno("\nServer FIONREAD failed");
		return NULL;
	}
	if (n == 0) {
		return NULL;
	}

	// read it
	char *buf = calloc(n + 1, sizeof(char));
	if (recv(socket_client, buf, n, 0) == -1) {
		log_error_errno("\nSocket recv failed");
		return NULL;
	}

	return buf;
}

ssize_t socket_write(int socket_client, char *data, size_t len) {
	ssize_t n;
	if ((n = write(socket_client, data, len)) == -1) {
		log_error_errno("\nSocket write failed");
		return -1;
	}

	return n;
}

void socket_path(struct sockaddr_un *addr) {
	size_t sun_path_size = sizeof(addr->sun_path);

	char name[sun_path_size - 4];
	if (getenv("XDG_VTNR")) {
		snprintf(name, sizeof(name), "/way-displays.%s.sock", getenv("XDG_VTNR"));
	} else {
		snprintf(name, sizeof(name), "/way-displays.sock");
	}

	if (!getenv("XDG_RUNTIME_DIR") || strlen(name) + strlen(getenv("XDG_RUNTIME_DIR")) > sun_path_size) {
		snprintf(addr->sun_path, sun_path_size, "/tmp%s", name);
	} else {
		snprintf(addr->sun_path, sun_path_size, "%s%s", getenv("XDG_RUNTIME_DIR"), name);
	}
}

int create_socket_server(void) {
	int socket_server = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_server == -1) {
		log_error_errno("\nServer socket failed, clients unavailable");
		return -1;
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	socket_path(&addr);
	unlink(addr.sun_path);

	if (bind(socket_server, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		log_error_errno("\nServer socket bind failed, clients unavailable");
		close(socket_server);
		return -1;
	}

	if (listen(socket_server, 3) < 0) {
		log_error_errno("\nServer socket listen failed, clients unavailable");
		close(socket_server);
		return -1;
	}

	struct timeval timeout = { .tv_sec = SERVER_TIMEOUT_SEC, .tv_usec = 0, };
	if (!set_socket_timeout(socket_server, timeout)) {
		close(socket_server);
		return -1;
	}

	return socket_server;
}

int create_socket_client(void) {
	int socket_client = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_client == -1) {
		log_error_errno("\nSocket create failed");
		return -1;
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	socket_path(&addr);

	if (connect(socket_client, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		log_error_errno("\nSocket connect failed");
		close(socket_client);
		return -1;
	}

	struct timeval timeout = { .tv_sec = CLIENT_TIMEOUT_SEC, .tv_usec = 0, };
	if (!set_socket_timeout(socket_client, timeout)) {
		close(socket_client);
		return -1;
	}

	return socket_client;
}


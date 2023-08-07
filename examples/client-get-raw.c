#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sockets.h"

const char *request_yaml = "OP: GET";

/*
 * Execute a GET and print the raw output
 */
int
main(int argc, char **argv) {

	// open a socket
	int fd;
	if ((fd = create_socket_client()) == -1) {
		return 1;
	}

	fprintf(stdout, "========request=================\n%s\n", request_yaml);

	// write a GET request to the socket
	if (write(fd, request_yaml, strlen(request_yaml)) == -1) {
		fprintf(stderr, "socket write failed: %d: %s\n", errno, strerror(errno));
		return 1;
	}

	// listen to all responses
	for (;;) {
		char *response = NULL;
		if (!(response = socket_read(fd))) {
			return 1;
		}

		fprintf(stdout, "========response================\n%s", response);

		bool done = strstr(response, "DONE: TRUE");
		free(response);

		if (done) {
			break;
		}
	}

	return 0;
}


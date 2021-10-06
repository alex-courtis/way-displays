#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include "process.h"

#define PID_FN "/tmp/way-displays.pid"
#define NPBUF 11

char pbuf[NPBUF] = "";

void ensure_singleton() {

	int fd = open(PID_FN, O_RDWR | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		fprintf(stderr, "ERROR: unable to open pid file '%s' %d: %s, exiting\n", PID_FN, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
		if (errno != EWOULDBLOCK) {
			fprintf(stderr, "ERROR: unable to lock pid file '%s' %d: %s, exiting\n", PID_FN, errno, strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (read(fd, pbuf, NPBUF) == -1) {
			fprintf(stderr, "ERROR: unable to read pid file '%s' %d: %s, exiting\n", PID_FN, errno, strerror(errno));
			exit(EXIT_FAILURE);
		}

		fprintf(stderr, "ERROR: another instance %s is running, exiting\n", pbuf);
		exit(EXIT_FAILURE);
	}

	if (ftruncate(fd, 0) == -1) {
		fprintf(stderr, "ERROR: unable to truncate pid file '%s' %d: %s, exiting\n", PID_FN, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	snprintf(pbuf, NPBUF, "%d", getpid());
	if (write(fd, pbuf, NPBUF) == -1) {
		fprintf(stderr, "ERROR: unable to write pid file '%s' %d: %s, exiting\n", PID_FN, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "process.h"

#define PID_FN "/tmp/way-displays.pid"
#define NPBUF 11

char pbuf[NPBUF] = "";

void ensure_singleton() {

	// attempt to use existing, regardless of owner
	int fd = open(PID_FN, O_RDWR | O_CLOEXEC);
	if (fd == -1 && errno != ENOENT) {
		fprintf(stderr, "ERROR: unable to open existing pid file for writing '%s' %d: %s, exiting\n", PID_FN, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	// create a new file
	if (fd == -1) {
		mode_t umask_prev = umask(0000);
		fd = open(PID_FN, O_RDWR | O_CLOEXEC | O_CREAT, 0666);
		umask(umask_prev);
		if (fd == -1) {
			fprintf(stderr, "ERROR: unable to create pid file '%s' %d: %s, exiting\n", PID_FN, errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	// lock it forever
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

	// write the new pid
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


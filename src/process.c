#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#define PID_FN "/tmp/way-layout-displays.pid"

void ensure_singleton() {
	const int NBUF = 11;
	char buf[NBUF];

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

		buf[0] = '\0';
		read(fd, buf, NBUF);
		fprintf(stderr, "ERROR: another instance %s is running, exiting\n", buf);
		exit(EXIT_FAILURE);
	}

	if (ftruncate(fd, 0) == -1) {
		fprintf(stderr, "ERROR: unable to truncate pid file '%s' %d: %s, exiting\n", PID_FN, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	snprintf(buf, NBUF, "%d", getpid());
	if (write(fd, buf, NBUF) == -1) {
		fprintf(stderr, "ERROR: unable to write to pid file '%s' %d: %s, exiting\n", PID_FN, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}


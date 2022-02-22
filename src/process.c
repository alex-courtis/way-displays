#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "process.h"

#include "log.h"

char *pid_path() {
	char *path = calloc(1, PATH_MAX);

	const char *xdg_vtnr = getenv("XDG_VTNR");
	if (xdg_vtnr) {
		snprintf(path, PATH_MAX, "/tmp/way-displays.%s.pid", xdg_vtnr);
	} else {
		snprintf(path, PATH_MAX, "/tmp/way-displays.pid");
	}

	return path;
}

__pid_t pid_active_server() {
	static char pbuf[11];

	__pid_t pid = 0;

	char *path = pid_path();

	int fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd != -1) {
		if (read(fd, pbuf, sizeof(pbuf)) != -1) {
			if (sscanf(pbuf, "%d", &pid) != 1) {
				pid = 0;
			} else if (kill(pid, 0) == -1) {
				pid = 0;
			}
		}
		close(fd);
	}

	free(path);

	return pid;
}

void pid_file_create() {
	char *path = pid_path();

	__pid_t pid = pid_active_server();
	if (pid_active_server()) {
		log_error("\nanother instance %d is running, exiting", pid);
		exit(EXIT_FAILURE);
	}

	// attempt to use existing, regardless of owner
	int fd = open(path, O_RDWR | O_CLOEXEC);
	if (fd == -1 && errno != ENOENT) {
		log_error_errno("\nunable to open existing pid file for writing %s, exiting", path);
		exit(EXIT_FAILURE);
	}

	// create a new file
	if (fd == -1) {
		mode_t umask_prev = umask(0000);
		fd = open(path, O_RDWR | O_CLOEXEC | O_CREAT, 0666);
		umask(umask_prev);
		if (fd == -1) {
			log_error_errno("\nunable to create pid file %s, exiting", path);
			exit(EXIT_FAILURE);
		}
	}

	// lock it forever
	if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
		log_error_errno("\nunable to lock pid file %s, exiting", path);
		exit(EXIT_FAILURE);
	}

	// clear it
	if (ftruncate(fd, 0) == -1) {
		log_error_errno("\nunable to truncate pid file %s, exiting", path);
		exit(EXIT_FAILURE);
	}

	// write the new pid
	if (dprintf(fd, "%d", getpid()) <= 0) {
		log_error_errno("\nunable to write to pid file %s, exiting", path);
		exit(EXIT_FAILURE);
	}

	free(path);
}


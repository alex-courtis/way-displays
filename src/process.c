#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "process.h"

#include "log.h"

char *pid_path(void) {
	char *path = calloc(1, PATH_MAX);

	const char *xdg_vtnr = getenv("XDG_VTNR");
	if (xdg_vtnr) {
		snprintf(path, PATH_MAX, "/tmp/way-displays.%s.pid", xdg_vtnr);
	} else {
		snprintf(path, PATH_MAX, "/tmp/way-displays.pid");
	}

	return path;
}

pid_t pid_active_server(void) {
	static char pbuf[11];

	pid_t pid = 0;

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

void pid_file_create(void) {
	char *path = pid_path();

	pid_t pid = pid_active_server();
	if (pid_active_server()) {
		log_error("\nanother instance %d is running, exiting", pid);
		wd_exit(EXIT_FAILURE);
		return;
	}

	// attempt to use existing, regardless of owner
	int fd = open(path, O_RDWR | O_CLOEXEC);
	if (fd == -1 && errno != ENOENT) {
		log_error_errno("\nunable to open existing pid file for writing %s, exiting", path);
		wd_exit_message(EXIT_FAILURE);
		return;
	}

	// create a new file
	if (fd == -1) {
		mode_t umask_prev = umask(0000);
		fd = open(path, O_RDWR | O_CLOEXEC | O_CREAT, 0666);
		umask(umask_prev);
		if (fd == -1) {
			log_error_errno("\nunable to create pid file %s, exiting", path);
			wd_exit_message(EXIT_FAILURE);
			return;
		}
	}

	// lock it forever
	if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
		log_error_errno("\nunable to lock pid file %s, exiting", path);
		wd_exit_message(EXIT_FAILURE);
		return;
	}

	// clear it
	if (ftruncate(fd, 0) == -1) {
		log_error_errno("\nunable to truncate pid file %s, exiting", path);
		wd_exit_message(EXIT_FAILURE);
		return;
	}

	// write the new pid
	if (dprintf(fd, "%d", getpid()) <= 0) {
		log_error_errno("\nunable to write to pid file %s, exiting", path);
		wd_exit_message(EXIT_FAILURE);
		return;
	}

	free(path);
}

void spawn_sh_cmd(const char * const command, char * const message) {
	if (!command || !message)
		return;

	// experiments show that environment variable length tops out at 128k: variable itself plus contents
	if (strlen(message) > 1024 * 120) {
		message[1024 * 120] = '\0';
	}

	pid_t pid = fork();
	if (pid < 0) {
		log_error_errno("\nfailed to fork");
		return;
	}

	if (pid == 0) {
		struct sigaction sa;

		setsid();
		sigemptyset(&sa.sa_mask);
		// reset signals to the default
		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;
		sigaction(SIGCHLD, &sa, NULL);

		setenv("WD_MESSAGE", message, 1);

		// execute command in the child process
		execl("/bin/sh", "/bin/sh", "-c", command, (char *)NULL);
		log_error_errno("\nfailed to execute /bin/sh");
		// exit the child process in case the exec fails
		exit(-1);
	}
}

void wd_exit(int __status) {
	exit(__status);
}

void wd_exit_message(int __status) {
	log_error("\nPlease raise an issue: https://github.com/alex-courtis/way-displays/issues");
	log_error("Attach this log and describe the events that occurred before this failure.");
	exit(__status);
}


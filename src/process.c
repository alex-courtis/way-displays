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

#include "cli.h"
#include "process.h"
#include "smaps.h"

#include "log.h"

void pid_path_generate(char *pid_path) {
	const char *xdg_vtnr = getenv("XDG_VTNR");
	if (xdg_vtnr) {
		snprintf(pid_path, PATH_MAX - 1, "/tmp/way-displays.%s.pid", xdg_vtnr);
	} else {
		snprintf(pid_path, PATH_MAX - 1, "/tmp/way-displays.pid");
	}
}

pid_t pid_active_server(const char *pid_path) {
	pid_t pid = 0;

	int fd = open(pid_path, O_RDONLY | O_CLOEXEC);
	if (fd != -1) {
		static char pbuf[11];
		if (read(fd, pbuf, sizeof(pbuf)) != -1) {
			if (sscanf(pbuf, "%d", &pid) != 1) {
				pid = 0;
			} else if (kill(pid, 0) == -1) {
				pid = 0;
			}
		}
		close(fd);
	}

	return pid;
}

void pid_file_create(void) {
	char pid_path[PATH_MAX];
	pid_path_generate(pid_path);

	pid_t pid = pid_active_server(pid_path);
	if (pid) {
		log_fatal("another instance %d is running, exiting\n", pid);
		usage(stderr);
		wd_exit(EXIT_FAILURE);
		return;
	}

	// attempt to use existing, regardless of owner
	int fd = open(pid_path, O_RDWR | O_CLOEXEC);
	if (fd == -1 && errno != ENOENT) {
		log_fatal(NULL);
		log_fatal_errno("unable to open existing pid file for writing %s, exiting", pid_path);
		wd_exit_message(EXIT_FAILURE);
		return;
	}

	// create a new file
	if (fd == -1) {
		mode_t umask_prev = umask(0000);
		fd = open(pid_path, O_RDWR | O_CLOEXEC | O_CREAT, 0666);
		umask(umask_prev);
		if (fd == -1) {
			log_fatal(NULL);
			log_fatal_errno("unable to create pid file %s, exiting", pid_path);
			wd_exit_message(EXIT_FAILURE);
			return;
		}
	}

	// lock it forever
	if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
		log_fatal(NULL);
		log_fatal_errno("unable to lock pid file %s, exiting", pid_path);
		wd_exit_message(EXIT_FAILURE);
		return;
	}

	// clear it
	if (ftruncate(fd, 0) == -1) {
		log_fatal(NULL);
		log_fatal_errno("unable to truncate pid file %s, exiting", pid_path);
		wd_exit_message(EXIT_FAILURE);
		return;
	}

	// write the new pid
	if (dprintf(fd, "%d", getpid()) <= 0) {
		log_fatal(NULL);
		log_fatal_errno("unable to write to pid file %s, exiting", pid_path);
		wd_exit_message(EXIT_FAILURE);
		return;
	}
}

void spawn_sh_cmd(const char * const command, const struct SMapS * const env) {
	if (!command)
		return;

	pid_t pid = fork();
	if (pid < 0) {
		log_error(NULL);
		log_error_errno("failed to fork");
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

		for (const struct SMapSIter *i = smaps_iter(env); i; i = smaps_iter_next(i)) {

			// experiments show that environment variable length tops out at 128k: variable itself plus contents
			char value[1024 * 120];
			snprintf(value, sizeof(value), "%s", (char*)i->val);
			setenv(i->key, value, 1);
		}

		// execute command in the child process
		execl("/bin/sh", "/bin/sh", "-c", command, (char *)NULL);
		log_error(NULL);
		log_error_errno("failed to execute /bin/sh");
		// exit the child process in case the exec fails
		exit(-1);
	}
}

void wd_exit(const int __status) {
	exit(__status);
}

void wd_exit_message(const int __status) {
	log_fatal(NULL);
	log_fatal("Please raise an issue: https://github.com/alex-courtis/way-displays/issues");
	log_fatal("Attach this log and describe the events that occurred before this failure.");
	exit(__status);
}


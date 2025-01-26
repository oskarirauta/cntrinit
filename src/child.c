#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sched.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#include "config.h"
#include "logging.h"
#include "child.h"

extern char **environ;

static void clean_cmd_path(struct config* cfg) {

	if ( cfg -> argv[0] == NULL || strlen(cfg -> argv[0]) == 0 )
		return;

	char *path_dir = strdup(cfg -> argv[0]);
	char *path_exe = strdup(cfg -> argv[0]);
	char *dir = dirname(path_dir);
	char *exe = basename(path_exe);
	char new_path[PATH_MAX];

	if ( dir != NULL && exe != NULL && strlen(dir) > 0 && strlen(exe) > 0 ) {

		if ( dir[strlen(dir) - 1] != '/' && exe[strlen(exe) - 1] != '/' )
			sprintf(new_path, "%s/%s", dir, exe);
		else sprintf(new_path, "%s%s", dir, exe);

		free(cfg -> argv[0]);
		cfg -> argv[0] = strdup(new_path);
	}

	if ( realpath(cfg -> argv[0], new_path) != NULL ) {

		free(cfg -> argv[0]);
		cfg -> argv[0] = strdup(new_path);

	} else WARN("realpath failed, %s", strerror(errno));

	if ( path_dir )
		free(path_dir);
	if ( path_exe )
		free(path_exe);
}

int start_child(struct config* cfg) {

	if ( chdir("/") == -1 )
		WARN("chdir failed, %s", strerror(errno));

	if ( cfg -> op != CNTR )
		return 0;

	struct stat st;
	if ( !(stat(cfg -> argv[0], &st) == 0 && st.st_mode & S_IXUSR )) {

		char new_path[PATH_MAX];

		if ( cfg -> argv[0][0] == '/' )
			sprintf(new_path, "%s", cfg -> argv[0]);
		else sprintf(new_path, "%s/%s", cfg -> cwd, cfg -> argv[0]);

		if ( stat(new_path, &st) == 0 && st.st_mode & S_IXUSR ) {

			free(cfg -> argv[0]);
			cfg -> argv[0] = strdup(new_path);
			clean_cmd_path(cfg);
			DEBUG("child command fixed to %s", cfg -> argv[0]);

		} else WARN("command %s not found or not executable, child process will fail", cfg -> argv[0]);

	} else clean_cmd_path(cfg);

	pid_t child = syscall(__NR_clone, 0 | SIGCHLD, 0, 0, 0, 0);

	if ( child < 0 )
		FAIL2("failed to fork: %s", strerror(errno))
	else if ( child > 0 ) {

		if ( kill(child, 0) < 0 )
			FAIL2("failed to start %s [%d]", cfg -> argv[0], child);

		cfg -> child_pid = child;
		DEBUG("child process started with PID %d", child);

		write_cpidfile(cfg);

		return child;
	}

	if ( setpgid(0, 0) <  0 )
		FAIL2("setpgid failed, %s", strerror(errno));

	pid_t pgid = getpgrp();
	if ( pgid < 0 )
		FAIL2("getpgrp failed, %s", strerror(errno));

	if ( sigaddset(&cfg -> signal.child, SIGTSTP) < 0 )
		FAIL2("sigaddset SIGTSTP failed, %s", strerror(errno));

	if ( sigaddset(&cfg -> signal.child, SIGTTOU) < 0 )
		FAIL2("sigaddset SIGTTOU failed, %s", strerror(errno));

	if ( sigaddset(&cfg -> signal.child, SIGTTIN) < 0 )
		FAIL2("sigaddset SIGTTIN failed, %s", strerror(errno));

	int ttyfd = open("/dev/tty", O_RDWR | O_CLOEXEC);
	if ( ttyfd < 0 )
		WARN("non-fatal, could not open /dev/tty, %s", strerror(errno));

	if ( tcsetpgrp(ttyfd < 0 ? STDIN_FILENO : ttyfd, pgid) < 0 ) {

		if ( errno == ENOTTY || errno == EBADF )
			DEBUG("no tty present, %s", strerror(errno));
		else if ( errno == ENXIO )
			DEBUG("start_child failure, device not available: %s", strerror(errno));
		else {

			if ( ttyfd >= 0 )
				close(ttyfd);

			FAIL2("start_child failed, %s", strerror(errno));
		}
	}

	if ( ttyfd >= 0 )
		close(ttyfd);

	if ( sigprocmask(SIG_SETMASK, &cfg -> signal.child, NULL) < 0 )
		FAIL2("sigprocmask failed: %s", strerror(errno));

	if ( debugging ) {

		int i;

		fprintf(stdout, "[DEBUG] child cmd:");
		for ( i = 0; i < cfg -> argc; i++ ) {

			if ( cfg -> argv[i] != NULL )
				fprintf(stdout, " %s", cfg -> argv[i]);
		}

		fprintf(stdout, "\n");
	}

	if ( execvpe(cfg -> argv[0], cfg -> argv, environ) == -1 )
		FAIL2("failed to execute %s", cfg -> argv[0]);

	return 0; // not reached due to execvpe..
}

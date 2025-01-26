#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include "config.h"
#include "logging.h"
#include "child.h"

extern char **environ;

int start_child(struct config* cfg) {

	chdir("/");

	if ( cfg -> argc == 0 )
		return 0;

	pid_t child = fork();

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

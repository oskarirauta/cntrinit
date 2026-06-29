#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/prctl.h>

#include "config.h"
#include "logging.h"
#include "child.h"

extern char **environ;

int start_child(struct config* cfg) {

	if ( cfg -> op != CNTR ) {

		// infra mode: no child to run, just don't keep the startup directory pinned
		if ( chdir("/") == -1 )
			WARN("chdir failed, %s", strerror(errno));

		return 0;
	}

	pid_t self = getpid(); // init's own pid, used by the child to detect our death

	// flush buffered diagnostics so the forked child does not inherit and
	// re-emit them when stdout is not a terminal (and thus fully buffered)
	fflush(NULL);

	pid_t child = fork();

	if ( child < 0 )
		FAIL2("failed to fork, %s", strerror(errno));

	if ( child > 0 ) { // parent

		cfg -> child_pid = child;

		/*
		 * Put the child in its own process group from the parent side too.
		 * The child does the same, but doing it here as well closes the race
		 * window so that signal forwarding to the group (-g) is reliable.
		 * EACCES means the child already exec'd, ESRCH that it already exited.
		 */
		if ( setpgid(child, child) < 0 && errno != EACCES && errno != ESRCH )
			DEBUG("setpgid for child %d failed, %s", child, strerror(errno));

		// don't keep the container's working directory pinned in the init itself
		if ( chdir("/") == -1 )
			DEBUG("chdir failed, %s", strerror(errno));

		DEBUG("child process started with PID %d", child);

		write_cpidfile(cfg);

		return child;
	}

	/*
	 * Child. The working directory is left as the runtime set it (so WORKDIR
	 * is honoured) and the command is resolved by execvpe(): a name without a
	 * slash is looked up in PATH, otherwise it is taken as a literal path.
	 * We deliberately do not canonicalise it with realpath() so that
	 * multi-call binaries (busybox, coreutils, ...) still see the name they
	 * were invoked as in argv[0].
	 */

	if ( setpgid(0, 0) < 0 ) {
		ERR("setpgid failed, %s", strerror(errno));
		_exit(127);
	}

	pid_t pgid = getpgrp();
	if ( pgid < 0 ) {
		ERR("getpgrp failed, %s", strerror(errno));
		_exit(127);
	}

	/*
	 * All signals are still blocked here (inherited from the init's signalfd
	 * setup), so tcsetpgrp() cannot stop us with SIGTTOU while we grab the
	 * controlling terminal.
	 */
	int ttyfd = open("/dev/tty", O_RDWR | O_CLOEXEC);
	if ( ttyfd < 0 )
		DEBUG("non-fatal, could not open /dev/tty, %s", strerror(errno));

	if ( tcsetpgrp(ttyfd < 0 ? STDIN_FILENO : ttyfd, pgid) < 0 ) {

		if ( errno == ENOTTY || errno == EBADF )
			DEBUG("no tty present, %s", strerror(errno));
		else if ( errno == ENXIO )
			DEBUG("controlling terminal not available, %s", strerror(errno));
		else {

			if ( ttyfd >= 0 )
				close(ttyfd);

			ERR("tcsetpgrp failed, %s", strerror(errno));
			_exit(127);
		}
	}

	if ( ttyfd >= 0 )
		close(ttyfd);

	// restore the signal mask the init originally inherited so the child starts clean
	if ( sigprocmask(SIG_SETMASK, &cfg -> signal.child, NULL) < 0 ) {
		ERR("sigprocmask failed, %s", strerror(errno));
		_exit(127);
	}

	if ( debugging ) {

		int i;

		fprintf(stdout, "[DEBUG] child cmd:");
		for ( i = 0; i < cfg -> argc; i++ ) {

			if ( cfg -> argv[i] != NULL )
				fprintf(stdout, " %s", cfg -> argv[i]);
		}

		fprintf(stdout, "\n");
		fflush(stdout);
	}

	/*
	 * Arrange for the child to be signalled when the init dies (-k). This must
	 * be done here, in the child after fork(): PR_SET_PDEATHSIG is per-process
	 * and is cleared by fork(), but it survives execve(). It is set just before
	 * exec to keep the window small, and getppid() is checked afterwards to
	 * cover the race where the init already died during that window.
	 */
	if ( cfg -> kill_sig >= 0 ) {

		if ( prctl(PR_SET_PDEATHSIG, cfg -> kill_sig) != 0 )
			WARN("failed to set parent death signal %d, %s", cfg -> kill_sig, strerror(errno));
		else if ( getppid() != self )
			raise(cfg -> kill_sig); // init already gone, honour -k now
	}

	execvpe(cfg -> argv[0], cfg -> argv, environ);

	// only reached if execvpe failed
	ERR("failed to execute %s, %s", cfg -> argv[0], strerror(errno));
	_exit(127);
}

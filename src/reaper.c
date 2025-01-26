#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#include "logging.h"
#include "util.h"
#include "reaper.h"

int setup_child_reaper() {

	if ( getpid() == 1 )
		return 0;

	if ( prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0) != 0 ) {

		if ( errno == EINVAL )
			ERR("required PR_SET_CHILD_SUBREAPER is not available on this kernel");
		else ERR("Failed to register as a child process reaper, ", strerror(errno));

		return -1;
	}

	INFO("registered as a child process reaper");

	return 1;
}

int reaper(int* exitcode) {

	while ( true ) {

		int status = 0;
		pid_t pid = waitpid(-1, &status, WNOHANG);

		if ( pid <= 0 ) {

			if ( pid < 0 && errno == ECHILD ) {

				INFO("No child processes left");
				pid = 0;

			} else if ( pid < 0 ) {
				ERR("waitpid failure: ", strerror(errno));
				pid = -1;
			}

			return (int)pid;
		}

		if ( pid == 0 ) {

			if ( WIFEXITED(status))
				*exitcode = WEXITSTATUS(status);
			else if ( WIFSIGNALED(status))
				*exitcode = 128 + WTERMSIG(status);
			else if ( kill(pid, 0) < 0 )
				*exitcode = 127;
			else WARN("PID %d received SIGCHLD but process isn't dead", pid);

			continue;
		}

		if ( WIFEXITED(status)) {
			*exitcode = WEXITSTATUS(status);
			DEBUG("child process %d exited with status %d", pid, WEXITSTATUS(status));
		} else if ( WIFSIGNALED(status)) {
			*exitcode = 128 + WTERMSIG(status);
			DEBUG("child process %d killed with signal %d(%s)", pid, WTERMSIG(status), sig_to_string(WTERMSIG(status)));
		}
	}

	return 0;
}

#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#include "logging.h"
#include "util.h"
#include "reaper.h"

int setup_child_reaper(void) {

	if ( getpid() == 1 )
		return 0; // PID 1 already reaps orphaned processes

	if ( prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0) != 0 ) {

		if ( errno == EINVAL )
			ERR("required PR_SET_CHILD_SUBREAPER is not available on this kernel");
		else ERR("failed to register as a child process reaper, %s", strerror(errno));

		return -1;
	}

	DEBUG("registered as a child process reaper");

	return 1;
}

int reaper(pid_t child_pid, int* exitcode) {

	while ( true ) {

		int status = 0;
		pid_t pid = waitpid(-1, &status, WNOHANG);

		if ( pid == 0 )
			return 0; // children remain, but none are ready to be reaped

		if ( pid < 0 ) {

			if ( errno == ECHILD )
				return 0; // no children left; keep waiting for signals
			if ( errno == EINTR )
				continue;

			ERR("waitpid failure, %s", strerror(errno));
			return -1;
		}

		int code;

		if ( WIFEXITED(status)) {
			code = WEXITSTATUS(status);
			DEBUG("process %d exited with status %d", pid, code);
		} else if ( WIFSIGNALED(status)) {
			code = 128 + WTERMSIG(status);
			DEBUG("process %d killed with signal %d(%s)", pid, WTERMSIG(status), sig_to_string(WTERMSIG(status)));
		} else continue; // stopped/continued, not a termination

		if ( child_pid >= 0 && pid == child_pid ) {
			*exitcode = code;
			return 1; // our main child is gone, time to exit
		}

		DEBUG("reaped orphaned process %d", pid);
	}
}

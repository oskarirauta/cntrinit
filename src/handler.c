#include <unistd.h>
#include <sys/wait.h>
#include <sys/signalfd.h>

#include "logging.h"
#include "reaper.h"
#include "util.h"
#include "handler.h"

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(*arr))

int setup_signal_handler(struct config* cfg) {

	if ( sigfillset(&cfg -> signal.parent) < 0 )
		FAIL("sigfillset failure, %s", strerror(errno));

	size_t i;
	int signals[] = { SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGABRT, SIGTRAP, SIGSYS };

	for ( i = 0; i < ARRAY_LEN(signals); i++ ) {
		if ( sigdelset(&cfg -> signal.parent, signals[i]) != 0 )
			FAIL("sigdelset %s failure, %s", sig_to_string(signals[i]), strerror(errno));
	}

	if ( sigprocmask(SIG_SETMASK, &cfg -> signal.parent, &cfg -> signal.child) != 0 )
		FAIL("sigprocmask failure, %s", strerror(errno));

	cfg -> signal.fd = signalfd(-1, &cfg -> signal.parent, SFD_CLOEXEC);
	if ( cfg -> signal.fd < 0 )
		FAIL("signalfd failure, %s", strerror(errno));

	return 0;
}

int handle_signals(struct config* cfg) {

	int exitcode = 0;

	while ( true ) {

		struct signalfd_siginfo info;
		ssize_t n = read(cfg -> signal.fd, &info, sizeof(info));

		if ( n != (ssize_t)sizeof(info)) {

			if ( n < 0 ) {
				if ( errno == EINTR )
					continue;
				WARN("signalfd read failed, %s", strerror(errno));
			} else WARN("signalfd read returned unexpected size %zd", n);

			continue;
		}

		if ( info.ssi_signo == SIGTSTP || info.ssi_signo == SIGTTOU || info.ssi_signo == SIGTTIN )
			DEBUG("ignored job control signal %s", sig_to_string(info.ssi_signo));
		else if ( info.ssi_signo == SIGCHLD ) {

			int r = reaper(cfg -> child_pid, &exitcode);

			if ( r < 0 )
				return -1;
			if ( r == 1 )
				return exitcode; // main child has exited
			// otherwise only orphans were reaped; keep running

		} else if ( cfg -> op == INFRA && ( info.ssi_signo == SIGTERM || info.ssi_signo == SIGINT )) {
			DEBUG("signal %s received, infra exiting", sig_to_string(info.ssi_signo));
			return 0;
		} else if ( cfg -> op == CNTR && cfg -> child_pid >= 0 ) { // forward signals to child

			pid_t target = cfg -> forward_pgroup ? -cfg -> child_pid : cfg -> child_pid;

			if ( kill(target, info.ssi_signo) < 0 )
				WARN("forwarding signal %d(%s) to child process %d failed, %s",
					info.ssi_signo, sig_to_string(info.ssi_signo), cfg -> child_pid, strerror(errno));
		}
	}
}

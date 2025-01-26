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
		FAIL("sigfillset failure: ", strerror(errno));

	int i;
	int signals[] = { SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGABRT, SIGTRAP, SIGSYS };

	for ( i = 0; i < ARRAY_LEN(signals); i++ ) {
		if ( sigdelset(&cfg -> signal.parent, signals[i]) != 0 )
			FAIL("sigdelset %s failure: ", sig_to_string(signals[i]), strerror(errno));
	}

	if ( sigprocmask(SIG_SETMASK, &cfg -> signal.parent, &cfg -> signal.child) != 0 )
		FAIL("sigprocmask failure: ", strerror(errno));

	cfg -> signal.fd = signalfd(-1, &cfg -> signal.parent, SFD_CLOEXEC);
	if ( cfg -> signal.fd < 0 )
		FAIL("signalfd failure: ", strerror(errno));

	return 0;
}

int handle_signals(struct config* cfg) {

	int exitcode = -1;

	while ( exitcode < 0 ) {

		struct signalfd_siginfo info;
		int n = read(cfg -> signal.fd, &info, sizeof(info));

		if ( n != sizeof(info)) {

			if ( n < 0 )
				WARN("signalfd read failed: ", strerror(errno));
			else WARN("signalfd read failed, unexpected EOF");

			continue;
		}

		if ( info.ssi_signo == SIGTSTP || info.ssi_signo == SIGTTOU || info.ssi_signo == SIGTTIN )
			DEBUG("ignored kernel signal %s", sig_to_string(info.ssi_signo));
		else if ( info.ssi_signo == SIGCHLD ) {

			if ( reaper(&exitcode) < 0 )
				return -1;

			break;

		} else if ( cfg -> op == INFRA && ( info.ssi_signo == SIGTERM || info.ssi_signo == SIGINT )) {
			DEBUG("signal %d(%s) received, infra dies", info.ssi_pid, sig_to_string(info.ssi_signo));
			return 0;
		} else if ( cfg -> op == CNTR && cfg -> child_pid >= 0 ) { // forward signals to child

			if ( kill(cfg -> forward_pgroup ? -cfg -> child_pid : cfg -> child_pid, info.ssi_signo) < 0 )
				WARN("signal %d(%s) forwarding to child process %d failed, ",
					info.ssi_signo, sig_to_string(info.ssi_signo), cfg -> child_pid, strerror(errno));
		}
	}

	return exitcode;
}

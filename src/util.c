#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "util.h"

struct entry {
	int sig;
	const char *name;
};

static struct entry signals[] = {
	{ SIGHUP, "SIGHUP" },
	{ SIGINT, "SIGINT" },
	{ SIGQUIT, "SIGQUIT" },
	{ SIGILL, "SIGILL" },
	{ SIGTRAP, "SIGTRAP" },
	{ SIGABRT, "SIGABRT" },
	{ SIGBUS, "SIGBUS" },
	{ SIGFPE, "SIGFPE" },
	{ SIGKILL, "SIGKILL" },
	{ SIGUSR1, "SIGUSR1" },
	{ SIGUSR2, "SIGUSR2" },
	{ SIGSEGV, "SIGSEGV" },
	{ SIGPIPE, "SIGPIPE" },
	{ SIGALRM, "SIGALRM" },
	{ SIGTERM, "SIGTERM" },
	{ SIGCHLD, "SIGCHLD" },
	{ SIGCONT, "SIGCONT" },
	{ SIGSTOP, "SIGSTOP" },
	{ SIGTSTP, "SIGTSTP" },
	{ SIGTTIN, "SIGTTIN" },
	{ SIGTTOU, "SIGTTOU" },
	{ SIGURG, "SIGURG" },
	{ SIGXCPU, "SIGXCPU" },
	{ SIGXFSZ, "SIGXFSZ" },
	{ SIGVTALRM, "SIGVTALRM" },
	{ SIGPROF, "SIGPROF" },
	{ SIGWINCH, "SIGWINCH" },
	{ SIGSYS, "SIGSYS" },
	{ 0, NULL }
};

int sig_to_int(char* name) {

	if ( name == NULL || strcmp(name, "") == 0 )
		return -1;

	int i = 0;
	while ( signals[i].name != NULL ) {
		if ( strcmp(signals[i].name, name) == 0 )
			return signals[i].sig;
		i++;
	}

	return -1;
}

const char* sig_to_string(int sig) {

	int i = 0;
	while ( signals[i].name != NULL ) {
		if ( signals[i].sig == sig )
			return signals[i].name;
		i++;
	}

	return NULL;
}

void print_supported_signal_names() {

	int i = 0;
	while ( signals[i].name != NULL ) {
		printf(i == 0 ? "%s" : ", %s", signals[i].name);
		i++;
	}
	printf("\n");
}

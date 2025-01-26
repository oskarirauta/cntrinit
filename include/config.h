#ifndef _CONFIG_H_
#define _CONFIG_H_ 1

#include <stdbool.h>
#include <signal.h>

enum OPERATION {
	INFRA = 0, CNTR = 1, VERSION = 2, USAGE = 3
};

struct signal_config {
	sigset_t parent;
	sigset_t child;
	int fd;
};

struct config {
	char *cmd;
	char* short_name;
	char* long_name;
	enum OPERATION op;
	bool forward_pgroup;
	char* cwd;
	char* pidfile;
	char* cpidfile;
	int kill_sig;
	int argc;
	char **argv;
	pid_t child_pid;
	struct signal_config signal;
};

#define new_config { \
	.cmd = NULL, \
	.short_name = NULL, \
	.long_name = NULL, \
	.op = INFRA, \
	.forward_pgroup = false, \
	.cwd = NULL, \
	.pidfile = NULL, \
	.cpidfile = NULL, \
	.kill_sig = -1, \
	.argc = 0, \
	.argv = NULL, \
	.child_pid = -1 \
}

void get_cwd(struct config* cfg);
void write_pidfile(struct config* cfg);
void write_cpidfile(struct config* cfg);
void prepare_config(struct config* cfg, int argc, char** argv);
void free_config(struct config* cfg);

#endif

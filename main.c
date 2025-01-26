#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "logging.h"
#include "config.h"
#include "usage.h"
#include "reaper.h"
#include "child.h"
#include "handler.h"

int main(int argc, char** argv) {

	int i;
	struct config cfg = new_config;

	i = parse_args(&cfg, argc, argv);
	if ( i != 0 ) {
		free_config(&cfg);
		return i == 1 ? 0 : 1;
	}

	DEBUG("PID %d", getpid());
	DEBUG("running in %s mode", cfg.op == INFRA ? "infra" : "container");
	get_cwd(&cfg);

	i = setup_signal_handler(&cfg);
	if ( i != 0 ) {
		free_config(&cfg);
		DIE("exiting");
	}

	if ( setup_child_reaper < 0 ) {
		free_config(&cfg);
		DIE("exiting");
	}

	prepare_config(&cfg, argc, argv);

	i = start_child(&cfg);
	if ( i < 0 ) {
		free_config(&cfg);
		DIE("exiting");
	}

	i = handle_signals(&cfg);
	if ( i < 0 ) {
		free_config(&cfg);
		DIE("exiting")
	}

	free_config(&cfg);
	return i;
}

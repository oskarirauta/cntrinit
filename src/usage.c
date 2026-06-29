#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>

#include "logging.h"
#include "config.h"
#include "util.h"
#include "usage.h"

#ifndef max
#define max(x,y) ((x) < (y) ? (y) : (x))
#endif

#ifndef min
#define min(x,y) ((x) > (y) ? (y) : (x))
#endif

static const char version_string[] = "1.2.0";

static struct option long_opts[] = {
	{ "name", required_argument, 0, 'n' },
	{ "kill-signal", required_argument, 0, 'k' },
	{ "group-signal", no_argument, NULL, 'g' },
	{ "pid-file", required_argument, 0, 'p' },
	{ "cpid-file", required_argument, 0, 'c' },
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'v' },
	{ "debug", no_argument, NULL, 'd' },
	{ NULL, 0, NULL, 0 }
};

// leading '+': stop option scanning at the first non-option, so tokens after the
// command (`cntrinit /bin/sh -d`) go to the child instead of being permuted and
// parsed as our own flags. The documented `-- cmd args` path is unaffected.
static const char* short_opts = "+n:k:gp:c:hvd";

static void version() {

	printf("cntrinit v%s\n", version_string);
}

static void usage(struct config* cfg) {

	version();
	printf("author: Oskari Rauta\n");
	printf("license: MIT\n\n");
	printf("usage:\n\t%s [options] -- cmd args\n\n", basename(cfg -> cmd));

	printf("options:\n");
	printf("  -n, --name NAME\t\tname process\n");
	printf("  -k, --kill-signal SIGNAL\tsend this signal to child when parent is killed\n");
	printf("  -g, --group-signal\t\tsend signals to child's process group instead of just child\n");
	printf("  -p, --pid-file FILENAME\t write pidfile\n");
	printf("  -c, --cpid-file FILENAME\t write pidfile for child process\n");
	printf("  -h, --help\t\t\tshow this usage message\n");
	printf("  -v, --version\t\t\tshow version\n");
	printf("  -d, --debug\t\t\tenable debug output\n");

	printf("\nIf you leave out command - cntrinit will run as infra without child process\n\n");
}

int parse_args(struct config* cfg, int argc, char** argv) {

	int c;
	size_t sz;
	char* name = NULL;
	bool skip_args = argc > 1 ? true : false;

	cfg -> cmd = xstrdup(( argc > 0 && argv[0] ) ? argv[0] : "cntrinit"); // argc==0 => argv[0]==NULL
	opterr = 0;

	while ( true ) {

		int option_index = 0;
		c = getopt_long(argc, argv, short_opts, long_opts, &option_index);

		if ( c == -1 )
			break;

		switch ( c ) {
			case 'n':
				free(name); // free(NULL) is a no-op
				name = NULL;

				if ( optarg[0] != '\0' )
					name = xstrdup(optarg);

				break;
			case 'k': {
				int kill_sig = sig_to_int(optarg);
				if ( kill_sig == -1 ) {

					printf("unknown kill signal %s\n", optarg);
					printf("list of supported signals (remember to use uppercase characters):\n");
					print_supported_signal_names();

					if ( name != NULL )
						free(name);

					return -1;

				} else cfg -> kill_sig = kill_sig;
				break;
			}
			case 'g':
				cfg -> forward_pgroup = true;
				break;
			case 'p':
				free(cfg -> pidfile);
				cfg -> pidfile = xstrdup(optarg);
				break;
			case 'c':
				free(cfg -> cpidfile);
				cfg -> cpidfile = xstrdup(optarg);
				break;
			case 'h':
				cfg -> op = USAGE;
				break;
			case 'v':
				cfg -> op = cfg -> op == USAGE ? USAGE : VERSION;
				break;
			case 'd':
				debugging = true;
				break;
			default:

				if ( optopt == 'n' || optopt == 'k' || optopt == 'p' || optopt == 'c' )
					ERR("option -%c needs argument", optopt);
				else if ( optopt )
					ERR("unknown option -%c", optopt);
				else ERR("unknown option");

				free(name);

				return -1;
		}
	}

	if ( !skip_args ) {
		WARN("are you sure you want to run %s without any args? Add arg -- to ignore this message.", basename(cfg -> cmd));
		return -1;
	}

	if ( cfg -> op == USAGE || cfg -> op == VERSION ) {

		if ( name != NULL )
			free(name);

		if ( cfg -> op == VERSION )
			version();
		else usage(cfg);

		return 1;
	}

	if ( optind < argc ) {

		c = 0;
		sz = sizeof(char*) * ( argc - optind + 1); // +1 for NULL terminator

		cfg -> op = CNTR;
		cfg -> argc = argc - optind;
		cfg -> argv = xmalloc(sz);
		memset(cfg -> argv, 0, sz);

		while ( optind < argc ) {
			cfg -> argv[c] = xstrdup(argv[optind]);
			c++;
			optind++;
		}

		cfg -> argv[c] = NULL;
		cfg -> op = CNTR;
	}

	if ( name != NULL ) {

		sz = min((size_t)13, strlen(name));
		cfg -> short_name = xmalloc(sz + 3);
		memset(cfg -> short_name, 0, sz + 3);
		memcpy(cfg -> short_name + 1, name, sz);
		cfg -> short_name[0] = '[';
		cfg -> short_name[sz + 1] = ']';

		sz = 4 + strlen(name) + ( cfg -> op == INFRA ? 5 : 4 );
		cfg -> long_name = xmalloc(sz + 1);
		memset(cfg -> long_name, 0, sz + 1);
		snprintf(cfg -> long_name, sz + 1, "[%s: %s]", cfg -> op == INFRA ? "infra" : "cntr", name);

		free(name);
	}

	return 0;
}

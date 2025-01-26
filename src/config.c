#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

#include "logging.h"
#include "util.h"
#include "config.h"

void write_pidfile(struct config* cfg) {

	if ( cfg -> pidfile == NULL || strcmp(cfg -> pidfile, "") == 0 )
		return;

	FILE *fp = fopen(cfg -> pidfile, "w");
	if ( !fp ) {
		WARN("cannot create pidfile %s, %s", cfg -> pidfile, strerror(errno));
		return;
	}

	fprintf(fp, "%d\n", getpid());
	fclose(fp);

	DEBUG("pidfile %s created", cfg -> pidfile);
}

void write_cpidfile(struct config* cfg) {

	if ( cfg -> cpidfile == NULL || strcmp(cfg -> cpidfile, "") == 0 )
		return;

	FILE *fp = fopen(cfg -> cpidfile, "w");
	if ( !fp ) {
		WARN("cannot create pidfile %s for child process, %s", cfg -> cpidfile, strerror(errno));
		return;
	}

	fprintf(fp, "%d\n", cfg -> child_pid);
	fclose(fp);

	DEBUG("pidfile %s created for child process", cfg -> cpidfile);
}

void prepare_config(struct config* cfg, int argc, char** argv) {

	write_pidfile(cfg);

	if ( cfg -> short_name ) {

		if ( prctl(PR_SET_NAME, cfg -> short_name, NULL, NULL, NULL) != 0 )
			DEBUG("Failed to set process short name to %s", cfg -> short_name);
		else DEBUG("Process short name set to %s", cfg -> short_name);
	}

	if ( cfg -> long_name ) {

		int i, i2, sz;

		for ( i = 0; i < argc; i++ ) {

			sz = strlen(argv[i]);

			for ( i2 = 0; i2 < sz; i2++ )
				argv[i][i2] = 0;
		}

		if ( sprintf(argv[0], cfg -> long_name) != strlen(cfg -> long_name))
			DEBUG("Failed to set process long name to %s", cfg -> long_name);
		else DEBUG("Process long name set to %s", cfg -> long_name);
	}

	if ( cfg -> kill_sig >= 0 ) {

		if ( prctl(PR_SET_PDEATHSIG, cfg -> kill_sig) != 0 )
			WARN("Failed to set parent death signal to %s", sig_to_string(cfg -> kill_sig));
		else DEBUG("Parent death signal set to %s", sig_to_string(cfg -> kill_sig));
	}
}

void free_config(struct config* cfg) {

	if ( cfg -> cmd )
		free(cfg -> cmd);

	if ( cfg -> short_name )
		free(cfg -> short_name);

	if ( cfg -> long_name )
		free(cfg -> long_name);

	if ( cfg -> pidfile )
		free(cfg -> pidfile);

	if ( cfg -> cpidfile )
		free(cfg -> cpidfile);

	if ( cfg -> argc > 0 ) {

		int i;
		for ( i = cfg -> argc; i > 0; i-- ) {

			if ( cfg -> argv[i - 1] != NULL )
				free(cfg -> argv[i - 1]);
		}

		free(cfg -> argv);
	}

	cfg -> cmd = NULL;
	cfg -> short_name = NULL;
	cfg -> long_name = NULL;
	cfg -> pidfile = NULL;
	cfg -> cpidfile = NULL;
	cfg -> argc = 0;
	cfg -> argv = NULL;
}

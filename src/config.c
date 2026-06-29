#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <linux/limits.h>

#include "logging.h"
#include "util.h"
#include "config.h"

void get_cwd(struct config* cfg) {

	char cwd[PATH_MAX];
	if ( getcwd(cwd, sizeof(cwd)) != NULL ) {
		cfg -> cwd = xstrdup(cwd);
		DEBUG("current working directory is %s", cfg -> cwd);
	} else WARN("getcwd failed, %s", strerror(errno));
}

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

	if ( cfg -> long_name && argc > 0 ) {

		/*
		 * argv strings are laid out contiguously in memory, so the writable
		 * region for the process title spans from argv[0] up to the end of
		 * the last argument. Bound the write to that region to avoid
		 * clobbering adjacent memory (e.g. the environment).
		 */
		char* start = argv[0];
		char* end = argv[argc - 1] + strlen(argv[argc - 1]) + 1;
		size_t avail = (size_t)(end - start);

		memset(start, 0, avail);

		int w = snprintf(start, avail, "%s", cfg -> long_name);
		if ( w < 0 || (size_t)w >= avail )
			DEBUG("process long name truncated to fit argv (%zu bytes available)", avail);
		else DEBUG("process long name set to %s", cfg -> long_name);
	}

	/*
	 * Note: the parent-death signal (-k) is NOT set here. PR_SET_PDEATHSIG
	 * applies to the calling process and is cleared across fork(), so it must
	 * be set by the child after fork() (see start_child()) for it to take
	 * effect for the child process.
	 */
}

void free_config(struct config* cfg) {

	if ( cfg -> cmd )
		free(cfg -> cmd);

	if ( cfg -> short_name )
		free(cfg -> short_name);

	if ( cfg -> long_name )
		free(cfg -> long_name);

	if ( cfg -> cwd )
		free(cfg -> cwd);

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

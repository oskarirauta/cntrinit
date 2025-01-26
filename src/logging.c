#include <stdio.h>
#include <errno.h>
#include "logging.h"

bool debugging = false;

void _log(enum LOG_LEVEL level, char* fmt, ...) {

	switch ( level ) {
		case LOG_INFO:
			fprintf(stdout, "[INFO] ");
			break;
		case LOG_WARN:
			fprintf(stderr, "[WARN] ");
			break;
		case LOG_ERR:
			fprintf(stderr, "[ERROR] ");
			break;
		case LOG_FATAL:
			fprintf(stderr, "[FATAL] ");
			break;
		case LOG_DEBUG:
		default:
			if ( !debugging )
				return;
			fprintf(stdout, "[DEBUG] ");
			break;
	}

	int _errno = errno;
	va_list args;

	va_start(args, fmt);
	vfprintf(level == LOG_INFO || level == LOG_DEBUG ? stdout : stderr,
		fmt, args);
	va_end(args);

	fprintf(level == LOG_INFO || level == LOG_DEBUG ? stdout : stderr,
		"\n");
	errno = _errno;
}

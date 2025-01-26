#ifndef _LOGGING_H_
#define _LOGGING_H_ 1

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern bool debugging;

enum LOG_LEVEL {
        LOG_INFO = 0, LOG_WARN = 1, LOG_ERR = 2, LOG_FATAL = 3, LOG_DEBUG = 4
};

void _log(enum LOG_LEVEL level, char* fmt, ...);

#define INFO(...) _log(LOG_INFO, __VA_ARGS__)
#define WARN(...) _log(LOG_WARN, __VA_ARGS__)
#define ERR(...) _log(LOG_ERR, __VA_ARGS__)
#define DEBUG(...) _log(LOG_DEBUG, __VA_ARGS__)
#define FAIL(...) { _log(LOG_ERR, __VA_ARGS__); return 1; }
#define FAIL2(...) { _log(LOG_ERR, __VA_ARGS__); return -1; }
#define DIE(...) { _log(LOG_FATAL, __VA_ARGS__); exit(1); }

#endif

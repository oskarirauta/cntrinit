#ifndef _REAPER_H_
#define _REAPER_H_ 1

#include <sys/types.h>

int setup_child_reaper(void);

/*
 * Reap any terminated children without blocking.
 *
 * Returns:
 *    1  the main child (child_pid) has terminated; *exitcode is set
 *    0  the main child is still alive (only orphans were reaped, if any)
 *   -1  a fatal waitpid() error occurred
 */
int reaper(pid_t child_pid, int* exitcode);

#endif

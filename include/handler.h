#ifndef _HANDLER_H_
#define _HANDLER_H_ 1

#include <signal.h>
#include "config.h"

int setup_signal_handler(struct config* cfg);
int handle_signals(struct config* cfg);

#endif

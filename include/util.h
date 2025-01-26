#ifndef _UTIL_H_
#define _UTIL_H_ 1

int sig_to_int(char* name);
const char* sig_to_string(int sig);
void print_supported_signal_names();

#endif

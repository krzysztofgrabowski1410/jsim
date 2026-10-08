#ifndef MIM_ERR_H
#define MIM_ERR_H

#include <stdnoreturn.h>
// This is a slightly edited version of the err library from labs.

// Sets verbosity gate for fatal/syserr/error; default 2.
void err_set_verbosity(int v);

// Print info about a syscall error (level 2) and quit with status 1.
noreturn void syserr(const char* fmt, ...);

// Print info about a critical error (level 2) and quit with status 1.
noreturn void fatal(const char* fmt, ...);

// Print info about an invocation error (always, regardless of verbosity)
// and quit with status 1.
noreturn void fatal_usage(const char* fmt, ...);

// Print info about a non-critical error (level 3) and return.
void error(const char* fmt, ...);

#endif

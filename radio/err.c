#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"

static int g_verbosity = 2;

void err_set_verbosity(int v) {
    g_verbosity = v;
}

noreturn void syserr(const char* fmt, ...) {
    int org_errno = errno;
    if (g_verbosity >= 2) {
        va_list fmt_args;
        fprintf(stderr, "\tERROR: ");
        va_start(fmt_args, fmt);
        vfprintf(stderr, fmt, fmt_args);
        va_end(fmt_args);
        fprintf(stderr, " (%d; %s)\n", org_errno, strerror(org_errno));
    }
    exit(1);
}

noreturn void fatal(const char* fmt, ...) {
    if (g_verbosity >= 2) {
        va_list fmt_args;
        fprintf(stderr, "\tERROR: ");
        va_start(fmt_args, fmt);
        vfprintf(stderr, fmt, fmt_args);
        va_end(fmt_args);
        fprintf(stderr, "\n");
    }
    exit(1);
}

noreturn void fatal_usage(const char* fmt, ...) {
    va_list fmt_args;
    fprintf(stderr, "\tERROR: ");
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);
    fprintf(stderr, "\n");
    exit(1);
}

void error(const char* fmt, ...) {
    int org_errno = errno;
    if (g_verbosity >= 3) {
        va_list fmt_args;
        fprintf(stderr, "\tERROR: ");
        va_start(fmt_args, fmt);
        vfprintf(stderr, fmt, fmt_args);
        va_end(fmt_args);
        if (org_errno != 0) {
            fprintf(stderr, " (%d; %s)", org_errno, strerror(org_errno));
        }
        fprintf(stderr, "\n");
    }
}

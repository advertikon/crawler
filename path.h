#ifndef PATH_H
#define PATH_H

#include <limits.h>
#include <error.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#ifdef PATH_MAX
	static long pathmax = PATH_MAX;
#else
	static long pathmax = 0;
#endif

static long posix_version = 0;
static long xsi_version = 0;

/* If PATH_MAX is indeterminate, no guarantee this is adequate */
#define PATH_MAX_GUESS 4096

char* path_alloc( size_t* );

#endif
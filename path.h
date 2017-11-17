#ifndef PATH_H
#define PATH_H

#include <limits.h>
#include <error.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <glob.h>
#include <gtk/gtk.h>
#include <dirent.h>
#include <string.h>
#include <wchar.h>

#ifdef PATH_MAX
	static long pathmax = PATH_MAX;
#else
	static long pathmax = 0;
#endif

static long posix_version = 0;
static long xsi_version = 0;

/* If PATH_MAX is indeterminate, no guarantee this is adequate */
#define PATH_MAX_GUESS 4096

size_t get_path_max_size( void );
glob_t* Glob( const char *restrict );
GSList* Scandir( char *dirname );
void dump_slist( GSList* );
void dump_string( char * );
void dump_hash( gpointer, gpointer, gpointer );

#endif
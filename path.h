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
#include <sys/stat.h>
#include <signal.h>

#ifdef PATH_MAX
	static long pathmax = PATH_MAX;
#else
	static long pathmax = 0;
#endif

#define B_BLACK  "\e[1;30m"
#define B_RED    "\e[1;31m"
#define B_GREEN  "\e[1;32m"
#define B_YELLOW "\e[1;33m"
#define B_BLUE   "\e[1;34m"
#define B_PURPLE "\e[1;35m"
#define B_CYAN   "\e[1;36m"
#define B_WHITE  "\e[1;37m"

typedef int (*cb)( char*, void* );
typedef void Sigfunc( int );

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
void dump_vector( char** );
void print_color( const char *, char *, ... );
int Unlink( char* );
struct stat *Lstat( char* );
struct stat *_Lstat( char* );
gboolean is_dir( char* );
gboolean is_file( char*);
void clean_stat_cache( char* );
size_t filesize( char* );
char *Strcat( char*, ... ) G_GNUC_NULL_TERMINATED;
int iterate(  char* path, cb file_c, cb dir_c, cb err_c );

#endif
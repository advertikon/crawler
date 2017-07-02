#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <yaml.h>
#include <signal.h>
#include <regex.h>
#include <time.h>
#include <sys/times.h>
#include <termios.h>
#ifndef TIOCGWINSZ
#include <sys/ioctl.h>
#endif

#include "error.h"
#include "args.h"
#include "path.h"
#include "structure.h"

int usage( void );
int iterate( const char*, int* );
int parse_config( void );
int save_config( void );
int add_to( const char* );
int del_from( const char*, struct llist* );
int start_add( struct llist* );
int start_del( struct llist* );
int show_commands( void );
int add_to_config( struct llist* );
int remove_from_config( struct llist* );
int abort_config( void );
int print_del_list( struct llist* );
int confirmed_operation( void );
int end_operation( void );
int write_config_section( char*, FILE*, struct llist* );
void int_handler( int );
int check_file( const char* );
int collide_length( const char*, struct llist*, int* );
int collide_span( const char*, const char* );
char* get_regerror( int, regex_t* );
int match( const char*, const char*, regmatch_t*, int );
int check_regexp( const char*, struct llist* );
void start_clock( void );
void end_clock( char* );
int print_config( void );
int print_files( void );
static void set_winsize( void );
static void sig_winch( int );
int load_dependencies( void );
char *ltrim( char*, char* );
char *trim( char*, char* );
char *rtrim( char*, char* );

#define MAX_LINE 200
#define DEBUG 1
#define REGEX_MATCH_COUNT 10

#define IS_EMPTY( p ) ( NULL == p || '\0' == *p )

enum COMMANDS {
	C_ADD_INCL_FOLDER = 1,
	C_ADD_EXCL_FOLDER,
	C_DEL_INCL_FOLDER,
	C_DEL_EXCL_FOLDER,
	C_ADD_INCL_FILE,
	C_ADD_EXCL_FILE,
	C_DEL_INCL_FILE,
	C_DEL_EXCL_FILE,
	C_ADD_INCL_REGEXP,
	C_ADD_EXCL_REGEXP,
	C_DEL_INCL_REGEXP,
	C_DEL_EXCL_REGEXP,
	C_SET_NAME,
	C_ITERATE,
	C_PRINT_FILES,
	C_PRINT_CONFIG
};

typedef void Sigfunc( int );

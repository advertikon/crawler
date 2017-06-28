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

#include "error.h"
#include "args.h"
#include "path.h"
#include "structure.h"

int usage( void );
int iterate( const char* );
int parse_config( void );
int save_config( void );
int add_to( char* );
int del_from( char*, struct llist* );
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

#define MAX_LINE 200

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
	C_SET_NAME
};

typedef void Sigfunc( int );

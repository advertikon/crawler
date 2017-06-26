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

#include "error.h"
#include "args.h"
#include "path.h"
#include "structure.h"

int usage( void );
int iterate( const char* );
int parse_config( void );
int save_config( void );
int add_to( char*, struct llist* );
int del_from( char*, struct llist* );
int start_add();
int start_del( struct llist* );
int show_commands( void );
int flush_config( void );
int clear_temp_config( void );

#define MAX_LINE 200

#define C_ADD_INCL_FOLDER 1
#define C_ADD_EXCL_FOLDER 2
#define C_DEL_INCL_FOLDER 3
#define C_DEL_EXCL_FOLDER 4
#define C_SET_NAME 5

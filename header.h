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
int add_include_dir( char* );
int del_include_dir( int );
int start_add_include_dir( void );
int start_del_include_dir( void );

#define MAX_LINE 200

enum COMMANDS {
	C_ADD_INCL_FOLDER = 1;
	C_ADD_EXCL_FOLDER;
	C_DEL_INCL_FOLDER;
	C_DEL_EXCL_FOLDER;
};

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
/*#include <yaml.h>*/

#include "error.h"
#include "args.h"
#include "path.h"
#include "structure.h"
// #include "yaml.h"

int usage( void );
int iterate( const char* );

#define MAX_LINE 200

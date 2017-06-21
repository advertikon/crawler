
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

typedef struct arg_list {
	char *name;
	char *value;
	struct arg_list *next;
} ALIST;


int parse_args();
int add_arg( char*, char* );
int get_arg( char*, char** );
int print_args();
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef STRUCT_H
#define STRUCT_H

struct llist_item {
	char* name;
	char* value;
	struct llist_item* next;
};

struct llist {
	struct llist_item* first;
	struct llist_item* current;
	int(*add)( const char*, char*, struct llist* );
	int(*get)( char*, char**, struct llist* );
	int(*print)( struct llist* );
};

int add( const char*, char*, struct llist* );
int get( char*, char**, struct llist* );
int print( struct llist* );
struct llist* init_llist();
#endif
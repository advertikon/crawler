#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

struct llist_item {
	char* name;
	char* value;
	struct llist_item* next;
	int index;
};

struct llist {
	struct llist_item* first;
	struct llist_item* current;
	int(*add)( const char*, const char*, struct llist* );
	int(*get)( const char*, char**, struct llist* );
	int(*has_value)( const char*, struct llist* );
	int(*print)( struct llist* );
	int(*remove)( const char*, struct llist* );
	int(*merge)( struct llist*, struct llist* );
	int(*empty)( struct llist* );
	int(*count)( struct llist* );
	size_t size;
};

static int s_add( const char*, const char*, struct llist* );
static int s_get( const char*, char**, struct llist* );
static int s_has_value( const char*, struct llist* );
static int s_print( struct llist* );
static int s_remove( const char*, struct llist* );
static int s_merge( struct llist*, struct llist* );
static int s_empty( struct llist* );
struct llist* init_llist( void );
static int s_has_value( const char*, struct llist* );
static int s_count( struct llist* );

#define L_MEM( l ) ( ( strlen( l->name ) + strlen( l->value ) + sizeof( struct llist_item ) + 2 ) )

#endif
/**
 * Contains custom structure types
 *
 * Requires:
 * <stdio.h>
 * <stdlib.h>
 * <string.h>
 */

typedef struct llist_item {
	char* name;
	char* value;
	struct llist_item* next;
} LLI;

typedef struct llist {
	struct llist_item* first;
	struct llist_item* current;
	int(*add)( const char*, char*, struct llist* );
	int(*get)( char*, char**, struct llist* );
	int(*print)( struct llist* );
} LL;

static int add( const char*, char*, LL* );
static int get( char*, char**, LL* );
static int print( LL* );
LL* init_llist();

static size_t total_size = 0;

int add( const char *name, char *value, LL* l ) {
	printf("Add %s = %s\n", name, value );
	printf( "Size: %d\n", total_size );

	while( l->current ) {
		printf( "Current ponter: %p\n", l->current );
		l->current = l->current->next;
	}

	printf( "Get empty slot\n" );

	l->current = malloc( sizeof( LLI ) );

	total_size += sizeof( LLI );

	printf( "Allocate memory for list item strcture\n" );

	l->current->name = malloc( strlen( name ) );

	total_size += strlen( name );

	printf( "Allocate memory for name\n" );

	strcpy( l->current->name, name );

	printf( "Copt name\n" );

	l->current->value = malloc( strlen( value ) );

	total_size += strlen( value );

	printf( "Allocate memory for value\n" );

	strcpy( l->current->value, value );

	printf( "Copy value\n" );

	if ( NULL == l->first ) {
		printf( "First is empty\n" );
		l->first = l->current;
	}

	printf( "Exit add\n" );

	return 0;
}

int get( char *name, char** value, LL* l ) {
	l->current = l->first;
	int found = 0;

	while ( l->current->name ) {
		if ( !strcmp( l->current->name, name ) ) {
			found = 1;
			break;
		}

		l->current = l->current->next;
	}

	if ( found ) {
		*value = l->current->value;
	}

	l->current = l->first;

	return found ? 0 : 1;
}

int print( LL* l ) {
	l->current = l->first;

	while( l->current->name ) {
		printf( "Current structure to print: %s = %s\n", l->current->name, l->current->value );
		l->current = l->current->next;
	}

	return 0;
}

LL* init_llist() { printf( "Init start\n" );
	// LLI* tt = malloc( sizeof( LLI ) );
	LL* t = malloc( sizeof( LL ) );
	t->add = &add;

printf( "Init end\n" );
	return t;
}
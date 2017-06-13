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
	size_t size;
	char foo[ 100 ];
	LLI* prev;

	printf("Add %s = %s\n", name, value );
	printf( "Size: %ld\n", total_size );

	while( l->current ) {
		printf( "Current ponter: %p\n", l->current );
		printf( "Contents: %s\n", strncpy( foo, l->current, 100 ) );
		prev = l->current;
		l->current = l->current->next;
	}

	l->current = (LLI*)malloc( sizeof( LLI ) );
	l->current->next = NULL;

	if ( NULL != prev ) {
		prev->next = l->current;
		printf( "Prev value: %s\n", prev->name );
	}
	// printf( "Allocated %ld bytes for structure, address: %p\n", sizeof( LLI ), l->current );
	// printf( "Contents: %s\n", strncpy( foo, l->current, 100 ) );
	total_size += sizeof( LLI );

	size = strlen( name );
	l->current->name = (char*)malloc( size + 1 );
	// printf( "Allocated %ld bytes for name, address: %p\n", size + 1 , l->current->name );
	total_size += size + 1;
	strncpy( l->current->name, name, size );

	size = strlen( value );
	l->current->value = (char*)malloc( size + 1 );
	// printf( "Allocated %ld bytes for value, address: %p\n", size + 1 , l->current->value );
	total_size += size + 1;
	strncpy( l->current->value, value, size );

	if ( NULL == l->first ) {
		printf( "First to current" );
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
	printf( "Print\n" );
	l->current = l->first;

	while( l->current ) {
		printf( "Current structure to print: %s = %s\n", l->current->name, l->current->value );
		l->current = l->current->next;
	}

	return 0;
}

LL* init_llist() {
	LL* t = malloc( sizeof( LL ) );
	t->add = &add;
	t->get = &get;
	t->print = &print;
	t->current = NULL;
	t->first = NULL;

	return t;
}
/**
 * Contains custom structure types
 */
#include "structure.h"

static size_t total_size = 0;

int add( const char *name, char *value, llist* l ) {
	size_t size;
	char foo[ 100 ];
	llist_item* prev = NULL;

	printf("Add %s = %s\n", name, value );
	printf( "Size: %ld\n", total_size );

	while( l->current ) {
		// printf( "Current ponter: %p\n", l->current );
		// printf( "Contents: %s\n", strncpy( foo, l->current, 100 ) );
		prev = l->current;
		l->current = l->current->next;
	}

	l->current = (struct llist_item*)malloc( sizeof( struct llist_item ) );
	l->current->next = NULL;

	if ( NULL != prev ) {
		prev->next = l->current;
	}
	// printf( "Allocated %ld bytes for structure, address: %p\n", sizeof( LLI ), l->current );
	// printf( "Contents: %s\n", strncpy( foo, l->current, 100 ) );
	total_size += sizeof( llist_item );

	size = strlen( name );
	l->current->name = (char*)malloc( size + 1 );
	// printf( "Allocated %ld bytes for name, address: %p\n", size + 1 , l->current->name );
	total_size += size + 1;
	strncpy( l->current->name, name, size + 1 );

	if ( l->current->name[ strlen( l->current->name ) - 1 ] != '\0' ) {
		strcat( l->current->name, "\0" );
	}

	size = strlen( value );
	l->current->value = (char*)malloc( size + 1 );
	// printf( "Allocated %ld bytes for value, address: %p\n", size + 1 , l->current->value );
	total_size += size + 1;
	strncpy( l->current->value, value, size + 1 );

	if ( l->current->value[ strlen( l->current->value) - 1 ] != '\0' ) {
		strcat( l->current->value, "\0" );
	}

	if ( NULL == l->first ) {
		printf( "First to current" );
		l->first = l->current;
	}

	printf( "Exit add\n" );

	return 0;
}

int get( char *name, char** value, struct llist* l ) {
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

int print( struct llist* l ) {
	printf( "Print\n" );
	l->current = l->first;

	while( l->current ) {
		printf( "Current structure to print: %s = %s\n", l->current->name, l->current->value );
		l->current = l->current->next;
	}

	return 0;
}

llist* init_llist() {
	struct llist* t = malloc( sizeof( llist ) );
	t->add = &add;
	t->get = &get;
	t->print = &print;
	t->current = NULL;
	t->first = NULL;

	return t;
}
/**
 * Contains custom structure types
 */
#include "structure.h"

static size_t total_size = 0;

int add( const char *name, char *value, struct llist* l ) {
	size_t size;
	char foo[ 100 ];
	struct llist_item* prev = NULL;

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
	total_size += sizeof( struct llist_item );

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
		// printf( "First to current" );
		l->first = l->current;
	}

	// printf( "Exit add\n" );

	return 0;
}

int get( char *name, char** value, struct llist* l ) {
	if ( !l->first ) {
		print_error( "Structure::Get: Failed to rewind structure - pointer to the first element is empty" );
	}

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
	if ( !l->first ) {
		print_error( "Structure::Print: Failed to rewind structure - pointer to the first element is empty" );
	}

	l->current = l->first;
	printf( "Print\n" );

	while( l->current ) {
		printf( "Current structure to print: %s = %s\n", l->current->name, l->current->value );
		l->current = l->current->next;
	}

	return 0;
}

/*
 * Remove element from the list and rewind the list
 * name - value name
 * llist - list structure
 * Returns 0 is OK, 1 if specified name is not present in the list
 */
int remove( char *name, struct llist* l ) {
	if ( !l->first ) {
		print_error( "Structure::Remove: Failed to rewind structure - pointer to the first element is empty" );
	}

	l->current = l->first;
	struct llist prev = NULL;
	int found = 0;

	while ( l->current->name ) {
		if ( !strcmp( l->current->name, name ) ) {
			if ( prev ) {
				prev->next = l->curent->next;
				free( l->current );
				l->current = l->first;

			} else {
				l->first = l->current->next;
				free( l->current );
			}
			
			return 0;
		}

		prev = l->current;
		l->current = l->current->next;
	}

	return 1;
}

struct llist* init_llist() {
	struct llist* t = malloc( sizeof( struct llist ) );
	t->add = &add;
	t->get = &get;
	t->print = &print;
	t->remove = &remove;
	t->current = NULL;
	t->first = NULL;

	return t;
}
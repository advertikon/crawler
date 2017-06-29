/**
 * Contains custom structure types
 */
#include "structure.h"

static size_t total_size = 0;

static int s_add( const char *name, const char *value, struct llist* l ) {
	size_t size;
	char foo[ 100 ];
	struct llist_item* prev = NULL;
	int max_index = 0;

	if ( s_get( name, NULL, l ) == 0 ) {
		fprintf( stderr, "Failed to add item to list: name '%s' already exists", name );
		return 1;
	}

	// printf("Add %s = %s\n", name, value );
	// printf( "Size: %ld\n", total_size );

	while( l->current ) {
		// printf( "Current ponter: %p\n", l->current );
		// printf( "Contents: %s\n", strncpy( foo, l->current, 100 ) );
		prev = l->current;
		l->current = l->current->next;
	}

	l->current = (struct llist_item*)malloc( sizeof( struct llist_item ) );

	if ( NULL == l->current ) {
		print_error( "Failed to allocate memory for list structure" );
	}

	l->size += sizeof( struct llist_item );
	l->current->next = NULL;

	// Not the first element
	if ( NULL != prev ) {
		prev->next = l->current;
		l->current->index = ++prev->index;

	// First element in the list
	} else {
		l->current->index = 0;
	}
	// printf( "Allocated %ld bytes for structure, address: %p\n", sizeof( LLI ), l->current );
	// printf( "Contents: %s\n", strncpy( foo, l->current, 100 ) );

	if ( NULL == name ) {
		// printf( "Index: %d\n", l->current->index );
		sprintf( foo, "%d", l->current->index );
		size = strlen( foo ) + 1;
		l->current->name = (char*)malloc( size );

		if ( NULL == l->current->name ) {
			print_error( "Failed allocate memory for list name" );
		}

		strncpy( l->current->name, foo, size );

	} else {
		size = strlen( name ) + 1;
		l->current->name = (char*)malloc( size );

		if ( NULL == l->current->name ) {
			print_error( "Failed allocate memory for list name" );
		}

		strncpy( l->current->name, name, size );
	}
	
	// printf("there\n");
	// printf( "Allocated %ld bytes for name, address: %p\n", size + 1 , l->current->name );
	l->size += size;

	// if ( l->current->name[ strlen( l->current->name ) - 1 ] != '\0' ) {
	// 	strcat( l->current->name, "\0" );
	// }

	size = strlen( value ) + 1;
	l->current->value = (char*)malloc( size );

	if ( NULL == l->current->value ) {
		print_error( "Failed allocate memory for list value" );
	}

	// printf( "Allocated %ld bytes for value, address: %p\n", size + 1 , l->current->value );
	l->size += size;
	strncpy( l->current->value, value, size );

	// if ( l->current->value[ strlen( l->current->value) - 1 ] != '\0' ) {
	// 	strcat( l->current->value, "\0" );
	// }

	if ( NULL == l->first ) {
		// printf( "First to current" );
		l->first = l->current;
	}

	// printf( "Exit add\n" );

	return 0;
}

static int s_get( const char *name, char** value, struct llist* l ) {
	int found = 0;

	if ( NULL == name ) {
		return 1;
	}

	if ( NULL == l ) {
		return 1;
	}

	if ( NULL == l->first ) {
		return 1;
	}

	l->current = l->first;

	while ( l->current && l->current->name ) {
		if ( 0 == strcmp( l->current->name, name ) ) {
			found = 1;
			break;
		}

		l->current = l->current->next;
	}

	if ( found && NULL != value ) {
		*value = l->current->value;
	}

	l->current = l->first;

	return found ? 0 : 1;
}

/**
 * Checks if specific value exists in the structure
 * value - value to be searched
 * l - llist structure to search in
 * Returns 0 is element exists, 1 - otherwise
 */
static int s_has_value( const char *value, struct llist* l ) {
	int found = 0;

	if ( NULL == value || NULL == l || NULL == l->first ) {
		return 1;
	}

	l->current = l->first;

	while ( l->current && l->current->name ) {
		if ( !strcmp( l->current->value, value ) ) {
			found = 1;
			break;
		}

		l->current = l->current->next;
	}

	// if ( found ) {
	// 	*value = l->current->value;
	// }

	l->current = l->first;

	return found ? 0 : 1;
}

static int s_print( struct llist* l ) {
	if ( NULL == l->first ) {
		return 1;
	}

	l->current = l->first;

	while( l->current ) {
		printf( "%s = %s\n", l->current->name, l->current->value );
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
static int s_remove( const char *name, struct llist* l ) {
	if ( NULL == l->first ) {
		fprintf( stderr, "Structure::Remove: List is empty" );
		return 0;
	}

	l->current = l->first;
	struct llist_item *prev = NULL;
	int found = 0;

	while ( l->current && l->current->name ) {
		if ( 0 == strcmp( l->current->name, name ) ) {
			if ( NULL != prev ) {
				prev->next = l->current->next;
				l->size -= L_MEM( l->current );
				free( l->current );
				
			} else {
				l->first = l->current->next;
				l->size -= L_MEM( l->current );
				free( l->current );
			}

			l->current = l->first;

			return 0;
		}

		prev = l->current;
		l->current = l->current->next;
	}

	return 1;
}

/**
 * Adds unique records from structure 'two' to structure 'one'
 */
static int s_merge( struct llist *two, struct llist *one ) {
	if ( two && two->first ) {
		two->current = two->first;

		do {

			// Merge only unique values
			if ( 0 == s_has_value( two->current->value, one ) ) {
				continue; 
			}

			// If index (name) already exists - use numeric sequential index
			if ( 0 == one->get( two->current->name, NULL, one ) ) {
				one->add( NULL, two->current->value, one );

			} else {
				one->add( two->current->name, two->current->value, one );
			}
			
		} while ( two->current = two->current->next );

		one->current = one->first;
		two->current = two->first;
	}

	return 0;
}

static int s_empty( struct llist *l ) {
	struct llist_item* c;

	if ( l->first ) {
		l->current = l->first;

		while ( l->current ) {
			c = l->current;
			l->current = l->current->next;
			l->size -= L_MEM( c );
			free( c );
		}

		l->first = NULL;
		l->current = NULL;
	}

	return 0;
}

/**
 * Calculates list length
 * l - target list
 * Returns number of list items
 */
static int s_count( struct llist* l ) {
	int c = 0;

	if ( NULL != l && NULL != l->first ) {
		l->current = l->first;

		do {
			c++;
		} while ( NULL != ( l->current = l->current->next ) );

		l->current = l->first;
	}

	return c;
}

struct llist* init_llist() {
	struct llist* t = malloc( sizeof( struct llist ) );
	t->add = &s_add;
	t->get = &s_get;
	t->has_value = &s_has_value;
	t->print = &s_print;
	t->remove = &s_remove;
	t->merge = &s_merge;
	t->empty = &s_empty;
	t->count = &s_count;
	t->current = NULL;
	t->first = NULL;
	t->size = sizeof( struct llist );

	return t;
}
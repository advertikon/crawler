/**
 * Contains custom structure types
 */
#include "structure.h"

static size_t total_size = 0;

static int s_add( const char *name, const char *value, struct llist* l ) {
	int debug = 0;

	if ( debug ) fprintf( stderr, "Adding string '%s' to a list\n", value );

	char *t_val;
	size_t size;
	int status;
	
	size = strlen( value ) + 1;
	t_val = malloc( size );

	if ( NULL == t_val ) {
		print_error( "Failed allocate memory for list value" );
	}

	strcpy( t_val, value );

	if ( debug ) fprintf( stderr, "Copy value '%s' into temp variable\n", value );

	if ( 0 != ( status = s_addp( name, t_val, l ) ) ) {
		return status;
	}

	l->current->is_string = 1;
	l->size += size;

	return 0;
}

static int s_addp( const char *name, void *value, struct llist* l  ) {
	int debug = 0;

	if ( debug ) fprintf( stderr, "Adding item '%s' - '%s' to a list\n", name, (char*)value );

	size_t size;
	char foo[ 100 ];
	struct llist_item* prev = NULL;
	int max_index = 0;

	if ( s_get( name, NULL, l ) == 0 ) {
		fprintf( stderr, "Failed to add item to list: name '%s' already exists\n", name );
		return 1;
	}

	while( l->current ) {
		prev = l->current;
		l->current = l->current->next;
	}

	l->current = init_llist_item();

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

	if ( NULL == name ) {
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

	l->size += size;
	l->current->value = value;

	if ( NULL == l->first ) {
		l->first = l->current;
	}

	if ( debug ) fprintf( stderr, "Item has been added successfully\n" );

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

static char *s_fetch( const char *name, struct llist* l ) {
	int debug = 0;

	int found = 0;
	int len;
	char *ret;

	if ( debug ) fprintf( stderr, "Fetching value with name '%s'\n", name );

	if ( NULL == name || NULL == l || NULL == l->first ) {
		if ( debug ) fprintf( stderr, "Empty result\n" );

		return NULL;
	}

	l->current = l->first;

	while ( l->current ) {
		if ( 0 == strcmp( l->current->name, name ) ) {
			found = 1;

			if ( debug ) fprintf( stderr, "Found value '%s'\n", l->current->as_string( l->current ) );

			break;
		}

		l->current = l->current->next;
	}

	if ( found && NULL != l->current->value ) {
		len = strlen( l->current->value ) + 1;
		ret = (char*)malloc( len );

		if ( NULL == ret ) {
			print_error( "llist::fetch: failed to allocate memory for value" );
		} 

		memset( ret, '\0', len );
		strcpy( ret, l->current->value );
		l->current = l->first;

		return ret;
	}

	l->current = l->first;

	if ( debug ) fprintf( stderr, "Nothing found\n" );

	return NULL;
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

	while ( l->current && l->current->name && l->current->is_string ) {
		if ( !strcmp( l->current->value, value ) ) {
			found = 1;
			break;
		}

		l->current = l->current->next;
	}

	l->current = l->first;

	return found ? 0 : 1;
}

static int s_print( struct llist* l ) {
	if ( NULL == l->first ) {
		return 1;
	}

	l->current = l->first;

	while( l->current ) {
		printf( "%s = %s\n", l->current->name, (char*)l->current->value );
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
				
			} else {
				l->first = l->current->next;
			}

			if ( l->current->is_string ) {
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
	int debug = 0;

	if ( debug ) fprintf( stderr, "Emptying list\n" );

	struct llist_item* c;

	if ( l->first ) {
		l->current = l->first;

		while ( l->current ) {
			if ( debug ) fprintf( stderr, "Emptying item '%s' - '%s'\n", l->current->name, (char*)l->current->value );
			c = l->current;
			l->current = l->current->next;

			if ( c->is_string ) {
				if ( debug ) fprintf( stderr, "Is string item - free memory\n" );
				l->size -= L_MEM( c );
				free( c );
			}
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

static char* l_as_string( struct llist_item* li ) {
	return li->is_string ? li->value : "(pointer)";
}

struct llist_item* init_llist_item() {
	struct llist_item *li;
	li = (struct llist_item*)malloc( sizeof( struct llist_item ) );
	li->name = NULL;
	li->value = NULL;
	li->next = NULL;
	li->index = 0;
	li->is_string = 0;
	li->as_string = l_as_string;

	return li;
}

struct llist* init_llist() {
	int debug = 0;

	struct llist *t;

	if ( debug ) fprintf( stderr, "List initialization start\n" );

	t = (struct llist*)malloc( sizeof( struct llist ) );
	t->add = s_add;
	t->addp = s_addp;
	t->get = s_get;
	t->fetch = s_fetch;
	t->has_value = s_has_value;
	t->print = s_print;
	t->remove = s_remove;
	t->merge = s_merge;
	t->empty = s_empty;
	t->count = s_count;
	t->current = NULL;
	t->first = NULL;
	t->size = sizeof( struct llist );

	if ( debug ) fprintf( stderr, "List initialization end\n" );

	return t;

}
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

int usage( void );
int parse_args( char ** );
int print_error( char *, ... );

struct list {
	char *name;
	char *value;
	struct list *next;
};

int add_list_item( char*, char*, struct list* );
int get_list_item( char*, char**, struct list* );
int print_args( struct list* );

int add_list_item( char *name, char *value, struct list *this ) {
	struct list *last;
	last = this;

	while( last->name ) {
		last = last->next;
	}

	last->next = malloc( sizeof( struct list ) );
	last->name = malloc( strlen( name ) );
	strcpy( last->name, name );
	last->value = malloc( strlen( value ) );
	strcpy( last->value, value );

	return 0;
}

int get_list_item( char *name, char** value, struct list *this ) {
	struct list *current;
	current = this;
	int found = 0;

	do {
		if ( !strcmp( current->name, name ) ) {
			found = 1;
			break;
		}

		current = current->next;

	} while ( current->name );

	if ( found ) {
		*value = current->value;
	}

	return found ? 0 : 1;
}

int print_args( struct list* l ) {
	struct list *ll;
	ll = l;

	while( ll->name ) {
		printf( "Current structure to print: %s = %s\n", ll->name, ll->value );
		ll = ll->next;
	}

	return 0;
}

char *e_mess = "";
char *name = "Crawler";
struct list a_list;

int main( int argc, char **argv ) {
	char* value;

	parse_args( argv );
	print_args( &a_list );

	if ( 0 == get_list_item( "bb", &value, &a_list ) ) {
		printf( "bb = %s\n", value );

	} else {
		printf( "bb has no value\n" );
	}

	if ( 0 == get_list_item( "s", &value, &a_list ) ) {
		printf( "s = %s\n", value );

	} else {
		printf( "s has no value\n" );
	}

	return 0;
}

int usage() {
	printf("%s\n", "" );
	exit( 0 );
}

int parse_args( char **args ) {
	char *p1;
	char p2;
	char a_name[100];
	char a_value[100];
	int in_arg = 0;
	int in_long_arg = 0;
	int in_value = 0;
	int has_name = 0;
	int has_value = 0;
	int last_arg = 0;

	a_name[ 0 ] = '\n';

	args++; /* Skip program invocation name */

	while( p1 = *args++ ) {
		while( p2 = *p1++ ) {

			if ( '-' == p2 ) {
				if ( in_arg ) {
					in_long_arg = 1;

				} else {
					in_arg = 1;
				}

				if ( has_name ) {
					if ( last_arg >= 2 ) {
						print_error( "Option %s has no value", a_name );
					}

					if ( last_arg ) {
						add_list_item( a_name, "1", &a_list );
						has_name = 0;
						has_value = 0;
					}
				}

			} else if ( '"' == p2 || '\'' == p2 ) {
				// skip

			} else {
				if ( in_arg ) {
					if ( in_value ) {
						if ( !has_name ) {
							print_error( "Undefined argument %s", p1 - 1 );
						}

						add_list_item( a_name, "1", &a_list );
						has_value = 0;
						a_name[ 0 ] = p2;
						has_name = 1;
					}

					if ( in_long_arg ) {
						strcpy( a_name, p1 - 1 );
						has_name = 1;
						break;

					} else {
						a_name[ 0 ] = p2;
						a_name[ 1 ] = '\0';
						has_name = 1;
						in_value = 1;
					}

				} else {
					if ( !has_name  ) {
						print_error( "Undefined argument %s", p1 - 1 );
					}

					add_list_item( a_name, strcpy( a_value, p1 - 1 ), &a_list );
					has_name = 0;
					has_value = 0;
					break;
				}
			}
		}

		last_arg = in_arg + in_long_arg * 2;

		in_value = 0;
		in_arg = 0;
		in_long_arg = 0;
	}

	if ( has_name ) {
		if ( last_arg >= 2 ) {
			print_error( "Option %s has no value", a_name );
		}

		if ( last_arg ) {
			add_list_item( a_name, "1", &a_list );
		}
	}
}

int print_error( char * msg, ... ) {
	va_list args;

	va_start( args, msg );
	fprintf( stderr, "%s: ", name );
	vfprintf( stderr, msg, args );
	fputc( '\n', stderr );
	va_end( args );
	
	exit( 1 );
}

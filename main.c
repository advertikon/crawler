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
int get_list_item( char*, char*, struct list* );
int print_args( struct list* );

int add_list_item( char *name, char *value, struct list *this ) {
	printf( "Add to structure %s = %s\n", name, value );
	struct list *last;
	last = this;

	while( last->next ) {
		last = last->next;
	}

	last->next = malloc( sizeof( struct list ) );
	last->next->name = name;
	last->next->value = value;

	return 0;
}

int get_list_item( char *name, char *value, struct list *this ) {
	struct list *current;
	current = this;
	int found = 0;

	while ( current->next ) {
		if ( !strcmp( current->name, name ) ) {
			found = 1;
			break;
		}

		current = current->next;
	}

	if ( found ) {
		value = current->value;
	}

	return found ? 0 : 1;

}

// struct list {
// 	struct list_item first;
// 	int( *add )( char*, char*, struct list );
// 	int( *get )( char*, char* );
// };

char *e_mess = "";
char *name = "Crawler";
struct list a_list;

int main( int argc, char **argv ) {

	parse_args( argv );
	print_args( &a_list );

	return 0;
}

int usage() {
	printf("%s\n", "" );
	exit( 0 );
}

int parse_args( char **args ) {
	char *p1;
	char p2;
	char *a_name = NULL;
	char *a_val = NULL;
	int in_arg = 0;
	int in_long_arg = 0;
	int in_name = 0;
	int in_value = 0;

	args++; /* Skip program invocation name */

	while( p1 = *args++ ) {
		printf( "New arg\n" );

		while( p2 = *p1++ ) {
			printf( "New char\n" );

			if ( '-' == p2 ) {

				printf( "Dash\n" );

				if ( in_arg ) {
					printf( "Dash-dash\n" );

					in_long_arg = 1;

				} else {
					in_arg = 1;
				}

			} else if ( '"' == p2 || '\'' == p2 ) {

				// skip

			} else {
				printf( "Alpha: %c\n", p2 );

				if ( in_arg ) {
					printf( "In argument\n" );

					if ( in_value ) {
						printf( "In value\n" );

						if ( !a_name ) {
							print_error( "Undefined argument %s", p1 - 1 );
						}

						add_list_item( a_name, "1", &a_list );
						a_name = &p2;
						a_name[ 1 ] = '\0';
						printf( "Set name to: %s\n", a_name );
					}

					if ( in_long_arg ) {
						printf( "In long argument\n" );

						a_name = &p2;
						printf( "Set name to: %s\n", a_name );
						break;

					} else {
						printf( "In short argument\n" );

						a_name = &p2;
						a_name[ 1 ] = '\0';
						printf( "Set name to: %s\n", a_name );
						in_value = 1;
					}

				} else {
					printf( "Value part\n" );

					if ( !a_name  ) {
						print_error( "Undefined argument %s", p1 - 1 );
					}

					add_list_item( a_name, &p2, &a_list );
					a_name = NULL;
					printf( "Set name to: %s\n", "NULL" );
					break;
				}
			}
		}

		in_value = 0;
		in_arg = 0;
		in_long_arg = 0;
		
		printf("\n" );
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

int print_args( struct list* l ) {
	printf( "%s = %s\n", l->name, l->value );

	while( l->next ) {
		l = l->next;
		printf("%s = %s\n", l->name, l->value );
	}

	return 0;
}

/**
 * Parses arguments passed into script
 * To ini facility run parsre_args(void) - arguments vector need to be named "argv"
 * To add argument call add_arg
 * To get argument call get_arg - returns 1 if argument doesnt exist
 */

#include "args.h"

static ALIST a_list;

int add_arg( char *name, char *value ) {
	printf("Add %s = %s\n", name, value );
	ALIST* last;
	last = &a_list;

	while( last->name ) {
		last = last->next;
	}

	last->next = malloc( sizeof( ALIST ) );
	last->name = malloc( strlen( name ) );
	strcpy( last->name, name );
	last->value = malloc( strlen( value ) );
	strcpy( last->value, value );

	return 0;
}

int get_arg( char *name, char** value ) {
	ALIST* current;
	current = &a_list;
	int found = 0;

	while ( current->name ) {
		if ( !strcmp( current->name, name ) ) {
			found = 1;
			break;
		}

		current = current->next;
	}

	if ( found ) {
		*value = current->value;
	}

	return found ? 0 : 1;
}

int print_args() {
	ALIST* ll;
	ll = &a_list;

	while( ll->name ) {
		printf( "Current structure to print: %s = %s\n", ll->name, ll->value );
		ll = ll->next;
	}

	return 0;
}

int parse_args( char** args ) {
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
						add_arg( a_name, "1" );
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

						add_arg( a_name, "1" );
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

					add_arg( a_name, strcpy( a_value, p1 - 1 ) );
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
			add_arg( a_name, "1" );
		}
	}
}
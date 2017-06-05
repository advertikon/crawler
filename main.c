#include "header.h"

int usage( void );

int print_error( char *, ... );
char *e_mess = "";
char *name = "Crawler";

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



int print_error( char * msg, ... ) {
	va_list args;

	va_start( args, msg );
	fprintf( stderr, "%s: ", name );
	vfprintf( stderr, msg, args );
	fputc( '\n', stderr );
	va_end( args );
	
	exit( 1 );
}

#include "header.h"

int usage( void );

char *name = "Crawler";

int main( int argc, char **argv ) {
	char* value;

	parse_args( argv );
	print_args();

	if ( 0 == get_arg( "bb", &value ) ) {
		printf( "bb = %s\n", value );

	} else {
		printf( "bb has no value\n" );
	}

	if ( 0 == get_arg( "s", &value ) ) {
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

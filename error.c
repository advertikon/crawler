/**
 * Error handling functionallity
 */
#include "error.h"

int print_error( char * msg, ... ) {
	va_list args;
	extern char* mod_name;

	va_start( args, msg );
	fprintf( stderr, "%s: ", mod_name );
	vfprintf( stderr, msg, args );
	fputc( '\n', stderr );
	va_end( args );
	
	exit( 1 );
}
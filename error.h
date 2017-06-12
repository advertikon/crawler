/**
 * Error handling functionallity
 *
 * Requires:
 * <stdarg.h>
 * <stdio.h>
 */

int print_error( char *, ... );

int print_error( char * msg, ... ) {
	va_list args;
	extern char* name;

	va_start( args, msg );
	fprintf( stderr, "%s: ", name );
	vfprintf( stderr, msg, args );
	fputc( '\n', stderr );
	va_end( args );
	
	exit( 1 );
}
/**
 *
 *
 */

#include "path.h"
#include "error.h"

size_t get_path_max_size() {
	char* ptr;
	size_t size;

	if ( posix_version == 0 ) {
		posix_version = sysconf(_SC_VERSION);
	}

	if ( xsi_version == 0 ) {
		xsi_version = sysconf( _SC_XOPEN_VERSION );
	}

	if ( pathmax == 0 ) {

		/* first time through */
		errno = 0;

		if ( ( pathmax = pathconf( "/", _PC_PATH_MAX ) ) < 0 ) {
			if ( errno == 0 ) {
				pathmax = PATH_MAX_GUESS;

			} else {
				perror( "pathconf error for _PC_PATH_MAX" );
				return 0;
			}

		} else {

			/* add one since it’s relative to root */
			pathmax++;
		}
	}

	/*
	* Before POSIX.1-2001, we aren’t guaranteed that PATH_MAX includes
	* the terminating null byte. Same goes for XPG3.
	*/
	if ( ( posix_version < 200112L ) && ( xsi_version < 4 ) ) {
		size = pathmax + 1;

	} else {
		size = pathmax;
	}

	return size;
}

/**
 * Glob wrapper
 */
glob_t* Glob( const char *restrict pattern ) {
	int status;
	glob_t *pglob = (glob_t*)g_malloc( sizeof( glob_t ) );

	status = glob( pattern, 0, NULL, pglob );

	if ( status != 0 && status != GLOB_NOMATCH ) {
		fprintf( stderr, "GLOB error\n" );
		g_free( pglob );

		return NULL;
	}

	return pglob;
}

GSList* Scandir( char *dirname ) {
	struct dirent **namelist;
	int n;
	GSList *list = NULL;

	n = scandir( dirname, &namelist, NULL, alphasort );

	if (n == -1) {
		fprintf( stderr, "Failed to scan directory %s: %s", dirname, strerror( errno ) );
		return NULL;
	}

	while ( n-- ) {
		printf( "%s\n", namelist[ n ]->d_name  );
		list = g_slist_append( list, namelist[ n ]->d_name );
		free( namelist[ n ] );
	}

	free( namelist );

	return list;
}

/**
 * Dumps textual contents of GSList
 */
void dump_slist( GSList *list ) {
	GSList *current = list;
	printf( "\nGSList contents (%p):\n", current );

	if ( NULL == list ) return;

	do {
		printf( "[%p]->data[%p]%s\n", current, current->data, (char*)current->data );
		current = current->next;

	} while ( current );

	printf( "\n" );
}

/**
 * Dumps string
 */
void dump_string( char *string ) {
	char *p = string;
	int i = 0;
	int max = 1000;
	char
		*s_str = g_malloc0( max ),
		*h_str = g_malloc0( max ),
		*i_str = g_malloc0( max ),
		// *p_str = g_malloc0( max ),
		*temp = g_malloc0( max );

	strcpy( s_str, "str: " );
	strcpy( i_str, "int: " );
	strcpy( h_str, "hex: " );

	printf( "Dumping the string:\n" );
	printf( "%s\n", p );

	while( *p != '\0' ) {
		sprintf( temp, "%11c", *p );
		strncat( s_str, temp, strlen( temp ) );
		memset( temp, 0, max );

		sprintf( temp, "%11x", *p );
		strncat( h_str, temp, strlen( temp ) );
		memset( temp, 0, max );

		sprintf( temp, "%11u", *p ); 
		strncat( i_str, temp, strlen( temp ) );
		memset( temp, 0, max );

		// sprintf( temp, "%11p", p );
		// strncat( p_str, temp, strlen( temp ) );
		// memset( temp, 0, max );

		p++;
		if ( i++ > 30 )break;
	}

	printf( "%s\n", s_str );
	printf( "%s\n", h_str );
	printf( "%s\n", i_str );
	// printf( "%s\n", p_str );

	g_free( s_str );
	g_free( h_str );
	g_free( i_str );
	// g_free( p_str );
	g_free( temp );
}

/**
 * Dumps hash-list data
 */
void dump_hash( gpointer key, gpointer value, gpointer data ) {
	GSList *next;

	printf( "Hash contents:\n" );
	printf( "%s:\n", (gchar*)key );

	next = (GSList*)value;

	while( next ) {
		printf( "\t%s\n", (gchar*)next->data );
		next = next->next;
	}

	printf( "\n" );
}

/**
 * Dumps NULL terminated string vector
 */
void dump_vector( char** vector ) {
	char **p = vector;

	g_print( "Vector dump:\n" );

	while ( NULL != *p ) {
		g_print( "%s\n", *p );
		p++;
	}
}

/**
 * Prints colored output to stdout
 */
void print_color( const char *color, char *format, ... ) {
	va_list args;
	va_start( args, format );
	printf( color );
	vprintf( format, args );
	printf( "\e[0m\n" );
	va_end( args );
}
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
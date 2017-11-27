/**
 *
 *
 */

#include "path.h"

static GHashTable *stat_cache = NULL;
int is_stat_cache_inited = 0;

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
	printf( "\nGSList contents [%i] (%p):\n", g_slist_length( current ), current );

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

/**
 * POSIX unlink wrapper
 */
int Unlink( char *name ) {
	int status;

	if ( 0 == ( status = unlink( name ) ) ) {
		return 0;
	}

	fprintf( stderr, "Failed to unlink file '%s': %s\n", name, strerror( errno ) );

	return status;
}

/**
 * POSIX lstat wrapper
 * Returns pointer to stat buffer. Need to be freed
 */
struct stat *Lstat( char *path ) {
	struct stat *return_buffer, *stat_buffer = _Lstat( path );

	if ( NULL == stat_buffer ) return NULL;

	return_buffer = g_memdup( stat_buffer, sizeof( struct stat ) );

	return return_buffer;
}

/**
 * POSIX lstat wrapper
 * Returns pointer to stat buffer (data is cached)
 * For internal use since returns pointer to record in hash table (may not be freed)
 */
struct stat *_Lstat( char *path ) {
	struct stat *stat_buffer;

	// Initialize stat hash
	if ( 0 == is_stat_cache_inited ) {
		stat_cache = g_hash_table_new_full( g_str_hash, g_str_equal, (GDestroyNotify)clean_stat_cache, (GDestroyNotify)clean_stat_cache );
		is_stat_cache_inited = 1;
	}

	// Hash lookup
	if ( NULL == ( stat_buffer = g_hash_table_lookup( stat_cache, path ) ) ) {
		stat_buffer = g_malloc0( sizeof( struct stat ) );

		if ( lstat( path, stat_buffer ) < 0 ) {
			fprintf( stderr, "Failed to get statistics for '%s' in %s:%i: %s\n", path, __FILE__, __LINE__, strerror( errno ) );
			return NULL;
		}

		g_hash_table_insert( stat_cache, g_strdup( path ), stat_buffer );
	}

	return stat_buffer;
}

/**
 * Checks if path is a regular file
 */
gboolean is_dir( char *path ) {
	struct stat *stat_buffer = _Lstat( path );

	if ( NULL == stat_buffer ) return FALSE;

	return S_ISDIR( stat_buffer->st_mode );
}

/**
 * Checks if path is a directory
 */
gboolean is_file( char *path ) {
	struct stat *stat_buffer = _Lstat( path );

	if ( NULL == stat_buffer ) return FALSE;

	return S_ISREG( stat_buffer->st_mode );
}

/**
 * Callback to Clean up stat cache
 */
void clean_stat_cache( char* value ) {
	int debug = 1;

	if ( debug )printf( "Freeing stat cache value %s", (char*)value );
	g_free( value );
}

/**
 * Returns size of the file
 */
size_t filesize( char *name ) {
	struct stat *stat_buffer = _Lstat( name );

	return stat_buffer->st_size;
}

/**
 * Concatenates arbitrary strings number
 */
char *Strcat( char *str, ... ) {
	char *temp1, *temp2;
	size_t size, temp_size;
	char *ret;
	char *p;
	int realloc_multy = 1;

	va_list args;
	va_start( args, str );

	size = strlen( str );
	temp_size = size + 100;

	temp1 = g_malloc0( temp_size + 1 );
	temp2 = g_malloc( 1 );
	strncpy( temp1, str, size );
;
	while ( NULL != ( p = va_arg( args, char* ) ) ) {
		size += strlen( p );

		if ( size > temp_size ) {
			temp2 = g_realloc( temp2, temp_size + 1 );
			strncpy( temp2, temp1, temp_size );
			temp_size = size << realloc_multy++;
			temp1 = g_realloc( temp1, temp_size + 1 );
		}

		strncat( temp1, p, strlen( p ) );
	}

	ret = g_malloc( size + 1 );
	strcpy( ret, temp1 );

	va_end( args );

	g_free( temp1 );
	g_free( temp2 );

	return ret;
}

/**
 * Iterates over FS structure
 * Fires callbacks:
 * - file_c - callback on each found file. Arguments absolute file path, struct stat. If returns
 *           non-zero status function returns with status 1
 * - dir_c - callback on each found directory, fires after all callbacks for inner files
 *           Arguments: directory path (absolute, relative), struct stat. If returns non-zero
 *           function returns with status 1;
 * - err_c - callback on error. Argument: item path. If returns non-zero status - script terminates,
 *           otherwise function returns with status 1
 */
int iterate(  char* path, cb file_cb, cb dir_cb, cb err_cb ) {
	int debug = 0;

	DIR* dir = NULL;
	struct dirent* dir_entry = NULL;
	struct stat *stat_buffer = Lstat( path );
	gboolean is_error = FALSE;
	char *dir_to_iterate;

	if ( debug )fprintf( stderr, "Iterate: '%s'\n", path );

	if ( strncmp( G_DIR_SEPARATOR_S, path, 1 ) != 0 ) {
		fprintf( stderr, "Path need to be absolute (%s). In %s:%i\n", path, __FILE__, __LINE__ );
		is_error = TRUE;
		goto exit_point;
	}

	if ( NULL == stat_buffer ) {
		if ( NULL != err_cb && 0 != err_cb( path, NULL ) ) {
			exit( 1 );
		}

		is_error = TRUE;
		goto exit_point;
	}

	if ( is_file( path ) ) {
		if ( debug )fprintf( stderr, "Is file\n" );

		if ( NULL != file_cb && 0 != file_cb( path, NULL ) ) {
			is_error = TRUE;
			goto exit_point;
		}

	} else if ( is_dir( path ) ) {
		if ( debug )fprintf( stderr, "Is folder\n" );

		if ( NULL == ( dir = opendir( path ) ) ) {
			fprintf(stderr, "Failed to open folder in %s:%i:%s\n", __FILE__, __LINE__, strerror( errno ) );
			is_error = TRUE;
			goto exit_point;
		}

		if ( debug )fprintf( stderr, "Dir has been entered: %s\n", path );

		errno = 0;

		while ( NULL != ( dir_entry = readdir( dir ) ) ) {
			if ( dir_entry->d_name[ 0 ] == '.' ) {
				continue;
			}

			dir_to_iterate = g_build_filename( path, dir_entry->d_name, NULL ); /* needs to be freed in callee */

			iterate( dir_to_iterate, file_cb, dir_cb, err_cb );
		}

		// Error while reading directory
		if ( 0 != errno && NULL == dir_entry ) {
			fprintf( stderr, "Error while reading directory %s: %s in %s:%i\n", path, strerror( errno ), __FILE__, __LINE__ );
			is_error = TRUE;
			goto exit_point;
		}

		// Run DIR callback after all FILE callbacks
		if ( NULL != dir_cb && 0 != dir_cb( path, NULL ) ) {
			return 1;
		}

		closedir( dir );

	} else {
		fprintf( stderr, "%s is not a file nor a directory\n", path );
		is_error = TRUE;
		goto exit_point;
	}

exit_point:
	g_free( path );
	g_free( stat_buffer );

	if ( is_error ) {
		return 1;
	}

	return 0;
}

/**
 * Reliable signal function from APU (restarts all interrupted system calls but SIGALARM)
 */
Sigfunc *signal( int signo, Sigfunc *func ) {
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset( &act.sa_mask );
	act.sa_flags = 0;

	if ( signo == SIGALRM ) {
		#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;
		#endif

	} else {
		act.sa_flags |= SA_RESTART;
	}

	if ( sigaction( signo, &act, &oact ) < 0 ) {
		return( SIG_ERR );
	}

	return( oact.sa_handler );
}
#include "header.h"

int usage( void );
int iterate( const char* );

#define MAX_LINE 200

char* name = "Crawler";
char* code;
char* cur_input = NULL;
char* dot = "/var/www/html/oc";
char* config_name = ".crawler";
// char* item_name;
static int depth = 0;
size_t path_max_size;

struct llist* files;
struct stat stat_buffer;

int main( int argc, char **argv ) {
	char line[ MAX_LINE ];
	char* foo = path_alloc( &path_max_size );

	files = init_llist();

	parse_args( argv );
	print_args();

	code = (char*)malloc( MAX_LINE );
	get_arg( "code", &code );

	while ( 1 ) {
		if ( strlen( code ) == 0 ) {
			printf( "Specify module code\n" );
			cur_input = code;
		}

		if( fgets( line, MAX_LINE, stdin ) == NULL && ferror( stdin ) ) {
			perror( "Reading line error" );
			return 1;
		}

		if ( strcmp( line, "exit\n" ) == 0 ) {
			printf( "Exiting\n" );
			return 0;
		}

		if ( cur_input ) {
			strcpy( cur_input, line );
			cur_input = NULL;
		}

		printf("%s", line );

		if ( strlen( code ) == 0 ) {
			print_error( "Module code is missing" );
			return 1;
		}

		if( iterate( dot ) > 0 ) {
			return 1;
		}
	}

	files->print( files );

	return 0;
}

int usage() {
	printf("%.20s - %s\n", "exit", "exit function" );
	exit( 0 );
}

int iterate( const char* path ) {
	DIR* dir;
	struct dirent* item;
	char item_name[ path_max_size ];

	printf( "Depth: %d ", depth );
	printf( "Iterate over: %s\n", path );

	if ( lstat( path, &stat_buffer ) < 0 ) {
		return 1;
	}

	if ( S_ISREG( stat_buffer.st_mode ) ) {
		printf( "Is file\n" );
		files->add( path, "1", files );

	} else if ( S_ISDIR( stat_buffer.st_mode ) ) {
		printf( "Is DIR\n" );
		printf( "Path: %s\n", path );
		if ( NULL == ( dir = opendir( path ) ) ) {
			perror( "Opendir error" );
			return 1;
		}

		depth++;

		printf( "Open DIR\n" );

		while ( NULL != ( item = readdir( dir ) ) ) {
			if ( item->d_name[ 0 ] == '.' ) {
				continue;
			}

			strcpy( item_name, path );
			strcat( item_name, "/" );
			strcat( item_name, item->d_name );

			iterate( item_name );
		}

		closedir( dir );
		depth--;
	}
		
}

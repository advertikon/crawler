#include "header.h"

int usage( void );
int iterate( char* );

#define MAX_LINE 100

char* name = "Crawler";
char* code;
char* cur_input = NULL;
char* dot = "/var/www/html/oc";
char* config_name = ".crawler";

struct stat stat_buffer;

int main( int argc, char **argv ) {
	char line[ MAX_LINE ];

	parse_args( argv );
	print_args();

	code = ( char* )malloc( MAX_LINE );
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

	return 0;
}

int usage() {
	printf("%.20s - %s\n", "exit", "exit function" );
	exit( 0 );
}

int iterate( char* path ) {
	DIR* dir;
	struct dirent* item;
	char item_name[ MAX_LINE ];

	printf( "Iterate over: %s\n", path );

	if ( lstat( path, &stat_buffer ) < 0 ) {
		perror( "Lstat error" );
		return 1;
	}

	if ( S_ISREG( stat_buffer.st_mode ) ) {
		printf( "%s\n", path );

	} else if ( S_ISDIR( stat_buffer.st_mode ) ) {
		if ( NULL == ( dir = opendir( path ) ) ) {
			perror( "Opendir error" );
			return 1;
		}

		while ( NULL != ( item = readdir( dir ) ) ) {
			if ( strcmp( item->d_name, "." ) == 0 || strcmp( item->d_name, ".." ) == 0 ) {
				continue;
			}

			strcpy( item_name, path );
			strcat( item_name, "/" );
			strcat( item_name, item->d_name );

			iterate( item_name );
		}

		closedir( dir );
	}
}

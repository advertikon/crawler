#include "header.h"

char* mod_name = "Crawler";
char* code;
char* cur_input = NULL;
char* dot = "/var/www/html/oc";
char* config_name = ".crawler";
static int depth = 0;
size_t path_max_size;

struct llist* files, include_dir, exclude_dir, *lconfig;

char* cwd;

int main( int argc, char **argv ) {
	char line[ MAX_LINE ];
	cwd = path_alloc( &path_max_size );

	/* Simulate cwd */
	chdir( dot );

	if ( NULL == getcwd( cwd, path_max_size ) ) {
		print_error( "Failed to get CWD" );
	}

	/* Handle config */
	// if ( -1 == openat( AT_FDCWD, config_name, ) ) {
	// 	print_error( "Failed to open fonfig file %s relative to CWD %s\n", config_name, dot );
	// }

	exit(1);
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
			strncpy( cur_input, line, MAX_LINE );
			cur_input = NULL;
		}

		printf("%s", line );

		if ( strlen( code ) == 0 ) {
			print_error( "Module code is missing" );
			return 1;
		}

		if( iterate( dot ) > 0 ) {
			// print_error( "Iterate error" );
		}

		printf( "Iterate end\n" );

		files->print( files );
		break;
	}

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
	struct stat stat_buffer;

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

			strncpy( item_name, path, path_max_size );

			if ( strlen( item_name ) + 1 + strlen( item->d_name ) < path_max_size ) {
				strcat( item_name, "/" );
				strcat( item_name, item->d_name );

			} else {
				print_error( "Path name is too long" );
			}

			iterate( item_name );
		}

		closedir( dir );
		depth--;
	}
		
}

int parse_config( void )
{
  FILE *fh = fopen( config_name, "r" );
  yaml_parser_t parser;
  yaml_event_t  event;

  /* Initialize parser */
  if(!yaml_parser_initialize(&parser))
    fputs("Failed to initialize parser!\n", stderr);

  if(fh == NULL)
    fputs("Failed to open file!\n", stderr);

  /* Set input file */
  yaml_parser_set_input_file(&parser, fh);

  /* START new code */
  do {
    if (!yaml_parser_parse(&parser, &event)) {
       printf("Parser error %d\n", parser.error);
       exit(EXIT_FAILURE);
    }

    switch(event.type)
    { 
    case YAML_NO_EVENT: puts("No event!"); break;
    /* Stream start/end */
    case YAML_STREAM_START_EVENT: puts("STREAM START"); break;
    case YAML_STREAM_END_EVENT:   puts("STREAM END");   break;
    /* Block delimeters */
    case YAML_DOCUMENT_START_EVENT: puts("<b>Start Document</b>"); break;
    case YAML_DOCUMENT_END_EVENT:   puts("<b>End Document</b>");   break;
    case YAML_SEQUENCE_START_EVENT: puts("<b>Start Sequence</b>"); break;
    case YAML_SEQUENCE_END_EVENT:   puts("<b>End Sequence</b>");   break;
    case YAML_MAPPING_START_EVENT:  puts("<b>Start Mapping</b>");  break;
    case YAML_MAPPING_END_EVENT:    puts("<b>End Mapping</b>");    break;
    /* Data */
    case YAML_ALIAS_EVENT:  printf("Got alias (anchor %s)\n", event.data.alias.anchor); break;
    case YAML_SCALAR_EVENT: printf("Got scalar (value %s)\n", event.data.scalar.value); break;
    }
    if(event.type != YAML_STREAM_END_EVENT)
      yaml_event_delete(&event);
  } while(event.type != YAML_STREAM_END_EVENT);
  yaml_event_delete(&event);
  /* END new code */

  /* Cleanup */
  yaml_parser_delete(&parser);
  fclose(fh);
  return 0;
}

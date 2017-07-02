#include "header.h"

char* mod_name = "Crawler";
char* code;
char* cur_input = NULL;
char *abort_command_str = "q";
char* config_name = ".crawler";
char* cwd;

static int depth = 0;
static off_t total_size = 0;

int command = 0;
int config_is_dirty = 0;
int wait_confirm = 0;
int save_me = 0;
int cwd_length = 0;

size_t path_max_size;

struct llist
			*files,
			*include_dir,
			*exclude_dir,
			*include_file,
			*exclude_file,
			*include_regexp,
			*exclude_regexp,
			*temp;

static clock_t st_time;
static clock_t en_time;
static long clockticks = 0;
static struct tms st_cpu;
static struct tms en_cpu;

int main( int argc, char **argv ) {
	char line[ MAX_LINE ];
	cwd = path_alloc( &path_max_size );
	int files_count;

	signal( SIGINT, &int_handler );

	include_dir = init_llist();
	exclude_dir = init_llist();
	include_file = init_llist();
	exclude_file = init_llist();
	include_regexp = init_llist();
	exclude_regexp = init_llist();
	parse_config();

	files = init_llist();

	parse_args( argv );
	print_args();

	code = (char*)malloc( MAX_LINE );
	get_arg( "code", &code );

	while ( 1 ) {
		if ( 0 == command ) {
			show_commands();
		}

		if( fgets( line, MAX_LINE, stdin ) == NULL && ferror( stdin ) ) {
			perror( "Failed to read line from STDIN" );
			return 1;
		}

		line[ strlen( line ) - 1 ] = '\0';

		if ( strcmp( line, "exit" ) == 0 ) {
			printf( "Exiting...\n" );
		
			break;
		}

		if ( 0 == strcmp( "show_temp", line ) ) {
			temp->print( temp );
			continue;
		}

		if ( 0 == command ) {
			switch( atoi( line ) ) {
			case C_ADD_INCL_FOLDER:
				if( 0 == start_add( include_dir ) ) {
					command = C_ADD_INCL_FOLDER;
				}

				break;
			case C_DEL_INCL_FOLDER:
				if( 0 == start_del( include_dir ) ) {
					command = C_DEL_INCL_FOLDER;
				}

				break;
			case C_ADD_INCL_FILE:
				if( 0 == start_add( include_file ) ) {
					command = C_ADD_INCL_FILE;
				}

				break;
			case C_DEL_INCL_FILE:
				if( 0 == start_del( include_file ) ) {
					command = C_DEL_INCL_FILE;
				}

				break;
			case C_ADD_INCL_REGEXP:
				if( 0 == start_add( include_regexp ) ) {
					command = C_ADD_INCL_REGEXP;
				}

				break;
			case C_DEL_INCL_REGEXP:
				if( 0 == start_del( include_regexp ) ) {
					command = C_DEL_INCL_REGEXP;
				}

				break;
			case C_ADD_EXCL_FOLDER:
				if( 0 == start_add( exclude_dir ) ) {
					command = C_ADD_EXCL_FOLDER;
				}

				break;
			case C_DEL_EXCL_FOLDER:
				if( 0 == start_del( exclude_dir ) ) {
					command = C_DEL_EXCL_FOLDER;
				}

				break;
			case C_ADD_EXCL_FILE:
				if( 0 == start_add( exclude_file ) ) {
					command = C_ADD_EXCL_FILE;
				}

				break;
			case C_DEL_EXCL_FILE:
				if( 0 == start_del( exclude_file ) ) {
					command = C_DEL_EXCL_FILE;
				}

				break;
			case C_ADD_EXCL_REGEXP:
				if( 0 == start_add( exclude_regexp ) ) {
					command = C_ADD_EXCL_REGEXP;
				}

				break;
			case C_DEL_EXCL_REGEXP:
				if( 0 == start_del( exclude_regexp ) ) {
					command = C_DEL_EXCL_REGEXP;
				}

				break;
			case C_ITERATE:
				files->empty( files );
				getcwd( cwd, path_max_size );
				cwd_length = strlen( cwd );
				files_count = 0;
				total_size = 0;
				start_clock();
				iterate( cwd, &files_count );
				end_clock( "Time:" );
				printf("%d files was selected\n", files_count );
				printf( "Total size: %ld\n", total_size );
				break;
			case C_PRINT_FILES:
				files->print( files );
				break;
			case C_PRINT_CONFIG:
				print_config();
				break;
			default:
				show_commands();
				break;
			}

		} else if ( 0 != command ) {
			if ( 1 == wait_confirm ) {
				if ( 0 == strcmp( "y", line ) || 0 == strcmp( "Y", line ) ) {
					if ( 1 == save_me ) {
						confirmed_operation();
					}

					end_operation();

					continue;
				}

				wait_confirm = 0;
				save_me = 0;
				printf( "Does not matter... \n" );
				
				continue;
			}

			if ( 0 == strcmp( "q", line ) || 0 == strcmp( "s", line ) ) {
				printf( "Are sure (y/n)?: " );
				wait_confirm = 1;

				if ( 's' == line[ 0 ] ) {
					save_me = 1;
				}

				continue;
			}

			if ( NULL == temp ) {
				temp = init_llist();
			}

			switch( command ) {
			case C_ADD_INCL_FOLDER:
			case C_ADD_EXCL_FOLDER:
			case C_ADD_INCL_FILE:
			case C_ADD_EXCL_FILE:
			case C_ADD_INCL_REGEXP:
			case C_ADD_EXCL_REGEXP:
				add_to( line );
				break;
			case C_DEL_INCL_FOLDER:
				del_from( line, include_dir );
				break;
			case C_DEL_INCL_FILE:
				del_from( line, include_file );
				break;
			case C_DEL_INCL_REGEXP:
				del_from( line, include_regexp );
				break;
			case C_DEL_EXCL_FOLDER:
				del_from( line, exclude_dir );
				break;
			case C_DEL_EXCL_FILE:
				del_from( line, exclude_file );
				break;
			case C_DEL_EXCL_REGEXP:
				del_from( line, exclude_regexp );
				break;
			default :
				printf( "Unknown command: %d\n", command );
				command = 0;
			}

		} else {
			show_commands();
		}
	}

	save_config();

	return 0;
}

int usage() {
	printf("%.20s - %s\n", "exit", "exit function" );
	exit( 0 );
}

int iterate( const char* path, int* c ) {
	DIR* dir;
	struct dirent* item;
	char item_name[ path_max_size ];
	struct stat stat_buffer;

	if ( lstat( path, &stat_buffer ) < 0 ) {
		return 1;
	}

	if ( S_ISREG( stat_buffer.st_mode ) ) {
		
		// Path name is relative to CWD
		if ( 0 != check_file( &path[ cwd_length ] ) ) {
			return 0;
		}

		files->add( &path[ cwd_length ], path, files );
		total_size += stat_buffer.st_size;
		(*c)++;

	} else if ( S_ISDIR( stat_buffer.st_mode ) ) {
		// printf( "Is DIR\n" );
		// printf( "Path: %s\n", path );
		if ( NULL == ( dir = opendir( path ) ) ) {
			perror( "Opendir error" );
			return 1;
		}

		depth++;

		// printf( "Open DIR\n" );

		while ( NULL != ( item = readdir( dir ) ) ) {
			if ( item->d_name[ 0 ] == '.' ) {
				continue;
			}

			strncpy( item_name, path, path_max_size );

			if ( strlen( item_name ) + 2 + strlen( item->d_name ) < path_max_size ) {
				strcat( item_name, "/" );
				strcat( item_name, item->d_name );

			} else {
				print_error( "Path name is too long" );
			}

			iterate( item_name, c );
		}

		closedir( dir );
		depth--;
	}
}

int parse_config( void ) {
  yaml_parser_t parser;
  yaml_event_t  event;
  FILE *fh = fopen( config_name, "r" );
  int is_mapping = 0;
  int is_sequence = 0;
  int is_include_dir = 0;
  int is_include_file = 0;
  int is_include_regexp = 0;
  int is_exclude_dir = 0;
  int is_exclude_file = 0;
  int is_exclude_regexp = 0;

  if ( NULL == fh ) {
	perror( "Failed to open configuration file" );
	return 1;
  }

  /* Initialize parser */
  if( !yaml_parser_initialize( &parser ) )
    fputs( "Failed to initialize parser!\n", stderr );

  /* Set input file */
  yaml_parser_set_input_file( &parser, fh );

  /* START new code */
  do {
    if ( !yaml_parser_parse( &parser, &event ) ) {
       printf( "Parser error %d\n", parser.error );
       exit( EXIT_FAILURE );
    }

    switch( event.type ) { 
    case YAML_NO_EVENT: puts( "No event!" ); break;
    /* Stream start/end */
    case YAML_STREAM_START_EVENT:
    	// puts( "STREAM START" );
    	break;
    case YAML_STREAM_END_EVENT:
    	// puts( "STREAM END" );
    	break;
    /* Block delimeters */
    case YAML_DOCUMENT_START_EVENT:
    	// puts( "Start Document" );
    	break;
    case YAML_DOCUMENT_END_EVENT:
    	// puts( "End Document" );
    	break;
    case YAML_SEQUENCE_START_EVENT:
    	// puts( "Start Sequence" );
    	is_sequence = 1;
    	break;
    case YAML_SEQUENCE_END_EVENT:
    	// puts( "End Sequence" );
    	is_sequence = 0;
    	is_include_dir = 0;
    	is_include_file = 0;
    	is_include_regexp = 0;
    	is_exclude_dir = 0;
    	is_exclude_file = 0;
    	is_exclude_regexp = 0;
   		break;
    case YAML_MAPPING_START_EVENT:
    	// puts( "Start Mapping" );
    	is_mapping = 1;
    	break;
    case YAML_MAPPING_END_EVENT:
    	// puts( "End Mapping" );
    	is_mapping = 0;
   	 	break;
    /* Data */
    case YAML_ALIAS_EVENT:
    	// printf( "Got alias (anchor %s)\n", event.data.alias.anchor );
    	break;
    case YAML_SCALAR_EVENT:
	    // printf( "Got scalar (value %s)\n", event.data.scalar.value );

    	if ( 0 == is_sequence && 1 == is_mapping ) {
    		if ( 0 == strcmp( "include_dir", event.data.scalar.value ) ) {
    			is_include_dir = 1;

    		} else if ( 0 == strcmp( "include_file", event.data.scalar.value ) ) {
    			is_include_file = 1;

    		} else if ( 0 == strcmp( "include_regexp", event.data.scalar.value ) ) {
    			is_include_regexp = 1;

    		} else if ( 0 == strcmp( "exclude_dir", event.data.scalar.value ) ) {
    			is_exclude_dir = 1;

    		} else if ( 0 == strcmp( "exclude_file", event.data.scalar.value ) ) {
    			is_exclude_file = 1;

    		} else if ( 0 == strcmp( "exclude_regexp", event.data.scalar.value ) ) {
    			is_exclude_regexp = 1;
    		}

    	} else if ( 1 == is_sequence ) {
    		if ( is_include_dir ) {
    			include_dir->add( NULL, event.data.scalar.value, include_dir );

    		} else if ( is_include_file ) {
    			include_file->add( NULL, event.data.scalar.value, include_file );

    		} else if ( is_include_regexp ) {
    			include_regexp->add( NULL, event.data.scalar.value, include_regexp );

    		} else if ( is_exclude_dir ) {
    			exclude_dir->add( NULL, event.data.scalar.value, exclude_dir );

    		} else if ( is_exclude_file ) {
    			exclude_file->add( NULL, event.data.scalar.value, exclude_file );

    		} else if ( is_exclude_regexp ) {
    			exclude_regexp->add( NULL, event.data.scalar.value, exclude_regexp );
    		}
    	}

   		break;
    }

    if( event.type != YAML_STREAM_END_EVENT )
      yaml_event_delete( &event );

  } while( event.type != YAML_STREAM_END_EVENT );

  yaml_event_delete( &event );
  /* END new code */

  /* Cleanup */
  yaml_parser_delete( &parser );
  fclose( fh );

  return 0;
}

int save_config() {
	char *raw_name = "~conf";
	char *temp_name = "~temp";
	FILE *tc = fopen( raw_name, "w" );

	if ( 0 == config_is_dirty ) return 0;

	if ( NULL == tc ) {
		perror( "Failed to create temporary configuration file" );
		return 1;
	}

	write_config_section( "include_dir", tc, include_dir );
	write_config_section( "include_file", tc, include_file );
	write_config_section( "include_regexp", tc, include_regexp );
	write_config_section( "exclude_dir", tc, exclude_dir );
	write_config_section( "exclude_file", tc, exclude_file );
	write_config_section( "exclude_regexp", tc, exclude_regexp );

	/* Save changes into disk */
	if ( 0 == rename( config_name, temp_name ) ) { /* old config to temp name */
		if ( 0 == rename( raw_name, config_name ) ) { /* new config to config name */
			if ( 0 != unlink( temp_name ) ) { /* delete old config */
				perror( "Failed to delete transient configuration file" );
				return 5;
			}

			fputs( "Configuration file was saved\n", stdout );

		} else {
			perror( "Failed to set name for newly created configuration file" );

			if ( rename( temp_name, config_name ) ) {
				fputs( "Old configuration file is restored\n", stderr );
				return 3;

			} else {
				perror( "Failed to restore old configuration file" );
				return 4;
			}
		}

	} else {
		perror( "Failed to save new configurations. Failed set temporary name for configuration file" );
		return 2;
	}
}

int write_config_section( char* name, FILE* stream, struct llist* l ) {
	if ( NULL != l && l->first ) {
		l->current = l->first;

		fprintf( stream, "%s:\n", name );

		while( l->current ) {
			fprintf( stream, " - %s\n", l->current->value );
			l->current = l->current->next;
		}
	}

	return 0;
}

int start_del( struct llist* l ) {
	if ( l && l->first ) {
		printf(
			"Type a number of a record to be deleted\n"
			"To save changes type 's', to discard changes type 'q'\n"
		);

		print_del_list( l );
		
	} else {
		printf( "List is empty\n" );

		return 1;
	}

	return 0;
}

int print_del_list( struct llist* l ) {
	if ( NULL == l || NULL == l->first ) return 1;

	l->current = l->first;

	while( l->current && l->current->name ) {
		if ( NULL != temp && NULL != temp->first && 0 == temp->has_value( l->current->name, temp ) ) {
			printf( "*" );

		} else {
			printf( " " );
		}

		printf( "[%.2s] - %s\n", l->current->name, l->current->value );
		l->current = l->current->next;
	}

	printf( "Remove item >> " );

	return 0;
}

int start_add( struct llist* l ) {
	printf( "Type in one item at line\n" );
	printf( "To save changes print 's', to discard changes - 'q'\n" );

	if ( NULL != l->first ) {
		printf( "Existing items:\n" );
		l->print( l );

	} else {
		printf( "List is empty\n" );
	}

	printf( "Add item >> " );

	return 0;
}

int add_to( const char *item ) {
	if ( NULL != temp ) {
		printf( "Add item >> " );
		return temp->add( NULL, item, temp );
	}

	fprintf( stderr, "Failed to add item to list: list is missing" );

	return 1;
}

int del_from( const char *name, struct llist* l ) {
	if ( NULL != temp ) {
		temp->add( NULL, name, temp );
		printf( "\033[%dA", l->count( l ) + 1 );
		printf( "\033[100D" );
		print_del_list( l );
		printf( "\033[K" );
	}

	return 0;
}

int show_commands() {
	printf(
		"%2d - Set module name\n"

		"%2d - Add included directories\n"
		"%2d - Add excluded directories\n"
		"%2d - Delete included directories\n"
		"%2d - Delete excluded directories\n"

		"%2d - Add included files\n"
		"%2d - Add excluded files\n"
		"%2d - Delete included files\n"
		"%2d - Delete excluded files\n"

		"%2d - Add included regexp\n"
		"%2d - Add excluded regexp\n"
		"%2d - Delete included regexp\n"
		"%2d - Delete excluded regexp\n"

		"%2d - Iterate over FS\n"
		"%2d - Print files\n"
		"%2d - Print configurations\n"

		"Make your choice > ",

		C_SET_NAME,

		C_ADD_INCL_FOLDER,
		C_ADD_EXCL_FOLDER,
		C_DEL_INCL_FOLDER,
		C_DEL_EXCL_FOLDER,

		C_ADD_INCL_FILE,
		C_ADD_EXCL_FILE,
		C_DEL_INCL_FILE,
		C_DEL_EXCL_FILE,

		C_ADD_INCL_REGEXP,
		C_ADD_EXCL_REGEXP,
		C_DEL_INCL_REGEXP,
		C_DEL_EXCL_REGEXP,

		C_ITERATE,
		C_PRINT_FILES,
		C_PRINT_CONFIG
	);
}

int confirmed_operation() {
	switch ( command ) {
	case C_ADD_INCL_FOLDER:
		add_to_config( include_dir );
		break;
	case C_DEL_INCL_FOLDER:
		remove_from_config( include_dir );
		break;
	case C_ADD_INCL_FILE:
		add_to_config( include_file );
		break;
	case C_DEL_INCL_FILE:
		remove_from_config( include_file );
		break;
	case C_ADD_INCL_REGEXP:
		add_to_config( include_regexp );
		break;
	case C_DEL_INCL_REGEXP:
		remove_from_config( include_regexp );
		break;
	case C_ADD_EXCL_FOLDER:
		add_to_config( exclude_dir );
		break;
	case C_DEL_EXCL_FOLDER:
		remove_from_config( exclude_dir );
		break;
	case C_ADD_EXCL_FILE:
		add_to_config( exclude_file );
		break;
	case C_DEL_EXCL_FILE:
		remove_from_config( exclude_file );
		break;
	case C_ADD_EXCL_REGEXP:
		add_to_config( exclude_regexp );
		break;
	case C_DEL_EXCL_REGEXP:
		remove_from_config( exclude_regexp );
		break;
	default:
		print_error( "Invalid command: %d\n", command );
	}

	return 0;
}

int add_to_config( struct llist* l ) {
	l->merge( temp, l );
	config_is_dirty = 1;

	return 0;
}

int remove_from_config( struct llist* l ) {
	if ( NULL != l && NULL != l->first ) {
		l->current = l->first;

		while( l->current ) {
			if ( 0 == temp->has_value( l->current->name, temp ) ) {
				l->remove( l->current->name, l );

				if ( NULL == l->current ) {
					break;
				}
			}

			l->current = l->current->next;
		}

		config_is_dirty = 1;
	}

	return 0;
}

int end_operation() {
	temp->empty( temp );
	wait_confirm = 0;
	command = 0;
	save_me = 0;

	return 0;
}

/**
 * Reliable signal function from APU (restarts all interrupted system calls but SIGALARM)
 *
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

void int_handler( int s ) {
	if ( s == SIGINT ) {
		printf( "\nInterrupted\n" );
		save_config();
		exit( 2 );
	}
}

int check_file( const char *name ) {
	char dir[ path_max_size ];
	char *pos;

	int max_incl_dir;
	int max_excl_dir;

	// File is in the include list
	if ( 0 == include_file->has_value( name, include_file ) ) {
		return 0;
	}

	// File is in the exclude list
	if ( 0 == exclude_file->has_value( name, exclude_file ) ) {
		return 1;
	}

	strncpy( dir, name, path_max_size );
	pos = strrchr( dir, '/' );

	if ( NULL != pos ) {
		*pos = '\0';

		collide_length( dir, include_dir, &max_incl_dir );
		collide_length( dir, exclude_dir, &max_excl_dir );
		
		if ( max_incl_dir > 0 && max_incl_dir >= max_excl_dir ) {
			return 0;
		}

		if ( max_excl_dir > 0 ) {
			return 1;
		}
	}

	if ( 0 == check_regexp( name, include_regexp ) && 1 == check_regexp( name, exclude_regexp ) ) {
		return 0;
	}

	return 1;
}

int collide_length( const char *name, struct llist *l, int *max ) {
	int span = 0;
	int cur_str_len = 0;

	*max = 0;

	if ( l->first ) {
		l->current = l->first;

		while ( l->current ) {
			cur_str_len = strlen( l->current->value );

			if ( span > cur_str_len ) {
				continue;
			}

			if( 0 == collide_span( name, l->current->value ) ) {
				span = cur_str_len;

				if ( span > *max ) {
					*max = span;
				}
			}

			l->current = l->current->next;
		}

		l->current = l->first;
	}

	return 0;
}

int collide_span( const char *h, const char *n ) {
	int i;
	int c = 0;
	int nl = strlen( n );
	int hl = strlen( h );

	if ( hl >= nl ) {
		for( i = 0; i < nl; i++ ) {
			if ( h[ i ] == n[ i ] ) {
				c++;

			} else {
				break;
			}
		}
	}

	return c == nl ? 0 : 1;
}

int match( const char* str, const char* pattern, int flags ) {
	regex_t *compilled = (regex_t*)malloc( sizeof ( regex_t ) );
	flags |= REG_EXTENDED | REG_NOSUB;
	int status;

	if ( NULL == compilled ) {
		print_error( "Failed to locate memory for regex_t structure" );
	}

	status = regcomp( compilled, pattern, flags );

	if ( 0 != status ) {
		print_error( get_regerror( status, compilled ) );
	}

	status =  regexec ( compilled, str, 0, NULL, 0 );

	if ( REG_ESPACE == status ) {
		print_error( get_regerror( status, compilled ) );
	}

	regfree( compilled );
	free( compilled );

	return status;
}

char *get_regerror ( int errcode, regex_t *compiled ) {
	size_t length = regerror ( errcode, compiled, NULL, 0 );
	char *buffer = malloc ( length );

	if ( NULL == buffer ) {
		print_error( "Failed to allocate memory for regular expression error message" );
	}

	( void )regerror ( errcode, compiled, buffer, length );

	return buffer;
}

int check_regexp( const char* str, struct llist* l ) {
	if ( l->first ) {
		l->current = l->first;

		while ( l->current ) {
			if ( 0 == match( str, l->current->value, 0 ) ) {
				return 0;
			}

			l->current = l->current->next;
		}

		l->current = l->first;
	}

	return 1;
}

void start_clock() {
    st_time = times( &st_cpu );
}

void end_clock( char *msg ) {
    en_time = times( &en_cpu );

    if ( clockticks == 0 ) {
    	if ( 0 > ( clockticks = sysconf( _SC_CLK_TCK ) ) ) {
    		print_error( "Failed to fetch system clock ticks" );
    	}
    }

    printf( "%s\n", msg );
    printf(
    	"Real Time: %7.2f, User Time %7.2f, System Time %7.2f\n",
        ( en_time - st_time ) / (float)clockticks,
        ( en_cpu.tms_utime - st_cpu.tms_utime ) / (float)clockticks,
        ( en_cpu.tms_stime - st_cpu.tms_stime ) / (float)clockticks
    );
}

int print_config() {
	FILE *stream = fopen( config_name, "r" );
	char buffer[ MAX_LINE ];

	if ( NULL != stream ) {
		while ( NULL != fgets( buffer, MAX_LINE, stream ) ) {
			if ( EOF == fputs( buffer, stdout ) ) {
				perror( "Print configuration" );
				exit( 1 );
			}
		}

		if ( ferror( stream ) ) {
			perror( "Print configuration" );
			exit( 1 );
		}
	}

	return 0;
}


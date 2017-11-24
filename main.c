#include "header.h"

char* mod_name = "Crawler";
char* code;
char* cur_input = NULL;
char *abort_command_str = "q";
char config_name[ MAX_LINE ] = ".crawler";
char *pckg_tmp_dir = ".tpm_pckg/";
char *upload_folder = "upload/";
char *crawler_storage_dir = "/var/www/html/crawler/";
char *pckg_name_templ = "%s-%s-%d.%d.%d.ocmod.zip";
char *pckg_mane_regex = "%s-[^-]+-([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.ocmod\\.zip";
char *lang_prefix_23 = "language/en-gb/";
char *lang_prefix_20 = "language/english/";

char *lang_dir;
char* cwd;

static int depth = 0;
static off_t total_size = 0;

int command = 0;
int config_is_dirty = 0;
int wait_confirm = 0;
int save_me = 0;
int cwd_length = 0;
int files_count;
int major = 0; // Package major number
int minor = 0; // Package minor number
int patch = 0; // Package patch number

size_t path_max_size;

struct llist
			// *files,
			*temp,
			*filters,
			*admin_t,
			*catalog_t,
			*common_t;

GHashTable *config = NULL;
GSList *filter_names = NULL;

GSList *files;

// Lists to store filters temporary during file system iteration to boost up performance
GSList *include_file_temp;
GSList *exclude_file_temp;
GSList *include_folder_temp;
GSList *exclude_folder_temp;
GSList *include_regex_temp;
GSList *exclude_regex_temp;

struct winsize win_size;

regmatch_t m[ REGEX_MATCH_COUNT ];

static clock_t st_time;
static clock_t en_time;
static long clockticks = 0; 
static struct tms st_cpu;
static struct tms en_cpu;

// Select package config input
GtkComboBoxText *select_package;

// Package code input
GtkEntry *input_code;

// Package version number inputs
GtkSpinButton *input_major;
GtkSpinButton *input_minor;
GtkSpinButton *input_patch;

// Package file inputs
GtkTextBuffer *buffer_include_file;
GtkTextBuffer *buffer_exclude_file;
GtkTextBuffer *buffer_include_folder;
GtkTextBuffer *buffer_exclude_folder;
GtkTextBuffer *buffer_include_regex;
GtkTextBuffer *buffer_exclude_regex;

// Config manage buttons
GtkButton *button_reload_config;
GtkButton *button_save_config;
GtkButton *button_delete_config;

// Delete package config dialog
GtkDialog *delete_package_confirm;

// Delete package confirm dialog's buttons
GtkButton *button_delete_package_ok;
GtkButton *button_delete_package_cancel;

// Select package event handler ID
gulong select_package_handler = 0;

int main( int argc, char **argv ) {
	if ( DEBUG )print_color( B_RED, "Stat\n" );

	GtkBuilder *UI_builder;
	GSList *lp, *lst;
	GtkWidget *window;

	char *path;
	char *dir;
	char *file;

	// Define system dependent path maximum size
	path_max_size = get_path_max_size();

	// Remember CWD where extension was launched
	cwd = g_get_current_dir();

	file = (char*)g_malloc( path_max_size );
	dir = g_path_get_dirname( __FILE__ );
	file = g_build_filename( dir, "ui.glade", NULL );

	if ( DEBUG )printf("UI file: %s\n", file );

	gtk_init(NULL, NULL);

	UI_builder = gtk_builder_new_from_file ( file );

	g_free( dir );
	g_free( file );

	/*                     Main window                       */
    window = GTK_WIDGET( gtk_builder_get_object( UI_builder, "window1" ) );

    // Close the program
    g_signal_connect (window, "destroy", G_CALLBACK ( destroy ), NULL);

    /*                    Select package combobox              */
    select_package = GTK_COMBO_BOX_TEXT( gtk_builder_get_object( UI_builder, "select_package" ) );

    // Select configuration file action
    select_package_handler = g_signal_connect ( select_package, "changed", G_CALLBACK ( fill_in_config ), NULL);

    /*                    Module code input                     */
    input_code = GTK_ENTRY( gtk_builder_get_object( UI_builder, "input_code" ) );
 
    /*                    Version numbers                        */
    input_major = GTK_SPIN_BUTTON( gtk_builder_get_object( UI_builder, "input_major" ) );
    input_minor = GTK_SPIN_BUTTON( gtk_builder_get_object( UI_builder, "input_minor" ) );
    input_patch = GTK_SPIN_BUTTON( gtk_builder_get_object( UI_builder, "input_patch" ) );

    /*                    Filter inputs                           */
    buffer_include_file   = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_include_file" ) );
    buffer_exclude_file   = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_exclude_file" ) );
    buffer_include_folder = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_include_folder" ) );
    buffer_exclude_folder = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_exclude_folder" ) );
    buffer_include_regex  = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_include_regex" ) );
    buffer_exclude_regex  = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_exclude_regex" ) );

    /*                     Configuration buttons                   */
    button_reload_config = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_reload_config" ) );
    button_save_config   = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_save_config" ) );
    button_delete_config = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_delete_config" ) );

    // Reload configuration files click handler
    g_signal_connect ( button_reload_config, "clicked", G_CALLBACK ( reload_config ), NULL);

    // Save configuration files click handler
    g_signal_connect ( button_save_config, "clicked", G_CALLBACK ( save_config ), NULL);

    // Delete configuration files click handler
    g_signal_connect ( button_delete_config, "clicked", G_CALLBACK ( delete_config ), NULL);

    /*                      Confirm delete package dialog                     */
    delete_package_confirm = GTK_DIALOG( gtk_builder_get_object( UI_builder, "delete_package_confirm" ) );
    button_delete_package_ok = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_delete_package_ok" ) );
    button_delete_package_cancel = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_delete_package_cancel" ) );
  
    gtk_widget_show( window );

    // Populate select package combobox with data
    get_package_configs();
    gtk_main ();

	return 0;
	// int debug = 0;

	// char line[ MAX_LINE ];

	// if( NULL == getcwd( cwd, path_max_size ) ) {
	// 	fprintf( stderr, "main: failed to get CWD" );
	// 	exit( 1 );
	// }

	// cwd_length = strlen( cwd );

	// int i;
	// regmatch_t *t_m;

	// code = (char*)malloc( MAX_LINE );

	// if ( NULL == code ) {
	// 	print_error( "Failed to allocate memory for code package code" );
	// }

	// memset( code, '\0', MAX_LINE );

	// lang_dir = malloc( path_max_size );

	// if ( NULL == lang_dir ) {
	// 	print_error( "Failed allocate memory for lang_dir variable" );
	// }

	// memset( lang_dir, '\0', path_max_size );

	// signal( SIGINT, &int_handler );
	// signal( SIGWINCH, &sig_winch );
	// // signal( SIGCLD, &sig_cld );

	// set_winsize();

	// if ( DEBUG || debug ) fprintf( stderr, "Initializing lists....\n" );

	// // include_dir = init_llist();
	// // exclude_dir = init_llist();
	// // include_file = init_llist();
	// // exclude_file = init_llist();
	// // include_regexp = init_llist();
	// // exclude_regexp = init_llist();
	// admin_t = init_llist();
	// catalog_t = init_llist();
	// common_t = init_llist();
	// temp = init_llist();
	// files = init_llist();

	// parse_args( argv );

	// if ( 1 == get_arg( "code", &code ) ) {
	// 	fprintf( stderr, "Supply package code name as --code argument\n" );
	// 	exit( 1 );
	// }

	// set_config_name();

	// if ( DEBUG || debug ) print_args();
	// if ( DEBUG || debug ) fprintf( stderr, "Parsing configuration....\n" );

	// // parse_config();

	// if ( DEBUG || debug ) fprintf( stderr, "Configuration has been parsed\n" );

	// while ( 1 ) {
	// 	if ( 0 == command ) {
	// 		show_commands();
	// 	}

	// 	memset( line, '\0', MAX_LINE );

	// 	if( fgets( line, MAX_LINE, stdin ) == NULL && ferror( stdin ) ) {
	// 		perror( "Failed to read line from STDIN" );
	// 		return 1;
	// 	}

	// 	// Trim trailing newline
	// 	line[ strlen( line ) - 1 ] = '\0';

	// 	if ( strcmp( line, "exit" ) == 0 ) {
	// 		printf( "Exiting...\n" );
		
	// 		break;
	// 	}

	// 	if ( 0 == strcmp( "show_temp", line ) ) {
	// 		temp->print( temp );
	// 		continue;
	// 	}

	// 	if ( 0 == command ) {
	// 		switch( atoi( line ) ) {
	// 		case C_ADD_INCL_FOLDER:
	// 			if( 0 == start_add( include_dir ) ) {
	// 				command = C_ADD_INCL_FOLDER;
	// 			}

	// 			break;
	// 		case C_DEL_INCL_FOLDER:
	// 			if( 0 == start_del( include_dir ) ) {
	// 				command = C_DEL_INCL_FOLDER;
	// 			}

	// 			break;
	// 		case C_ADD_INCL_FILE:
	// 			if( 0 == start_add( include_file ) ) {
	// 				command = C_ADD_INCL_FILE;
	// 			}

	// 			break;
	// 		case C_DEL_INCL_FILE:
	// 			if( 0 == start_del( include_file ) ) {
	// 				command = C_DEL_INCL_FILE;
	// 			}

	// 			break;
	// 		case C_ADD_INCL_REGEXP:
	// 			if( 0 == start_add( include_regexp ) ) {
	// 				command = C_ADD_INCL_REGEXP;
	// 			}

	// 			break;
	// 		case C_DEL_INCL_REGEXP:
	// 			if( 0 == start_del( include_regexp ) ) {
	// 				command = C_DEL_INCL_REGEXP;
	// 			}

	// 			break;
	// 		case C_ADD_EXCL_FOLDER:
	// 			if( 0 == start_add( exclude_dir ) ) {
	// 				command = C_ADD_EXCL_FOLDER;
	// 			}

	// 			break;
	// 		case C_DEL_EXCL_FOLDER:
	// 			if( 0 == start_del( exclude_dir ) ) {
	// 				command = C_DEL_EXCL_FOLDER;
	// 			}

	// 			break;
	// 		case C_ADD_EXCL_FILE:
	// 			if( 0 == start_add( exclude_file ) ) {
	// 				command = C_ADD_EXCL_FILE;
	// 			}

	// 			break;
	// 		case C_DEL_EXCL_FILE:
	// 			if( 0 == start_del( exclude_file ) ) {
	// 				command = C_DEL_EXCL_FILE;
	// 			}

	// 			break;
	// 		case C_ADD_EXCL_REGEXP:
	// 			if( 0 == start_add( exclude_regexp ) ) {
	// 				command = C_ADD_EXCL_REGEXP;
	// 			}

	// 			break;
	// 		case C_DEL_EXCL_REGEXP:
	// 			if( 0 == start_del( exclude_regexp ) ) {
	// 				command = C_DEL_EXCL_REGEXP;
	// 			}

	// 			break;
	// 		case C_ITERATE:
	// 			files->empty( files );
	// 			files_count = 0;
	// 			total_size = 0;
	// 			start_clock();
	// 			iterate( cwd, &check_item, NULL, &on_iterate_error );
	// 			load_dependencies();
	// 			end_clock( "Time:" );
	// 			printf("%d files was selected\n", files_count );
	// 			printf( "Total size: %ld\n", total_size );
	// 			break;
	// 		case C_PRINT_FILES:
	// 			print_files();
	// 			break;
	// 		case C_PRINT_CONFIG:
	// 			print_config();
	// 			break;
	// 		case C_MAKE:
	// 			make_package();
	// 			break;
	// 		case C_SET_NAME:
	// 			command = C_SET_NAME;
	// 			printf( "Extension name >> " );
	// 			break;
	// 		case C_SET_MAJOR:
	// 			command = C_SET_MAJOR;
	// 			printf( "Major number >> " );
	// 			break;
	// 		case C_SET_MINOR:
	// 			command = C_SET_MINOR;
	// 			printf( "Manor number >> " );
	// 			break;
	// 		case C_SET_PATCH:
	// 			command = C_SET_PATCH;
	// 			printf( "Patch number >> " );
	// 			break;
	// 		default:
	// 			show_commands();
	// 			break;
	// 		}

	// 	} else if ( 0 != command ) {
	// 		if ( 1 == wait_confirm ) {
	// 			if ( 0 == strcmp( "y", line ) || 0 == strcmp( "Y", line ) ) {
	// 				if ( 1 == save_me ) {
	// 					confirmed_operation();
	// 				}

	// 				end_operation();

	// 				continue;
	// 			}

	// 			wait_confirm = 0;
	// 			save_me = 0;
	// 			printf( "Does not matter... \n" );
				
	// 			continue;
	// 		}

	// 		if ( 0 == strcmp( "q", line ) || 0 == strcmp( "s", line ) ) {
	// 			printf( "Are sure (y/n)?: " );
	// 			wait_confirm = 1;

	// 			if ( 's' == line[ 0 ] ) {
	// 				save_me = 1;
	// 			}

	// 			continue;
	// 		}

	// 		if ( NULL == temp ) {
	// 			temp = init_llist();
	// 		}

	// 		switch( command ) {
	// 		case C_ADD_INCL_FOLDER:
	// 		case C_ADD_EXCL_FOLDER:
	// 		case C_ADD_INCL_FILE:
	// 		case C_ADD_EXCL_FILE:
	// 		case C_ADD_INCL_REGEXP:
	// 		case C_ADD_EXCL_REGEXP:
	// 			add_to( line );
	// 			break;
	// 		case C_DEL_INCL_FOLDER:
	// 			del_from( line, include_dir );
	// 			break;
	// 		case C_DEL_INCL_FILE:
	// 			del_from( line, include_file );
	// 			break;
	// 		case C_DEL_INCL_REGEXP:
	// 			del_from( line, include_regexp );
	// 			break;
	// 		case C_DEL_EXCL_FOLDER:
	// 			del_from( line, exclude_dir );
	// 			break;
	// 		case C_DEL_EXCL_FILE:
	// 			del_from( line, exclude_file );
	// 			break;
	// 		case C_DEL_EXCL_REGEXP:
	// 			del_from( line, exclude_regexp );
	// 			break;
	// 		case C_SET_NAME:
	// 			memset( code, '\0', MAX_LINE );
	// 			strncpy( code, line, MAX_LINE );
	// 			command = 0;
	// 			config_is_dirty = 1;
	// 			break;
	// 		case C_SET_MAJOR:
	// 			major = atoi( line );
	// 			minor = 0;
	// 			patch = 0;
	// 			command = 0;
	// 			config_is_dirty = 1;
	// 			break;
	// 		case C_SET_MINOR:
	// 			minor = atoi( line );
	// 			patch = 0;
	// 			command = 0;
	// 			config_is_dirty = 1;
	// 			break;
	// 		case C_SET_PATCH:
	// 			patch = atoi( line );
	// 			command = 0;
	// 			config_is_dirty = 1;
	// 			break;
	// 		default :
	// 			printf( "Unknown command: %d\n", command );
	// 			command = 0;
	// 		}

	// 	} else {
	// 		show_commands();
	// 	}
	// }

	// save_config();

	return 0;
}

/**
 * Exits main loop and terminates the program
 */
void destroy( GtkWidget *widget, gpointer data ) {
    gtk_main_quit ();
}

/**
 * Fills in dropdown with package configuration files
 */
int get_package_configs() {
	struct dirent **list;
	int n;

	if ( DEBUG ) printf( "Searching for package configs...\n" );

	gtk_combo_box_text_remove_all ( select_package );

	n = scandir( ".", &list, &filter_package_config_name, alphasort );

	if ( n == -1 ) {
		fprintf( stderr, "Failed to scan directory '.': %s", strerror( errno ) );
		return 1;
	}

	if ( DEBUG )printf("%i package files were found\n", n );

	// Add empty filed
	gtk_combo_box_text_append_text( select_package, "" );

	while ( n-- ) {
		if ( DEBUG )printf( "%s\n", list[ n ]->d_name );
		gtk_combo_box_text_append_text( select_package, list[ n ]->d_name );
		free( list[ n ] );
	}

	free( list );

	return 0;
}

/**
 * Filters directory entry
 * Returns 1 if directory entry is package config file
 */
int filter_package_config_name( const struct dirent* entry ) {
	return g_str_has_suffix( entry->d_name, ".package" );
}

/**
 * Print system error
 */
void show_error( char *message ) {
	fputs( message, stderr );
}

int usage() {
	// printf("%.20s - %s\n", "exit", "exit function" );
	exit( 0 );
}

void get_files() {
	include_file_temp = g_hash_table_lookup( config, "include_file" );
	exclude_file_temp = g_hash_table_lookup( config, "include_file" );
	include_folder_temp = g_hash_table_lookup( config, "include_file" );
	exclude_folder_temp = g_hash_table_lookup( config, "include_file" );
	include_regex_temp = g_hash_table_lookup( config, "include_file" );
	exclude_regex_temp = g_hash_table_lookup( config, "include_file" );

	iterate( g_strdup( cwd ), &check_item, NULL, &on_iterate_error );
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
	struct stat *stat_buffer = NULL;
	gboolean = is_error = FALSE;
	char *dir_to_iterate;

	if ( DEBUG || debug )fprintf( stderr, "Iterate: '%s'\n", path );

	if ( G_DIR_SEPARATOR_S != path[ 0 ] ) {
		// absolute_path = g_build_filename( cwd, )

		// TODO: handle situation with relative paths, if needed
		fprintf( stderr, "Path need to be absolute (%s). In %s:%i\n", path, __FILE__, __LINE__ );
		is_error = TRUE;
		goto exit_point;
	}

	struct stat *stat_buffer = Lstat( path );

	if ( NULL == stat_buffer ) {
		if ( NULL != err_cb && 0 != err_cb( path, NULL ) ) {
			exit( 1 );
		}

		is_error = TRUE;
		goto exit_point;
	}

	if ( is_file( path ) ) {
		if ( DEBUG || debug )fprintf( stderr, "Is file\n" );

		if ( NULL != file_cb && 0 != file_cb( path, NULL ) ) {
			is_error = TRUE;
			goto exit_point;
		}

	} else if ( is_dir( path ) ) {
		if ( DEBUG || debug )fprintf( stderr, "Is folder\n" );

		if ( NULL == ( dir = opendir( path ) ) ) {
			fprintf(stderr, "Failed to open folder in %s:%i:%s\n", __FILE__, __LINE__, strerror( errno ) );
			is_error = TRUE;
			goto exit_point;
		}

		if ( DEBUG || debug )fprintf( stderr, "Dir has been entered: %s\n", path );

		depth++;

		errno = 0;

		while ( NULL != ( dir_entry = readdir( dir ) ) ) {
			if ( dir_ntry->d_name[ 0 ] == '.' ) {
				continue;
			}

			dir_to_iterate = g_build_filename( path, dir_entry->d_name ); /* needs to be freed in callee */

			iterate( dir_to_iterate, file_cb, dir_cb, err_cb );
		}

		if ( 0 != errno && NULL == item ) {
			fprintf( stderr, "Error while reading directory %s: %s in %s:%i\n", path, strerror( errno ), __FILE__, __LINE__ );
			is_error = TRUE;
			goto exit_point;
		}

		// Run DIR callback after all FILE callbacks
		if ( NULL != dir_cb && 0 != dir_cb( path, NULL ) ) {
			return 1;
		}

		closedir( dir );
		depth--;

	} else {
		print_error( "%s is not a file nor a directory\n", path );
		is_error = TRUE;
		goto exit_point;
	}

exit_point:
	g_free( path );
	g_free( stat_buffer );

	if ( NULL != dir ) g_free( dir );

	if ( is_error ) {
		return 1;
	}

	return 0;
}

/**
 * Checks item (file) and add it to package files list on success 
 */
int check_item( char *name, void* data ) {
	if ( DEBUG )fprintf( stderr, "Start checking file %s\n", name );

	if ( check_file( name ) ) {
		if ( DEBUG )fprintf( stderr, "Passed check\n" );

		g_slist_append( files, g_strdup( name ) );
		total_size += filesize( name );
		files_count++;

	} else {
		if ( DEBUG )fprintf( stderr, "Check failed\n" );
	}

	return 0;
}

/**
 * Add files from header source section to package files list
 *
 */
int check_source(  char *name, struct stat* sb ) {
	int debug = 0;

	if ( DEBUG || debug )fprintf( stderr, "Add source file %s\n", name );

	// Potential source of inaccuracy: some files may be already present in files structure
	// and on merge such files will be skipped
	temp->add( &name[ cwd_length  +1 ], name, temp );
	total_size += sb->st_size;
	files_count++;
}

/**
 * Callback for FS iterator
 * Prints error in STDERR end returns status 1
 *
 */
int on_iterate_error( char *name ) {
	fprintf( stderr, "%s: %s\n", strerror( errno ), name );

	return 1;
}

/**
 * Saves configuration file
 * CHDIR to CWD
 *
 */
// int save_config() {
	// xmlDocPtr doc;
	// xmlNodePtr root, child;

	// char b[ 10 ];

	// if ( 0 == config_is_dirty ) return 0;

	// if ( -1 == chdir( cwd ) ) {
	// 	fprintf( stderr, "save_config: failed to change CWD to '%s': %s\n", cwd, strerror( errno ) );
	// 	exit( 1 );
	// }

	// doc = xmlNewDoc( "1.0" );
	// root = xmlNewNode( NULL, "config" );
	// xmlDocSetRootElement( doc, root );

	// xmlAddChild( root, config_to_xml( "include_file", include_file ) );
	// xmlAddChild( root, config_to_xml( "exclude_file", exclude_file ) );
	// xmlAddChild( root, config_to_xml( "include_dir", include_dir ) );
	// xmlAddChild( root, config_to_xml( "exclude_dir", exclude_dir ) );
	// xmlAddChild( root, config_to_xml( "include_regexp", include_regexp ) );
	// xmlAddChild( root, config_to_xml( "exclude_regexp", exclude_regexp ) );

	// child = xmlNewNode( NULL, "code" );
	// xmlNodeAddContent( child, code );
	// xmlAddChild( root, child );

	// child = xmlNewNode( NULL, "major" );
	// sprintf( b, "%d", major );
	// xmlNodeAddContent( child, b );
	// xmlAddChild( root, child );

	// child = xmlNewNode( NULL, "minor" );
	// sprintf( b, "%d", minor );
	// xmlNodeAddContent( child, b );
	// xmlAddChild( root, child );

	// child = xmlNewNode( NULL, "patch" );
	// sprintf( b, "%d", patch );
	// xmlNodeAddContent( child, b );
	// xmlAddChild( root, child );

	// if ( xmlSaveFormatFile( config_name, doc, 1 ) != -1 ) {
	// 	fprintf( stderr, "Configuration file was updated\n" );

	// } else {
	// 	fprintf( stderr, "Failed to update configuration file\n" );
	// }

	// xmlFreeNode( child );
	// xmlFreeDoc( doc );

// 	return 0;
// }

/**
 * Adds list items to XML structure
 */
xmlNodePtr config_to_xml( char *name, GSList *l ) {
	if ( DEBUG )print_color( B_GREEN, "config_to_xml: %s\n", name );

	int debug = 0;
	GSList *list = l;

	xmlNodePtr cur, parent;

	if ( DEBUG || debug ) fprintf( stderr, "Processing list %s\n", name );
	if ( DEBUG || debug ) dump_slist( list );

	parent = xmlNewNode( NULL, name );

	while ( NULL != list ) {
		if ( DEBUG || debug ) fprintf( stderr, "Adding %s\n", (char*)list->data );
		cur = xmlNewNode( NULL, "item" );
		xmlNodeAddContent( cur, (xmlChar*)list->data );
		xmlAddChild( parent, cur );
		list = list->next;
	}

	return parent;
}

/**
 * Writes configuration from configuration structure into file
 *
 *
 */
// int write_config_section( char* name, FILE* stream, struct llist* l ) {
	// if ( NULL != l && l->first ) {
	// 	l->current = l->first;

	// 	fprintf( stream, "%s:\n", name );

	// 	while( l->current ) {
	// 		fprintf( stream, " - %s\n", l->current->as_string( l->current ) );
	// 		l->current = l->current->next;
	// 	}
	// }

// 	return 0;
// }

/**
 * Mark start of delete configuration action
 *
 */
// int start_del( struct llist* l ) {
	// if ( l && l->first ) {
	// 	printf(
	// 		"Type a number of a record to be deleted\n"
	// 		"To save changes type 's', to discard changes type 'q'\n"
	// 	);

	// 	print_del_list( l );
		
	// } else {
	// 	printf( "List is empty\n" );

	// 	return 1;
	// }

// 	return 0;
// }

/**
 * Prints list of configuration list that can be deleted
 *
 */
// int print_del_list( struct llist* l ) {
	// if ( NULL == l || NULL == l->first ) return 1;

	// l->current = l->first;

	// while( l->current && l->current->name ) {
	// 	if ( NULL != temp && NULL != temp->first && 0 == temp->has_value( l->current->name, temp ) ) {
	// 		printf( "*" );

	// 	} else {
	// 		printf( " " );
	// 	}

	// 	printf( "[%.2s] - %s\n", l->current->name, l->current->as_string( l->current ) );
	// 	l->current = l->current->next;
	// }

	// printf( "Remove item >> " );

// 	return 0;
// }

/**
 * Marks start of add configuration action
 *
 */
// int start_add( struct llist* l ) {
	// printf( "Type in one item at line\n" );
	// printf( "To save changes print 's', to discard changes - 'q'\n" );

	// if ( NULL != l->first ) {
	// 	printf( "Existing items:\n" );
	// 	l->print( l );

	// } else {
	// 	printf( "List is empty\n" );
	// }

	// printf( "Add item >> " );

// 	return 0;
// }

/**
 * Adds item to configuration list
 *
 */
// int add_to(  char *item ) {
	// if ( NULL != temp ) {
	// 	printf( "Add item >> " );
	// 	return temp->add( NULL, item, temp );
	// }

	// fprintf( stderr, "Failed to add item to list: list is missing" );

// 	return 1;
// }

/**
 * Deletes item from configuration list
 *
 */
// int del_from(  char *name, struct llist* l ) {
	// if ( NULL != temp ) {
	// 	temp->add( NULL, name, temp );
	// 	printf( "\033[%dA", l->count( l ) + 1 );
	// 	printf( "\033[100D" );
	// 	print_del_list( l );
	// 	printf( "\033[K" );
	// }

// 	return 0;
// }

/**
 * Prints out a list of available commands
 *
 */
// int show_commands() {
	// char *line = malloc( win_size.ws_col + 1 );
	// memset( line, '-', win_size.ws_col );
	// line[ win_size.ws_col ] = '\0';

	// printf(
	// 	"%s\n" /* separator */

	// 	"%2d - Add included directories\n"
	// 	"%2d - Add excluded directories\n"
	// 	"%2d - Delete included directories\n"
	// 	"%2d - Delete excluded directories\n"

	// 	"%2d - Add included files\n"
	// 	"%2d - Add excluded files\n"
	// 	"%2d - Delete included files\n"
	// 	"%2d - Delete excluded files\n"

	// 	"%2d - Add included regexp\n"
	// 	"%2d - Add excluded regexp\n"
	// 	"%2d - Delete included regexp\n"
	// 	"%2d - Delete excluded regexp\n"

	// 	"%2d - Iterate over FS\n"
	// 	"%2d - Print files\n"
	// 	"%2d - Print configurations\n"
	// 	"%2d - Make package\n"

	// 	"%2d - Set module name\n"
	// 	"%2d - Set package major number\n"
	// 	"%2d - Set package minor number\n"
	// 	"%2d - Set package patch number\n"

	// 	"%s\n" /* Separator */

	// 	"Make your choice > ",

	// 	line,

	// 	C_ADD_INCL_FOLDER,
	// 	C_ADD_EXCL_FOLDER,
	// 	C_DEL_INCL_FOLDER,
	// 	C_DEL_EXCL_FOLDER,

	// 	C_ADD_INCL_FILE,
	// 	C_ADD_EXCL_FILE,
	// 	C_DEL_INCL_FILE,
	// 	C_DEL_EXCL_FILE,

	// 	C_ADD_INCL_REGEXP,
	// 	C_ADD_EXCL_REGEXP,
	// 	C_DEL_INCL_REGEXP,
	// 	C_DEL_EXCL_REGEXP,

	// 	C_ITERATE,
	// 	C_PRINT_FILES,
	// 	C_PRINT_CONFIG,
	// 	C_MAKE,

	// 	C_SET_NAME,
	// 	C_SET_MAJOR,
	// 	C_SET_MINOR,
	// 	C_SET_PATCH,

	// 	line
	// );

	// free( line );
// }

/**
 * Action switcher after action confirmation action (eg save, delete)
 *
 */
// int confirmed_operation() {
	// switch ( command ) {
	// case C_ADD_INCL_FOLDER:
	// 	add_to_config( include_dir );
	// 	break;
	// case C_DEL_INCL_FOLDER:
	// 	remove_from_config( include_dir );
	// 	break;
	// case C_ADD_INCL_FILE:
	// 	add_to_config( include_file );
	// 	break;
	// case C_DEL_INCL_FILE:
	// 	remove_from_config( include_file );
	// 	break;
	// case C_ADD_INCL_REGEXP:
	// 	add_to_config( include_regexp );
	// 	break;
	// case C_DEL_INCL_REGEXP:
	// 	remove_from_config( include_regexp );
	// 	break;
	// case C_ADD_EXCL_FOLDER:
	// 	add_to_config( exclude_dir );
	// 	break;
	// case C_DEL_EXCL_FOLDER:
	// 	remove_from_config( exclude_dir );
	// 	break;
	// case C_ADD_EXCL_FILE:
	// 	add_to_config( exclude_file );
	// 	break;
	// case C_DEL_EXCL_FILE:
	// 	remove_from_config( exclude_file );
	// 	break;
	// case C_ADD_EXCL_REGEXP:
	// 	add_to_config( exclude_regexp );
	// 	break;
	// case C_DEL_EXCL_REGEXP:
	// 	remove_from_config( exclude_regexp );
	// 	break;
	// default:
	// 	print_error( "Invalid command: %d\n", command );
	// }

// 	return 0;
// }

/**
 * Merges config list with temporary configuration list
 *
 */
// int add_to_config( struct llist* l ) {
	// l->merge( temp, l );
	// config_is_dirty = 1;

// 	return 0;
// }

/**
 * Removes items from configuration list, which present in temporary configuration list
 *
 */
// int remove_from_config( struct llist* l ) {
	// if ( NULL != l && NULL != l->first ) {
	// 	l->current = l->first;

	// 	while( NULL != l->current ) {
	// 		if ( 0 == temp->has_value( l->current->name, temp ) ) {
	// 			l->remove( l->current->name, l );

	// 			if ( NULL == l->current ) {
	// 				break;

	// 			} else {
	// 				l->current = l->first;
	// 				continue;
	// 			}
	// 		}

	// 		l->current = l->current->next;
	// 	}

	// 	config_is_dirty = 1;
	// }

// 	return 0;
// }

/**
 * Mark operation add ended
 *
 */
// int end_operation() {
	// temp->empty( temp );
	// wait_confirm = 0;
	// command = 0;
	// save_me = 0;

// 	return 0;
// }

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

/**
 * Interrupt signal handler
 *
 */
void int_handler( int s ) {
	if ( s == SIGINT ) {
		printf( "\nInterrupted\n" );
		// save_config();
		exit( 2 );
	}
}

/**
 * Checks whether or not include file in files list
 * Firstly checks files to be present in files include list. If present - check
 * Then checks if file is present in excluded files list. If present - fail
 * Then checks if path in included folders list is longer than path in excluded folders list. If so - check
 * Then checks if path is present in excluded folders list. If so - fail
 * Then checks if path is present in included regex list and is not present i excluded regex list. Check
 * Default: fail
 */
gboolean check_file( char *name ) {
	int debug = 0;

	if ( DEBUG || debug )fprintf( stderr, "Start checking file: '%s'\n", name );

	int max_incl_dir_length = 0;
	int max_excl_dir_length = 0;

	char *relative_file_name = name[ strlen( cwd ) ];
	char *relative_folder_name;

	if ( DEBUG )printf( "Relative file name: '%s'\n", relative_file_name );

	// File is in the include list
	if ( NULL != g_slist_find( include_file_temp, relative_file_name ) ) {
		if ( DEBUG || debug )fprintf( stderr, "Is in included files list\n" );
		return TRUE;
	}

	// File is in the exclude list
	if (  NULL != g_slist_find( exclude_file_temp, relative_file_name ) ) {
		if ( DEBUG || debug )fprintf( stderr, "Is in excluded files list\n" );
		return FALSE;
	}

	relative_folder_name = g_path_get_dirname( relative_file_name );

	if ( DEBUG )printf( "Relative folder: '%s'\n" );

	// If path has folder part
	if ( '.' != relative_folder_name ) {
		if ( DEBUG || debug )fprintf( stderr, "Check as directory: '%s'\n", relative_folder_name );

		collide_length( relative_folder_name, include_folder_temp, &max_incl_dir_length );
		collide_length( relative_folder_name, exclude_folder_temp, &max_excl_dir_length );

		g_free( relative_folder_name );

		if ( DEBUG || debug )
			fprintf(
				stderr,
				"Included directory max. length: %d, excluded directory max. length: %d\n",
				max_incl_dir_legth,
				max_excl_dir_length
			);
		
		if ( max_incl_dir_length > 0 && max_incl_dir_length >= max_excl_dir_length ) {
			if ( DEBUG || debug )fprintf( stderr, "Passed as directory\n");
			return TRUE;
		}

		if ( max_excl_dir > 0 ) {
			if ( DEBUG || debug )fprintf( stderr, "Rejected as directory\n" );
			return FALSE;
		}

	} else {
		g_free( relative_folder_name );
	}

	if ( DEBUG || debug )fprintf( stderr, "Check against regex\n");

	if ( TRUE == check_regexp( name, include_regex_temp ) && FALSE == check_regexp( name, exclude_regex_temp ) ) {
		if ( DEBUG || debug )fprintf( stderr, "Passed as regex\n");

		return TRUE;
	}

	if ( DEBUG || debug )fprintf( stderr, "No filter rules applied. Skip\n");

	return FALSE;
}

/**
 * Returns longest path
 */
// void collide_length( char *name, GSList *list, int *max ) {
	// int current_str_len = 0;
	// *max = 0;
	// GSList *curret = list;

	// while ( NULL != current ) {
	// 	current_str_len = strspn( name, current->data );

	// 	// We already have longer match. Skip
	// 	if ( *max < current_str_len ) {
	// 		*max = current_str_len;
	// 	}

	// 	current = current->next;
	// }
// }

/**
 * Calculates strings' intersection length
 */
// gboolean collide_span(  char *h,  char *n ) {
// 	int i;
// 	int c = 0;
// 	int nl = strlen( n );
// 	int hl = strlen( h );

// 	if ( hl >= nl ) {
// 		for( i = 0; i < nl; i++ ) {
// 			if ( h[ i ] == n[ i ] ) {
// 				c++;

// 			} else {
// 				break;
// 			}
// 		}
// 	}

// 	return c == nl ? 0 : 1;
// }

/**
 * Tests if regular expression matches the pattern
 * str - pointer to string to test
 * pattern - pointer to the pattern
 * m - array of regmatch_t structures
 * Returns 0 if match succeeded
 */
// int match(  char* str,  char* pattern, regmatch_t *m, int flags ) {
	// size_t c = NULL == m ? 0 : REGEX_MATCH_COUNT;
	// int status;

	// if ( DEBUG )fprintf( stderr, "Matching string '%s' against regex '%s'\n", str, pattern );

	// if ( IS_EMPTY( str ) || IS_EMPTY( pattern ) ) {
	// 	if ( DEBUG )fprintf( stderr, "One of operands is empty. Skip\n" );
	// 	return 0;
	// }

	// regex_t *compilled = (regex_t*)malloc( sizeof ( regex_t ) );
	// flags |= REG_EXTENDED;

	// if ( NULL == compilled ) {
	// 	print_error( "Failed to locate memory for regex_t structure" );
	// }

	// status = regcomp( compilled, pattern, flags );

	// if ( 0 != status ) {
	// 	print_error( get_regerror( status, compilled ) );
	// }

	// status = regexec( compilled, str, c, m, 0 );

	// if ( REG_ESPACE == status ) {
	// 	print_error( get_regerror( status, compilled ) );
	// }

	// regfree( compilled );
	// free( compilled );

	// if ( DEBUG )fprintf( stderr, "Match status: %d\n", status );

// 	return status;
// }

/**
 * Returns error from regex library
 *
 */
// char *get_regerror ( int errcode, regex_t *compiled ) {
// 	size_t length = regerror ( errcode, compiled, NULL, 0 );
// 	char *buffer = malloc ( length );

// 	if ( NULL == buffer ) {
// 		print_error( "Failed to allocate memory for regular expression error message" );
// 	}

// 	( void )regerror ( errcode, compiled, buffer, length );

// 	return buffer;
// }

/**
 * Checks path against list of regexps
 *
 */
gboolean check_regexp( char* str, GSList *list ) {
	GSList *current = list;

	if ( DEBUG )fprintf( stderr, "Checking file '%s' name against regex\n", str );

	while ( NULL != current ) {
		if ( DEBUG )fprintf( stderr, "Regex: '%s'\n", (char*)current->data );

		if ( g_regex_match_simple ( current->data, str, 0, 0 ) ) {
			if ( DEBUG )fprintf( stderr, "Match\n" );

			return TRUE;
		}

		current = current->next;
	}

	if ( DEBUG )fprintf( stderr, "No match found\n" );

	return FALSE;
}

/**
 * Starts clock measurements
 *
 */
void start_clock() {
    st_time = times( &st_cpu );
}

/**
 * Ends clock measurements and print out the result
 *
 */
void end_clock( char *msg ) {
    en_time = times( &en_cpu );

    if ( clockticks == 0 ) {
    	if ( 0 > ( clockticks = sysconf( _SC_CLK_TCK ) ) ) {
    		print_error( "Failed to fetch system clock ticks" );
    	}
    }

    printf( "%-30.30s: ", msg );
    printf(
    	"Real Time: %4.2f, User Time %4.2f, System Time %4.2f\n",
        ( en_time - st_time ) / (float)clockticks,
        ( en_cpu.tms_utime - st_cpu.tms_utime ) / (float)clockticks,
        ( en_cpu.tms_stime - st_cpu.tms_stime ) / (float)clockticks
    );
}

/**
 * Prints out contents of configuration file
 *
 */
// int print_config() {
	// FILE *stream = fopen( config_name, "r" );
	// char buffer[ MAX_LINE ];

	// if ( NULL != stream ) {
	// 	while ( NULL != fgets( buffer, MAX_LINE, stream ) ) {
	// 		if ( EOF == fputs( buffer, stdout ) ) {
	// 			perror( "Print configuration" );
	// 			exit( 1 );
	// 		}
	// 	}

	// 	if ( ferror( stream ) ) {
	// 		perror( "Print configuration" );
	// 		exit( 1 );
	// 	}

	// 	fclose( stream );
	// }

// 	return 0;
// }

/**
 * Print in-memory list of package files
 *
 */
// int print_files() {
	// if ( NULL == files->first ) {
	// 	printf( "List is empty\n" );

	// } else {
	// 	files->current = files->first;

	// 	while ( files->current ) {
	// 		printf( "[%3d] - %s\n", files->current->index, files->current->name );
	// 		files->current = files->current->next;
	// 	}

	// 	files->current = files->first;
	// }

// 	return 0;
// }

/**
 * Populates static variables with current terminal window sizes
 *
 */
static void set_winsize() {
	if ( 0 == isatty( STDIN_FILENO ) ) {
		print_error( "STDIN is not a terminal device" );
	}

	if ( ioctl( STDIN_FILENO, TIOCGWINSZ, (char *)&win_size ) < 0 ) {
		print_error( "TIOCGWINSZ error" );
	}

	// printf( "%d rows, %d columns\n", win_size.ws_row, win_size.ws_col );
}

/**
 * Terminal window change size signal handler
 *
 */
static void sig_winch( int signo ) {
	set_winsize();
}

/**
 * SIGCHL handler
 *
 */
void sig_cld( int signo ) {
	int status;
	pid_t pid;

	if ( ( pid = wait( &status ) ) < 0 ) {
		print_error( "Wait child error\n" );
	}

	printf( "Child with PID %d was terminated with status code %d\n", pid, status );
}

/**
 * Loads dependency files form source header sections
 * CHDIR to CWD
 *
 */
int load_dependencies() {
	int debug = 0;

	if ( DEBUG || debug )fprintf( stderr, "Start loading dependencies\n" );

	FILE *f;
	char line[ path_max_size ];
	char name[ path_max_size ];
	int max_line = 100; // Max depth to look at
	int in_header = 0;
	int c = 0; // Line count
	char *patt = "\\*\\s+@source\\s+([^*]+)"; // Regex pattern to match source against
	int reg_len;
	temp->empty( temp );
	char t_line[ path_max_size ];

	if ( -1 == chdir( cwd ) ) {
		fprintf( stderr, "run_filters: failed to CHDIR to '%s'\n", cwd );
		exit( 1 );
	}

	// Files not empty
	if ( files->first ) {
		files->current = files->first;

		while ( files->current ) {
			if ( DEBUG || debug )fprintf( stderr, "Iterate: '%s'\n", files->current->name );

			// Look for sources in controller files
			if ( match( files->current->name, "/controller/", NULL, 0 ) == 0 ) {
				if ( DEBUG || debug )fprintf( stderr, "Is controller\n" );

				if ( NULL == ( f = fopen( files->current->value, "r" ) ) ) {
					print_error( "Failed to open file '%s' for fetching dependencies: %s\n", files->current->name, strerror( errno ) );
				}

				if ( DEBUG )fprintf( stderr, "Open file: '%s'\n", files->current->as_string( files->current ) );

				// Read up to max_line lines
				while ( NULL != ( fgets( line, MAX_LINE, f ) ) ) {
					if ( DEBUG || debug )fprintf( stderr, "Line: '%s'", line );

					if ( c > max_line ) {
						fprintf( stderr, "Maximum depth of %d lines reached in file %s\n", max_line, files->current->as_string( files->current ) );
						goto next;
					}

					if ( in_header ) {
						trim( line, NULL );

						if ( DEBUG || debug )fprintf( stderr, "After trim: '%s'\n", line );

						if ( 0 == strncmp( line, "*/", 2 ) ) {
							if ( DEBUG || debug )fprintf( stderr, "Header closing tag\n" );
							goto next;
						}

						if ( 0 == match( line, patt, m, 0 ) ) {
							if ( -1 != m[ 1 ].rm_so ) {
								memset( t_line, '\0', path_max_size );
								reg_len = m[ 1 ].rm_eo - m[ 1 ].rm_so;
								strncpy( t_line, &line[ m[ 1 ].rm_so ], reg_len );

								if ( DEBUG || debug )fprintf( stderr, "Got source: '%s'\n", t_line );

								iterate( t_line, &check_source, NULL, &on_iterate_error );

							} else {
								fprintf( stderr, "Error while matching string '%s' against regex '%s'\n", line, patt );
								exit( 1 );
							}
						}

					} else {
						if ( 0 == strncmp( ltrim( line, NULL ), "/**", 3 ) ) {
							if ( DEBUG || debug )fprintf( stderr, "Header opening tag\n" );
							in_header = 1;
							continue;
						}
					}

					c++;
				}

				if ( ferror( f ) ) {
					print_error( "Failed to read file '%s' for fetching dependencies: %s\n", files->current->name, strerror( errno ) ); 
				}

			next:
				fclose( f );
				in_header = 0;
			}

			if ( DEBUG || debug ) {
				fprintf( stderr, "Next loop\n" );
				fprintf( stderr, "Next item: %p\n", files->current->next );
			}

			files->current = files->current->next;
		}

		files->merge( temp, files );

	} else {
		if ( DEBUG || debug )fprintf( stderr, "Files list is empty\n" );
	}
}

/**
 * Trims the string. Default behavior - trim whitespaces
 *
 */
char *trim( char *str,  char *ch ) {
	return ltrim( rtrim( str, ch ), ch );
}

/**
 * Trims left part of the string. Default behavior - trim whitespaces
 *
 */
char *ltrim( char *str,  char *ch ) {
	int len = strlen( str );
	int ch_len = 0;
	int i = 0;
	int y = 0;
	int flag = 1;
	char *t_str;

	if ( IS_EMPTY( str ) )return str;

	if ( NULL != ch ) {
		ch_len = strlen( ch );
	}

	for( i = 0; i < len && flag; i++ ) {
		flag = 0;

		if ( NULL == ch ) {
			if ( str[ i ] <= ' ' ) {
				flag = 1;
			}

		} else {
			for ( y = 0; y < ch_len; y++ ) {
				if ( str[ i ] == ch[ y ] ) {
					flag = 1;
					break;
				}
			}
		}
	}

	i--;
	t_str = malloc( len - i + 1 );

	if ( NULL == t_str ) {
		print_error( "Failed to allocate memory for temp string in ltrim function\n" );
	}

	memset( t_str, '\0', len - i + 1 );
	strncpy( t_str, &str[ i ], len - i );
	strncpy( str, t_str, len - 1 + 1 );
	free( t_str );

	return str;
}

/**
 * Trims right part of the string. Default behavior - trim whitespaces
 *
 */
char *rtrim( char *str,  char *ch ) {
	int len = strlen( str );
	int ch_len = 0;
	int i = 0;
	int y = 0;
	int flag = 1;
	char *p;

	if ( IS_EMPTY( str ) )return str;

	if ( NULL != ch ) {
		ch_len = strlen( ch );
	}

	for( p = &str[ len -1 ]; len >= 0 && flag; len--, p-- ) {
		flag = 0;

		if ( NULL == ch ) {
			if ( *p <= ' ' ) {
				flag = 1;
			}

		} else {
			for ( y = 0; y < ch_len; y++ ) {
				if ( *p == ch[ y ] ) {
					flag = 1;
					break;
				}
			}
		}
	}

	*( p + 2 ) = '\0';

	return str;
}

/**
 * Checks if entry is a regular file
 *
 */
int is_file(  char *name ) {
	struct stat stat_buffer;

	if ( lstat( name, &stat_buffer ) < 0 ) {
		return 0;
	}

	return  S_ISREG( stat_buffer.st_mode );
}

/**
 * Checks if an entry is a directory
 *
 */
int is_dir(  char *name ) {
	struct stat stat_buffer;

	if ( lstat( name, &stat_buffer ) < 0 ) {
		return 0;
	}

	return  S_ISDIR( stat_buffer.st_mode );
}

/**
 * Prepends CWD to the path
 *
 */
char* add_cwd( char* path ) {
	char t[ path_max_size ];
	// memset( t, '\0', path_max_size );
	strcpy( t, cwd );
	strcat( t, "/" );
	strcat( t, path );
	strcpy( path, t );

	return path;
}

/**
 * Makes package
 * CHDIR to CWD
 */
int make_package() { 
	int s;
	char u_folder[ path_max_size ];
	char pckg_name[ path_max_size ];
	char *pckg_dir;
	char version[ VERSION_SIZE ];

	if ( NULL == files->first ) {
		fprintf( stderr, "Files list is empty. Nothing to process\n" );
		return 1;
	}

	if ( '\0' == code[ 0 ] ) {
		fprintf( stderr, "Extension name is mandatory\n" );
		return 1;
	}

	if ( -1 == chdir( cwd ) ) {
		fprintf( stderr, "run_filters: failed to CHDIR to '%s'\n", cwd );
		exit( 1 );
	}

	memset( pckg_name, '\0', path_max_size );
	memset( version, '\0', VERSION_SIZE );

	if ( 0 != get_version() ) {
		return 1;
	}

	// Directory to store packages in
	pckg_dir = get_package_dir();

	if ( make_dir( pckg_dir, 0775 ) < 0 ) {
		fprintf( stderr, "make_package: Failed to create folder '%s': %s\n", pckg_dir, strerror( errno ) );
		exit( 1 );
	}

	// Upload folder of temporary package structure
	memset( u_folder, '\0', path_max_size );
	strcpy( u_folder, pckg_tmp_dir );
	strcat( u_folder, upload_folder );

	if ( ( s = make_dir( u_folder, S_IRWXU | S_IRWXG | S_IRWXO ) ) < 0 ) {
		fprintf( stderr, "make_package: Failed to create folder '%s': %s\n", u_folder, strerror( errno ) );
		exit( 1 );
	}

	start_clock();
	fill_temp_package();
	end_clock( "Fill in temp structure" );

	start_clock();
	make_vqmod();
	end_clock( "VQMOD" );

	start_clock();
	run_filters();
	end_clock( "Filters" );

	start_clock();
	save_translation( "admin", admin_t );
	end_clock( "Translation admin" );

	start_clock();
	save_translation( "catalog", catalog_t );
	end_clock( "Translation catalog" );

	start_clock();
	php_lint();
	end_clock( "php lint" );

	sprintf( pckg_name, pckg_name_templ, code, "OC23", major, minor, patch );
	run_zip( pckg_name );

	make_oc20();
	sprintf( pckg_name, pckg_name_templ, code, "OC20", major, minor, patch );
	run_zip( pckg_name );

	free( pckg_dir );

	fprintf( stderr, "Package was saved under version: %d.%d.%d\n", major, minor, patch );

	return 0;
}

/**
 * Returns path to folder where zipped packages stored with respect to package name
 *
 */
char *get_package_dir() {
	char *pckg_dir = (char*)malloc( path_max_size );

	if ( NULL == pckg_dir ) {
		print_error( "get_package_dir: failed to allocate memory" );
	}

	memset( pckg_dir, '\0', path_max_size );
	strcpy( pckg_dir, crawler_storage_dir );
	strcat( pckg_dir, code );
	strcat( pckg_dir, "/" );

	return pckg_dir;
}

/**
 * Get next version numbers for the package. Gets values form the config and checks them against
 * saved packages
 *
 */
int get_version() {
	int debug = 0;

	int f_major = 0;
	int f_minor = 0;
	int f_patch = 0;

	int t_major = 0;
	int t_minor = 0;
	int t_patch = 0;

	int is_empty_dir = 1;

	char regex[ MAX_LINE ];
	char *pckg_dir;
	char *temp;

	struct llist *matches;

	DIR *dir;
	struct dirent *entry;

	pckg_dir = get_package_dir();

	if ( NULL != ( dir = opendir( pckg_dir ) ) ) {
		sprintf( regex, pckg_mane_regex, code );

		while ( NULL != ( entry = readdir( dir ) ) ) {
			if ( entry->d_name[ 0 ] == '.' ) continue;

			if ( DEBUG || debug ) fprintf( stderr, "Processing file '%s'\n", entry->d_name );
			if ( DEBUG || debug ) fprintf( stderr, "Regex '%s'\n", regex );

			if ( 0 == match( entry->d_name, regex, m, 0 ) ) {
				if ( DEBUG || debug ) fprintf( stderr, "Match\n" );

				is_empty_dir = 0;

				matches = get_matches( entry->d_name );

				if ( DEBUG || debug ) {
					temp = matches->fetch( "0", matches );
					if ( NULL != temp ) {
						fprintf( stderr, "Full match: '%s'\n", temp );
						free( temp );
					}
				}

				temp = matches->fetch( "1", matches );

				if ( NULL != temp ) {
					t_major = atoi( temp );
					free( temp );
				}

				temp = matches->fetch( "2", matches );

				if ( NULL != temp ) {
					t_minor = atoi( temp );
					free( temp );
				}

				temp = matches->fetch( "3", matches );

				if ( NULL != temp ) {
					t_patch = atoi( temp );
					free( temp );
				}

				if ( DEBUG || debug ) fprintf( stderr, "Version numbers found: %d.%d.%d\n", t_major, t_minor, t_patch );

				if ( t_major > f_major ) {
					f_major = t_major;
					f_minor = t_minor;
					f_patch = t_patch;

				} else if ( t_major == f_major && t_minor > f_minor ) {
					f_minor = t_minor;
					f_patch = t_patch;

				} else if ( t_major == f_major && t_minor == f_minor && t_patch > f_patch ) {
					f_patch = t_patch;
				}

				free( matches );
			}
		}

		if ( major == f_major && minor == f_minor && patch == f_patch && 0 == is_empty_dir) {
			if ( DEBUG || debug ) fprintf( stderr, "Patch number was automatically incremented\n" );

			patch++;
			config_is_dirty = 1;
		}

	} else if ( DEBUG || debug ) {
		fprintf( stderr, "Package directory '%s' doesn't exist\n", pckg_dir );
	}

	if ( DEBUG || debug ) fprintf( stderr, "Version: %d.%d.%d\n", f_major, f_minor, f_patch );

	// Patch number may be equal in case of first release, version will be 0.0.0
	if ( major <= f_major && minor <= f_minor && patch < f_patch ) {
		fprintf( stderr, "Can not create package with version (%d.%d.%d) that is less then existing one(%d.%d.%d)\n", major, minor, patch, f_major, f_minor, f_patch );

		return 1;
	}

	free( pckg_dir );

	return 0;
}

/**
 * Spawns ZIP to make zipped package
 *
 */
int run_zip( char *package_name ) {
	int debug = 0;

	char src_path[ path_max_size ];
	char mode[] = "-q";
	char *path;

	pid_t pid;

	if ( DEBUG || debug ) fprintf( stderr, "ZIPpping package '%s'....\n", package_name );

	start_clock();

	if ( ( pid = fork() ) < 0 ) {
		print_error( "Failed to fork process for zip" );

	} else if ( pid > 0 ) {
		if ( DEBUG || debug ) printf( "Child with pid %d was forked\n", pid );
		if ( wait( NULL ) > 0 ) {
			end_clock( "Zipping package for OC23+" );
		}

		return 0;
	}

	strcpy( src_path, pckg_tmp_dir );

	if ( 0 != chdir( cwd ) ) {
		fprintf( stderr, "run_zip: failed to change working directory to '%s': %s", cwd, strerror( errno ) );
		exit( 1 );
	}

	if ( 0 != chdir( pckg_tmp_dir ) ) {
		fprintf( stderr, "run_zip: failed to change working directory to '%s': %s", pckg_tmp_dir, strerror( errno ) );
		exit( 1 );
	}

	if ( DEBUG || debug ) {
		printf( "Changing CWD to '%s'\n", pckg_tmp_dir );
		mode[ 1 ] = 'v';
	}

	path = get_package_dir();
	strcat( path, package_name );

	if ( DEBUG || debug ) fprintf( stderr, "Creating ZIP archive... '%s'\n", path );

	// ZIP everything that are in CWD and save as path
	if ( execlp( "zip", "zip", "-r", mode, path, ".", (char*)0 ) < 0 ) {
		print_error( "run_zip: Failed to exec zip" );
	}
}

/**
 * mkdir wrapper which makes directories recursively
 * Set errno as mkdir doest it
 * returns 0 on success -1 on error
 */
int make_dir( char *path, mode_t mode ) {
	int debug = 0;

	if ( DEBUG || debug ) fprintf( stderr, "Making directory '%s'...\n", path );

	char t_path[ path_max_size ];
	char t_cwd[ path_max_size ];
	char *pos, *cur_pos;
	char part[ path_max_size ];

	int status = 0;

	// CWD
	memset( t_cwd, '\0', path_max_size );
	getcwd( t_cwd, path_max_size );

	if ( '/' == path[ 0 ] ) {
		chdir( "/" );
	}

	// Copy of path name without leading and railing slashes
	memset( t_path, '\0', path_max_size );
	strncpy( t_path, path, path_max_size );
	trim( t_path, "/" );

	cur_pos = t_path;

	if ( DEBUG || debug ) printf( "Makedir: Input folder: %s\n", t_path );

	// Iterate over the path till we still have slash in it
	while ( NULL != ( pos = strchr( cur_pos, '/' ) ) ) {
		memset( part, '\0', path_max_size );
		strncpy( part, cur_pos, pos - cur_pos );

		if ( DEBUG || debug ) printf( "Makedir: Creating folder: '%s'\n", part );

		if( ( status = mkdir( part, mode ) ) < 0 && errno != EEXIST ) {
			goto out;
		}

		cur_pos = ++pos;

		// Chdir into newly created directory to create next directory relative to it
		chdir( part );
	}

	// Last part (or the only one)
	memset( part, '\0', path_max_size );
	strncpy( part, cur_pos, &t_path[ strlen( t_path ) ] - cur_pos );

	if ( DEBUG || debug ) printf( "Makedir: Creating folder: '%s'\n", part );

	if( ( status = mkdir( part, mode ) ) < 0 && errno == EEXIST ) {
		status = 0;
		errno = 0;
	}

out:
	chdir( t_cwd );

	return status;
}

/**
 * Fills in temporary package structure before ZIPping
 * Implied CWD
 */
int fill_temp_package() {
	int debug = 0;

	int src, dest, r_count;

	char path[ path_max_size ];
	char buffer[ BUFF_SIZE ];
	char copy[ path_max_size ];
	char readme_file_name[ MAX_LINE ];
	int is_copy = 0;

	struct stat sb;

	memset( copy, '\0', path_max_size );
	memset( readme_file_name, '\0', MAX_LINE );

	strcpy( readme_file_name, code );
	strcat( readme_file_name, "_readme" );

	if ( DEBUG || debug ) fprintf( stderr, "Deleting temporary files..." );

	iterate( pckg_tmp_dir, del_file_cb, del_dir_cb, on_iterate_error );

	files->current = files->first;

	while( files->current ) {
		if ( DEBUG || debug )fprintf( stderr, "Processing file '%s'\n", files->current->name );

		// Get file mode to be able to preserve it
		// TODO: directories in filename will inherit permissions as well
		if ( lstat( files->current->name, &sb ) < 0 ) {
			fprintf( stderr, "Failed to stat file '%s': %s\n", files->current->name, strerror( errno ) );
			return 1;
		}

		if ( ( src = open( files->current->name, O_RDONLY  ) ) < 0 ) {
			fprintf( stderr, "fill_temp_package: Failed to open file '%s': %s\n", files->current->name, strerror( errno ) );
			return 1;
		}

		if ( DEBUG || debug )fprintf( stderr, "Open file '%s'\n", files->current->name );

		memset( path, '\0', path_max_size );
		strcpy( path, pckg_tmp_dir );

		// Place OCMOD in package root
		if ( 0 == strcmp( ".ocmod.xml", &files->current->name[ strlen( files->current->name ) - 10 ] ) ) {
			if ( DEBUG || debug )fprintf( stderr, "OCMOD file detected '%s'\n", files->current->name );

			// Create OCMOD which need to be installed via Extension Installer
			strcpy( copy, path );
			strcat( copy, "install.xml" );

			// Create OCMOD which can be installed directly
			strcat( path, strrchr( files->current->name, '/' ) );

		// Place README file in the package root
		} else if ( strcmp( files->current->name, readme_file_name ) == 0 ) {
			if ( DEBUG || debug ) fprintf( stderr, "Readme file found: '%s'\n", files->current->name );

			strcat( path, "README.TXT" );

		} else {
			strcat( path, upload_folder );
			strcat( path, files->current->name );
		}

	copy:
		if ( DEBUG || debug )fprintf( stderr, "Creating file '%s'\n", path );

		if ( ( dest = make_file( path, sb.st_mode ) ) < 0 ) {
			fprintf( stderr, "fill_temp_package: Failed to save file '%s': %s\n", path, strerror( errno ) );
			return 1;
		}

		if ( DEBUG || debug )fprintf( stderr, "File '%s' created\n", path );

		while ( ( r_count = read( src, buffer, BUFF_SIZE ) ) > 0 ) {
			if ( -1 == write( dest, buffer, r_count ) ) {
				fprintf( stderr, "fill_temp_package: write file error: %s", strerror( errno ) );
				return -1;
			}
		}

		if ( -1 == r_count ) {
			fprintf( stderr, "fill_temp_package: read file error: %s", strerror( errno ) );
			exit( 1 );
		}

		close( dest );

		if ( strlen( copy ) > 0 ) {
			strcpy( path, copy );
			memset( copy, '\0', path_max_size );

			if ( lseek( src, 0, SEEK_SET ) == -1 ) {
				print_error( "fill_temp_package: Failed to rewind source file" );
			}

			goto copy;
		}

		close( src );

		files->current = files->current->next;
	}

	return 0;
}

/**
 * Creates file and directories structure if needed
 * Returns file description on success, -1 on error
 * Modifies errno variable
 */
int make_file( char *name, mode_t mode ) {
	int debug = 0;
	int fd;
	char *d_name;

	if ( DEBUG || debug )fprintf( stderr, "Creating file '%s' with mode %o\n", name, mode );

	if ( ( fd = creat( name, mode ) ) < 0 &&  ENOENT == errno ) {
		errno = 0;

		if ( DEBUG || debug )fprintf( stderr, "Directories structure doesn't exist - creating structure\n" );

		d_name = dir_name( name );

		if ( make_dir( d_name, mode | S_IXUSR | S_IXGRP | S_IXOTH ) < 0 ) {
			free( d_name );
			return -1;
		}

		free( d_name );
		fd = creat( name, mode );
	}

	return fd;
}

/**
 * Returns last part of a string (after last slash) as a copy
 * If no slashes are present returns copy of input string
 * On error returns NULL
 */
char *file_name( char *path ) {
	char *p;
	char *out;
	int len = strlen( path ) + 1;

	out = malloc( len );

	if ( NULL == out ) {
		fprintf( stderr, "file_name: Failed to allocate memory\n" );
		return NULL;
	}

	memset( out, '\0', len );

	p = strrchr( path, '/' );

	if ( NULL == p ) {
		strcpy( out, path );

	} else {
		strcpy( out, ++p );
	}

	return out;
}

/**
 * Returns path name up to the last slash as a copy
 * If no slashes are present returns copy of input string
 * On error returns NULL
 */
char *dir_name( char *path ) {
	int debug = 0;

	char *p;
	char *out;
	int len = strlen( path ) + 1;

	out = malloc( len );

	if ( NULL == out ) {
		fprintf( stderr, "dir_name: Failed to allocate memory\n" );
		return NULL;
	}

	memset( out, '\0', len );

	p = strrchr( path, '/' );

	if ( NULL == p ) {
		strcpy( out, path );

	} else {
		strncpy( out, path, p - &path[ 0 ] );
	}

	if ( DEBUG || debug )fprintf( stderr, "Path '%s' resolved to directory name '%s'\n", path, out );

	return out;
}

/**
 * Scan file and fill in structures of translations
 *
 */
int fill_translation( FILE* f, char *name ) {
	int debug = 0;

	char *p;

	if ( DEBUG || debug ) fprintf( stderr, "File: '%s'\n", name );

	if ( NULL != strstr( name, "/language/" ) ) {
		if ( DEBUG || debug ) fprintf( stderr, "Not language - skip\n" );

		return 0; // Skip language files
	}

	p = strrchr( name, '.' );

	if ( NULL == p || ( strcmp( p, ".php" ) != 0 && strcmp( p, ".xml" ) != 0 && strcmp( p, ".tpl" ) != 0 ) ) {
		if ( DEBUG || debug ) fprintf( stderr, "Wrong type - skip\n" );

		return 0; // Skip some files
	}

	if ( 0 == strncmp( name, "catalog/", 8 ) ) {
		if ( DEBUG || debug ) fprintf( stderr, "Catalog side\n" );

		fetch_translation( f, catalog_t );

	} else if ( 0 == ( strncmp( name, "admin/", 6 ) ) ) {
		if ( DEBUG || debug ) fprintf( stderr, "Admin side\n" );

		fetch_translation( f, admin_t );

	} else {
		if ( DEBUG || debug ) fprintf( stderr, "Common side\n" );

		fetch_translation( f, common_t );
	}

	return 0;
}

/**
 * Scans file pointed to by f and fills in structure l
 *
 */
int fetch_translation( FILE* f, struct llist* l ) {
	int debug = 0;

	int i;
	size_t tr_len;

	char line[ MAX_LINE ];
	char *tr;
	char *translation;
	char *start;
	char *end;

	struct llist* matches;

	rewind( f ); 

	while( NULL != fgets( line, MAX_LINE, f ) ) {
		if ( DEBUG || debug ) fprintf( stderr, "Processing line '%s'", line );

		if ( 0 == match( line, "__\\(([^)]+)\\)", m, 0 ) ) {
			if ( DEBUG || debug ) fprintf( stderr, "Match found\n" );

			matches = get_matches( line );

			if ( 0 == matches->get( "1", &tr, matches ) ) {
				tr = trim( tr, NULL );

				if ( tr[ 0 ] == '\'' || tr[ 0 ] == '"' ) {
					start = &tr[ 0 ];
					tr_len = strlen( tr );

					for ( i = 2; i < tr_len; i++ ) {
						if ( tr[ i ] == *start && tr[ i - 1 ] != '\\' ) {
							end = &tr[ i ];
							*end = '\0';
							l->add( NULL, ++start, l );
						}
					}
				}
			}

			matches->empty( matches );
			free( matches );
		}
	}

	if ( ferror( f ) ) {
		fprintf( stderr, "fetch_translation: read file error: %s\n", strerror( errno ) );
		exit( 1 );
	}

	return 0;
}

int php_lint() {
	char path[ MAX_LINE ];

	strcpy( path, pckg_tmp_dir );
	strcat( path, upload_folder );

	iterate( path, php_lint_cb, NULL, on_iterate_error );

	return 0;
}

int php_lint_cb( char *name, struct stat *sb ) {
	char cmd[ MAX_LINE + 7 ];
	int code;
	char *ext;

	ext = strrchr( name, '.' );

	// If file is not php, tpl or xml  - skip it
	if (
		NULL == ext
		||
		(
			strncmp( ext, ".php", 4 ) != 0 &&
			strncmp( ext, ".tpl", 4 ) != 0 &&
			strncmp( ext, ".xml", 4 ) != 0
		)
	) return 0;

	memset( cmd, '\0', MAX_LINE + 7 );
	strcpy( cmd, "php -l " );
	strcat( cmd, name );

	code = system( cmd );

	if ( -1 == code ) {
		fprintf( stderr, "php_lint: error: %s\n", strerror( errno ) );
		exit( 1 );
	}

	if ( 127 == code ) {
		fprintf( stderr, "php_lint: failed to execute php -l\n" );
		exit( 1 );
	}

	if ( 0 != code ) {
		fprintf( stderr, "php_lint: Failed to lint file %s\n", name );
		exit( 1 );
	}

	return 0;
}

// TODO: implement catalog translation maybe
/**
* Saves translation file into temporary directory
* name - admin or catalog
* l - list off all the translation for specific side
*
* CHDIR to CWD
*/
int save_translation( char *name, struct llist *l ) {
	int debug = 0;

	if ( DEBUG || debug ) fprintf( stderr, "Saving translations for %s...\n", name );

	int LEN = 1000; // Presume translation can have such length

	FILE *from;
	FILE *to;

	char buff[ LEN ];
	char from_name[ path_max_size ];
	char to_name[ path_max_size ];

	if ( -1 == chdir( cwd ) ) {
		fprintf( stderr, "run_filters: failed to CHDIR to '%s'\n", cwd );
		exit( 1 );
	}

	l->merge( common_t, l );

	if ( DEBUG || debug ) fprintf( stderr, "%s translation merged\n", name );

	strcpy( from_name, name );               // admin
	strcat( from_name, "/language/en-gb/" ); // admin/language/en-gb/
	strcat( from_name, get_common_dir() );   // admin/language/en-gb/extension/module/

	strcpy( to_name, pckg_tmp_dir );         // .temp_dir/
	strcat( to_name, upload_folder );        // .temp_dir/upload/
	strcat( to_name, from_name );            // .temp_dir/upload/admin/language/en-gb/extension/module/

	// If folder doesn't exist
	make_dir( to_name, 0775 );

	strcat( from_name, code );              // admin/language/en-gb/extension/module/extension_name
	strcat( to_name, code );				// .temp_dir/upload/admin/language/en-gb/extension/module/extension_name
	strcat( from_name, ".php" );            // admin/language/en-gb/extension/module/extension_name.php
	strcat( to_name, ".php" );				// .temp_dir/upload/admin/language/en-gb/extension/module/extension_name.php

	files->add( from_name, from_name, files );

	if ( DEBUG || debug ) {
		fprintf( stderr, "Source path: '%s'\nTarget path: '%s'\nCWD: '%s'\n", from_name, to_name, cwd );
	}

	if ( NULL == ( to = fopen( to_name, "w+" ) ) ) {
		fprintf( stderr, "save_translation: failed to open file '%s': %s\n", to_name, strerror( errno ) );
		exit( 1 );
	}

	fputs( "<?php\n", to );

	if ( DEBUG || debug ) fprintf( stderr, "File '%s' opened\n", to_name );

	if ( NULL != ( from  = fopen( from_name, "r" ) ) ) {
		if ( DEBUG || debug ) fprintf( stderr, "File '%s' opened\n", from_name );

		while ( NULL != fgets( buff, LEN, from ) ) {

			// Save predefined translations
			if (
				1 == match( buff, "\\$_\\[\\s*'\\s*heading_title\\s*'", NULL, 0 ) &&
				1 == match( buff, "\\$_\\[\\s*'\\s*text_advertikon_stripe\\s*'", NULL, 0 )
			) {
				continue;
			}

			if ( EOF == fputs( buff, to ) ) {
				fprintf( stderr, "save_translation: file '%s' writing error: %s\n",to_name, strerror( errno ) );
				exit( 1 );
			}
		}

		if ( ferror( from ) ) {
			fprintf( stderr, "save_translation: file '%s' reading error: %s\n", from_name, strerror( errno ) );
			exit( 1 );
		}

	} else if ( ENOENT != errno ) {
		fprintf( stderr, "save_translation: failed to open file '%s': %s\n", from_name, strerror( errno ) );
		exit( 1 );
	}

	if ( NULL != l->first ) {
		if ( DEBUG || debug ) fprintf( stderr, "Writing translations....\n" );

		if ( debug ) l->print( l );

		l->current = l->first;

		while( l->current ) {
			if ( debug ) fprintf( stderr, "$_['%1$s'] = '%1$s';\n", (char*)l->current->value );

			sprintf( buff, "$_['%1$s'] = '%1$s';\n", (char*)l->current->value );
			fputs( buff, to );
			l->current = l->current->next;
		}
	}

	if ( debug ) {
		fprintf( stderr, "From FILE: %p, to FILE %p\n", from, to );
	}

	if ( NULL != from ) {
		fclose( from );
	}

	fclose( to );

	if ( debug ) fprintf( stderr, "Exit save translation\n" );

	return 0;
}

/**
 * Runs all registered filters on each package files in turn
 * CHDIR to temp folder and then back to CWD
 */
int run_filters() {
	int debug = 0;

	FILE *f;
	char path[ path_max_size ];
	char *ext;

	if ( DEBUG || debug ) fprintf( stderr, "Start filtering...\n" );

	if ( NULL == filters ) {
		init_filters();
	}

	// There are no files to process upon or there are no filters to process with
	if ( NULL == files->first || NULL == filters->first ) {
		if ( DEBUG || debug ) fprintf( stderr, "There is no filters to be run\n" );
		return 0;
	}

	files->current = files->first;

	if ( -1 == chdir( cwd ) ) {
		fprintf( stderr, "run_filters: failed to CHDIR to '%s'\n", cwd );
		exit( 1 );
	}

	strcpy( path, pckg_tmp_dir );
	strcat( path, upload_folder );

	if ( -1 == chdir( path ) ) {
		fprintf( stderr, "run_filters: failed to CHDIR to '%s'\n", path );
		exit( 1 );
	}

	while ( files->current ) {
		if ( DEBUG || debug ) fprintf( stderr, "Opening file '%s'\n", files->current->name );

		ext = strrchr( files->current->name, '.' );

		// If file is not php, tpl or xml  - skip it
		if (
			NULL == ext
			||
			(
				strncmp( ext, ".php", 4 ) != 0 &&
				strncmp( ext, ".tpl", 4 ) != 0 &&
				strncmp( ext, ".xml", 4 ) != 0 &&
				strncmp( ext, ".js", 3 )  != 0 &&
				strncmp( ext, ".css", 4 ) != 0
			)
		) goto next;

		if ( NULL != strstr( files->current->name, "ocmod.xml" ) && NULL != strstr( files->current->name, "system/")  ) {

			// system/name.ocmod.xml is now ../install.xml
			if ( NULL == ( f = fopen( "../install.xml", "r+" ) ) ) {
				fprintf( stderr, "run_filters: failed to open file '%s'\n", files->current->name );
				return 1;
			}

		} else {
			if ( NULL == ( f = fopen( files->current->name, "r+" ) ) ) {
				fprintf( stderr, "run_filters: failed to open file '%s'\n", files->current->name );
				return 1;
			}
		}

		filters->current = filters->first;

		while( filters->current ) {
			if ( DEBUG || debug ) fprintf( stderr, "Running filter '%s'\n", filters->current->name );

			((callback)filters->current->value)( f, files->current->name );
			filters->current = filters->current->next;
		}

		fclose( f );
		next: files->current = files->current->next;
	}

	if ( -1 == chdir( cwd ) ) {
		fprintf( stderr, "run_filters: failed to CHDIR to '%s'\n", cwd );
		exit( 1 );
	}
}

/**
 * Registers filters
 *
 */
int init_filters() {
	if ( DEBUG ) fprintf( stderr, "Initializing filters\n" );

	filters = init_llist();
	filters->addp( "translation", fill_translation, filters );
	filters->addp( "version", add_version, filters );

	return 0;
}

/**
 * Returns pointer to list of matches gotten from match structure of regex
 *
 */
struct llist *get_matches( const char *str ) {
	int debug = 0;

	size_t str_len = strlen( str ) + 1;
	int reg_len, i;

	if ( DEBUG || debug ) fprintf( stderr, "Start fetching matches form string '%s' with the length: %ld\n", str, str_len );

	char* t_line = malloc( str_len );

	if ( NULL == t_line ) {
		print_error( "get_matches: failed to allocate memory" );
	}

	struct llist *matches;
	matches = init_llist();

	for ( i = 0; i < REGEX_MATCH_COUNT, m[ i ].rm_so >= 0; i++ ) {
		if ( -1 != m[ i ].rm_so ) {

			if ( DEBUG || debug ) fprintf( stderr, "Match #%d\n", i  );
			if ( DEBUG || debug ) fprintf( stderr, "Match start: %d, match end: %d\n", m[ i ].rm_so, m[ i ].rm_eo );

			memset( t_line, '\0', str_len );
			reg_len = m[ i ].rm_eo - m[ i ].rm_so;

			if ( DEBUG || debug ) fprintf( stderr, "Copying string from offset %d %d characters length\n", m[ i ].rm_so, reg_len );

			strncpy( t_line, &str[ m[ i ].rm_so ], reg_len );

			if ( DEBUG || debug ) fprintf( stderr, "Match value: '%s'\n", t_line );

			matches->add( NULL, t_line, matches );
		}
	}

	free( t_line );

	return matches;
}

/**
 * FS iterator callback to empty a folder. Unlinks files
 *
 */
int del_file_cb( char *name, struct stat *sb ) {
	if ( 0 != unlink( name ) ) {
		fprintf( stderr, "Failed to unlink file '%s': %s\n", name, strerror( errno ) );

		exit( 1 );
	}

	return 0;
}

/**
 * FS iterator callback to empty a folder. Deletes files
 *
 */
int del_dir_cb( char *name, struct stat *sb ) {
	if ( 0 != rmdir( name ) ) {
		fprintf( stderr, "Failed to remove directory '%s': %s\n", name, strerror( errno ) );

		exit( 1 );
	}

	return 0;
}

/**
 * Returns common part for package files. Eg extension/module
 *
 */
char *get_common_dir() {
	char *p;
	char *end;

	if ( 0 != strlen( lang_dir ) ) {
		return lang_dir;
	}

	if ( NULL == files->first ) {
		fprintf( stderr, "get_common_dir: Files list is empty\n" );
		return lang_dir;
	}

	files->current = files->first;

	while ( files->current ) {
		if ( NULL != ( p = strstr( files->current->name, "/controller/" ) ) ) {
			if ( NULL != ( end = strrchr( p, '/' ) ) ) {

				// With leading slash
				strncpy( lang_dir, p + 12, end - p - 11 );
				
			} else {
				print_error( "get_common_dir: failed to fetch common part from controller path '%s'\n", files->current->name );
			}
		}

		files->current = files->current->next;
	}

	return lang_dir;
}

/**
 * Makes changes to temporary files structure to make package conforms OC20 restrictions
 *
 */
int make_oc20() {
	int debug = 0;

	if ( DEBUG || debug ) fprintf( stderr, "Making structure for OC20...\n" );

	char t_cwd[ path_max_size ];
	char *p;
	char new_name[ path_max_size ];
	char *dir;

	strcpy( t_cwd, pckg_tmp_dir );
	strcat( t_cwd, upload_folder );


	if ( NULL == files->first ) {
		fprintf( stderr, "make_oc20: FIles list is empty, nothing to process\n" );
		return 1;
	}

	files->current = files->first;

	if ( -1 == chdir( t_cwd ) ) {
		fprintf( stderr, "make_oc20: failed to change CWD to '%s': %s\n", t_cwd, strerror( errno ) );
		exit( 1 );
	}

	while ( NULL != files->current ) {

		if ( NULL != ( p = strstr( files->current->name, "/controller/extension/" ) ) ) {
			memset( new_name, '\0', path_max_size );
			strncpy( new_name, files->current->name, p - files->current->name  + 12 ); // from the start up to leading slash
			strcat( new_name, p + 22 );

			if ( DEBUG || debug ) fprintf( stderr, "Changing '%s' => '%s'\n", files->current->name, new_name );

			dir = dir_name( new_name );
			make_dir( dir, 0775 );
			free( dir );

			if ( -1 == rename( files->current->name, new_name ) ) {
				fprintf( stderr, "make_oc20: failed to rename '%s' to '%s': %s\n", files->current->name, new_name, strerror( errno ) );
				exit( 1 );
			}

			content_to_oc20( new_name );
		}

		if ( NULL != ( p = strstr( files->current->name, "/en-gb/" ) ) ) {
			memset( new_name, '\0', path_max_size );
			strncpy( new_name, files->current->name, p - files->current->name ); // from the start up to leading slash
			strcat( new_name, "/english" );

			if ( NULL != strstr( files->current->name, "/en-gb/extension/" ) ) {
				strcat( new_name, p + 16 );

			} else {
				strcat( new_name, p + 6 );
			}

			if ( DEBUG || debug ) fprintf( stderr, "Changing '%s' => '%s'\n", files->current->name, new_name );

			dir = dir_name( new_name );
			make_dir( dir, 0775 );
			free( dir );
			
			if ( -1 == rename( files->current->name, new_name ) ) {
				fprintf( stderr, "make_oc20: failed to rename '%s' to '%s': %s\n", files->current->name, new_name, strerror( errno ) );
				exit( 1 );
			}
		}

		if ( NULL != ( p = strstr( files->current->name, "/model/extension/" ) ) ) {
			memset( new_name, '\0', path_max_size );
			strncpy( new_name, files->current->name, p - files->current->name  + 7 ); // from the start up to leading slash
			strcat( new_name, p + 17 );

			if ( DEBUG || debug ) fprintf( stderr, "Changing '%s' => '%s'\n", files->current->name, new_name );

			dir = dir_name( new_name );
			make_dir( dir, 0775 );
			free( dir );
			
			if ( -1 == rename( files->current->name, new_name ) ) {
				fprintf( stderr, "make_oc20: failed to rename '%s' to '%s': %s\n", files->current->name, new_name, strerror( errno ) );
				exit( 1 );
			}

			content_to_oc20( new_name );
		}

		if ( NULL != ( p = strstr( files->current->name, "/template/extension/" ) ) ) {
			memset( new_name, '\0', path_max_size );
			strncpy( new_name, files->current->name, p - files->current->name  + 10 ); // from the start up to leading slash
			strcat( new_name, p + 20 );

			if ( DEBUG || debug ) fprintf( stderr, "Changing '%s' => '%s'\n", files->current->name, new_name );

			dir = dir_name( new_name );
			make_dir( dir, 0775 );
			free( dir );
			
			if ( -1 == rename( files->current->name, new_name ) ) {
				fprintf( stderr, "make_oc20: failed to rename '%s' to '%s': %s\n", files->current->name, new_name, strerror( errno ) );
				exit( 1 );
			}
		}


		files->current = files->current->next;
	}

	// Remove all empty directories
	iterate( ".", NULL, del_empty_dirs_cb, on_iterate_error );

	if ( -1 == chdir( cwd ) ) {
		fprintf( stderr, "make_oc20: failed to change CWD to '%s': %s\n", cwd, strerror( errno ) );
		exit( 1 );
	}

	return 0;
}

/**
 * Directory callback for delete empty folders iterator
 *
 */
int del_empty_dirs_cb( char *path, struct stat *sb ) {
		if ( rmdir( path ) && ENOTEMPTY != errno ) {
		fprintf( stderr, "del_empty_dirs_cb: path: '%s', error: %s\n", path, strerror( errno ) );
		return 1;
	}

	return 0;
}

/**
 * Changes classes names for controller and model to be OC20 compliant
 *
 */
int content_to_oc20( char *name ) {
	int debug = 0;

	FILE *f;
	char buff[ MAX_LINE ];
	struct llist *matches;
	size_t len;

	char tb[ MAX_LINE ];

	if ( NULL == ( f = fopen( name, "r+" ) ) ) {
		fprintf( stderr, "content_to_oc20: failed to open file '%s': %s", name, strerror( errno ) );
		exit( 1 );
	}

	if ( DEBUG || debug ) fprintf( stderr,  "File '%s' opened for modifications\n", name );

	while ( NULL != fgets( buff, MAX_LINE, f ) ) {
		if ( DEBUG || debug ) fprintf( stderr,  "'%s'\n", buff );
		if ( DEBUG || debug ) fprintf( stderr,  "Current offest: %ld\n", ftello( f ) );

		if ( 0 == match( buff, "class\\s+\\w+?(extension)", m, REG_ICASE ) ) {
			if ( DEBUG || debug ) {
				fprintf( stderr, "Matched string \n'%s'\n", buff );
				matches = get_matches( buff );
				fprintf( stderr, "Match: '%s'\n", (char*)matches->fetch( "1", matches ) );
			}

			len = strlen( buff );

			memset( tb, '\0', MAX_LINE );
			strncpy( tb, buff, m[ 1 ].rm_so );
			strcat( tb, &buff[ m[ 1 ].rm_eo ] );

			// Add spaces in place of removed characters up to original newline character
			strcat( tb, "         \n" );

			if ( DEBUG || debug ) fprintf( stderr, "Result to be saved: \n'%s'\n", tb );
			if ( DEBUG || debug ) fprintf( stderr, "String length: %ld\n", len );

			if( -1 == fseeko( f, -1 * len, SEEK_CUR ) ) {
				fprintf( stderr, "content_to_oc20: failed to set new position on stream: %s\n", strerror( errno ) );
				exit( 1 );
			}

			if ( DEBUG || debug ) fprintf( stderr,  "Stream position after: %ld\n", ftello( f ) );

			if ( EOF == fputs( tb, f ) ) {
				fprintf( stderr, "content_to_oc20: failed to write back string to a file '%s': %s\n", name, strerror( errno ) );
				exit( 1 );
			}

			if ( DEBUG || debug ) fprintf( stderr,  "File '%s' was modified\n", name );

			break;
		}
	}

	fclose( f );

	return 0;
}

/**
 * Adds versions numbers to package files
 *
 *
 */
int add_version( FILE *f ) {
	int debug = 0;

	char buff[ MAX_LINE ];
	char prev[MAX_LINE ];
	char new_buff[ MAX_LINE ];
	struct llist *matches;
	size_t len, str_len, buff_len, new_len;
	int c = 0;
	char *p;
	int i;

	rewind( f );

	while ( NULL != fgets( buff, MAX_LINE, f ) ) {
		if ( c > 10 ) break;

		if ( 0 == match( buff, "@version\\s+([0-9]+\\.[0-9]+\\.[0-9]+ *)$", m, REG_NEWLINE ) ) {
			if ( DEBUG || debug ) {
				printf( "Prev: '%s'\n", prev );
				fprintf( stderr,  "Current offset: %ld\n", ftello( f ) );
				fprintf( stderr, "Matched string\n" );

				p = buff;
				while( '\0' != *p ) {
					printf( "%4c", *p );
					p++;
				}

				printf( "\n" );

				p = buff;
				while( '\0' != *p ) {
					printf( "%4d", *p );
					p++;
				}

				printf( "\n" );

				p = buff;
				i = 0;
				while( '\0' != *p ) {
					printf( "%4d", i++ );
					p++;
				}

				printf( "\n" );

				matches = get_matches( buff );
				fprintf( stderr, "Match: '%s'\n", (char*)matches->fetch( "1", matches ) );
			}

			// Length of package current version in characters
			len = N_LEN( major ) + N_LEN( minor ) + N_LEN( patch ) + 2;

			if ( DEBUG || debug ) fprintf( stderr, "Major: %f, minor - %f, patch: %f\n", N_LEN( major ), N_LEN( minor ), N_LEN( patch ) );

			// Matched version length
			str_len = m[ 1 ].rm_eo - m[ 1 ].rm_so;

			// Buffer length
			buff_len = strlen( buff );

			if ( DEBUG || debug ) fprintf( stderr,  "Package version length (%d.%d.%d) is : %ld\nMatched version length is: %ld\nBuffer length: %ld\nMatch start: %d\nMatch end: %d\n", major, minor, patch, len, str_len, buff_len, m[ 1 ].rm_so, m[ 1 ].rm_eo );

			memset( new_buff, '\0', MAX_LINE );
			strncpy( new_buff, buff, m[ 1 ].rm_so );

			// Add version numbers right after '@version '
			if ( str_len < len && str_len >= 5 ) {
				strcat( new_buff, "0.0.0" );
				len = 5;

			} else if ( str_len >= len ) {
				sprintf( &new_buff[ m[ 1 ].rm_so ], "%d.%d.%d", major, minor, patch );

			} else {
				break;
			}

			if ( debug ) {
				p = new_buff;
				while( '\0' != *p ) {
					printf( "%4c", *p );
					p++;
				}

				printf( "\n" );

				p = new_buff;
				while( '\0' != *p ) {
					printf( "%4d", *p );
					p++;
				}

				printf( "\n" );

				p = new_buff;
				i = 0;
				while( '\0' != *p ) {
					printf( "%4d", i++ );
					p++;
				}

				printf( "\n" );
			}
			
			new_len = strlen( new_buff );

			if ( DEBUG || debug ) fprintf( stderr, "New buffer length: %ld\n", new_len );

			if ( str_len > len ) {
				if ( debug ) fprintf( stderr, "Padding right with %ld spaces\n", str_len - len );
				memset( &new_buff[ new_len ], ' ', str_len - len );
			}

			strcat( new_buff, "\n" );

			if ( debug ) {
				p = new_buff;
				while( '\0' != *p ) {
					printf( "%4c", *p );
					p++;
				}

				printf( "\n" );

				p = new_buff;
				while( '\0' != *p ) {
					printf( "%4d", *p );
					p++;
				}

				printf( "\n" );

				p = new_buff;
				i = 0;
				while( '\0' != *p ) {
					printf( "%4d", i++ );
					p++;
				}

				printf( "\n" );

				fprintf( stderr, "Result to be saved: '%s'\n", new_buff );
				fprintf( stderr, "Buffer new length: %ld\n", strlen( new_buff ) );
			}

			if( -1 == fseeko( f, -1 * buff_len, SEEK_CUR ) ) {
				fprintf( stderr, "content_to_oc20: failed to set new position on stream: %s\n", strerror( errno ) );
				exit( 1 );
			}

			if ( DEBUG || debug ) fprintf( stderr,  "Stream position after: %ld\n", ftello( f ) );

			if ( EOF == fputs( new_buff, f ) ) {
				fprintf( stderr, "content_to_oc20: failed to write back string to a file: %s\n", strerror( errno ) );
				exit( 1 );
			}

			if ( DEBUG || debug ) fprintf( stderr,  "File was modified\n" );

			break;
		}

		c++;
		strcpy( prev, buff );
		memset( buff, '\0', MAX_LINE );
	}
}

xmlNodePtr find_node( xmlNodePtr nod, char *name ) {
	xmlNodePtr cur, ret;

	cur = nod->xmlChildrenNode;

	// Try first level
	while ( NULL != cur ) {
		if ( 0 == xmlStrcmp( cur->name, ( const xmlChar * )name ) ) {
			return cur;
		}

		cur = cur->next;
	}

	cur = nod->xmlChildrenNode;

	// Try recursion
	while ( NULL != cur ) {
		if ( NULL != ( ret = find_node( cur, name ) ) ) {
			return ret;
		}

		cur = cur->next;
	}

	return NULL;
}

int fix_vqmod_file( xmlDocPtr doc, xmlNodePtr cur ) {
	xmlNodePtr add, search;

	// file path => name
	xmlSetProp( cur, "name", xmlGetProp( cur, "path" ) );
	xmlUnsetProp( cur, "path" );

	add = find_node( cur, "add" );
	search = find_node( cur, "search" );

	if ( NULL == add ) {
		fprintf( stderr, "fix_vqmod_file: file node doesn't contain node 'add'\n" );
		exit( 1 );
	}

	if ( NULL == search ) {
		fprintf( stderr, "fix_vqmod_file: file node doesn't contain nod 'search'\n" );
		exit( 1 );
	}

	xmlSetProp( search, "position", xmlGetProp( add, "position" ) );
	xmlUnsetProp( add, "position" );

    return 1;
}

xmlDocPtr parseDoc() {

	xmlDocPtr doc;
	xmlNodePtr cur, prev, mod;
	char name[path_max_size ];

	strcpy( name, pckg_tmp_dir );
	sprintf( &name[ strlen( name ) ], "%s.ocmod.xml", code );

	doc = xmlParseFile( name );
	
	if ( doc == NULL ) {
		fprintf( stderr,"parseDoc: Document not parsed successfully.\n");
		return (NULL);
	}
	
	mod = xmlDocGetRootElement( doc );
	
	if (mod == NULL) {
		fprintf(stderr,"parseDoc: empty document\n");
		xmlFreeDoc(doc);
		return (NULL);
	}
	
	if (xmlStrcmp(mod->name, (const xmlChar *) "modification")) {
		fprintf(stderr,"parseDoc: document of the wrong type, root node != modification");
		xmlFreeDoc(doc);
		return (NULL);
	}
	
	cur = mod->xmlChildrenNode;
	prev = NULL;

	while ( cur != NULL ) {
		if ( ( ! xmlStrcmp( cur->name, ( const xmlChar * )"file" ) ) ) {
			fix_vqmod_file( doc, cur );

		} else if ( ( ! xmlStrcmp( cur->name, ( const xmlChar * )"name" ) ) ) {
			xmlNodeSetName( cur, "id" );

		} if ( 0 == xmlStrcmp( cur->name, ( const xmlChar * )"code" ) || 0 == xmlStrcmp( cur->name, ( const xmlChar * )"link" ) ) {
			prev = cur;
			cur = cur->next;

			xmlUnlinkNode( prev );
			xmlFreeNode( prev );
			continue;
		}
		 
		cur = cur->next;
	}

	return(doc);
}

int make_vqmod() {
	char name[ path_max_size ];
	char *keyword;
	xmlDocPtr doc;

	strcpy( name, pckg_tmp_dir );
	sprintf( &name[ strlen( name ) ], "%s.vqmod.xml", code );

	doc = parseDoc ();

	if (doc != NULL) {
		xmlSaveFormatFile ( name, doc, 0 );
		xmlFreeDoc( doc );
	}
	
	return 0;
}

int set_config_name() {
	strcat( config_name, "_" );
	strcat( config_name, code );

	return 0;
}

/**
 * Parses configuration file and fills in configuration container
 */
int parse_config( gchar *config_name ) {
	if ( DEBUG )g_print( "parse config: start\n");

	if ( IS_EMPTY( config_name ) ) {
		if ( DEBUG )printf("File name is empty. Skip\n" );
		return 0;
	}

	gchar name[ path_max_size ];
	gchar *keyword;

	g_return_val_if_fail( NULL != config, 1 );

	xmlDocPtr doc;
	xmlNodePtr cur, prev, root;

	if ( DEBUG )printf( "Opening configuration file %s\n", config_name );

	doc = xmlParseFile( config_name );

	if ( doc == NULL ) {
		fprintf( stderr,"Configuration file was not parsed.\n");
		return 1;
	}
	
	root = xmlDocGetRootElement( doc );
	
	if ( root == NULL) {
		fprintf(stderr,"Configuration file is empty\n");
		xmlFreeDoc(doc);
		return 0;
	}
	
	if (xmlStrcmp(root->name, (const xmlChar *) "config")) {
		fprintf(stderr,"parse_conf: document of the wrong type, root node != config");
		xmlFreeDoc(doc);
		exit( 1 );
	}
	
	cur = root->xmlChildrenNode;
	prev = NULL;

	while ( cur != NULL ) {
		if ( ! xmlStrcmp( cur->name, ( const xmlChar * )"include_file" ) ) {
			if ( DEBUG )printf( "Fill in 'include_file' list\n" );
			xml_to_config( "include_file", cur );

		} else if ( ! xmlStrcmp( cur->name, ( const xmlChar * )"exclude_file" ) ) {
			if ( DEBUG )printf( "Fill in 'exclude_file' list\n" );
			xml_to_config( "exclude_file", cur );

		} else if ( ! xmlStrcmp( cur->name, ( const xmlChar * )"include_folder" ) ) {
			if ( DEBUG )printf( "Fill in 'include_folder' list\n" );
			xml_to_config( "include_folder", cur );

		} else if ( ! xmlStrcmp( cur->name, ( const xmlChar * )"exclude_folder" ) ) {
			if ( DEBUG )printf( "Fill in 'exclude_folder' list\n" );
			xml_to_config( "exclude_folder", cur );

		} else if ( ! xmlStrcmp( cur->name, ( const xmlChar * )"include_regex" ) ) {
			if ( DEBUG )printf( "Fill in 'include_regex' list\n" );
			xml_to_config( "include_regex", cur );

		} else if ( ! xmlStrcmp( cur->name, ( const xmlChar * )"exclude_regex" ) ) {
			if ( DEBUG )printf( "Fill in 'exclude_regex' list\n" );
			xml_to_config( "exclude_regex", cur );

		} else if ( ! xmlStrcmp( cur->name, ( const xmlChar * )"code" ) ) {
			if ( DEBUG )printf( "Fill in 'code' list\n" );
			xml_to_config( "code", cur );

		} else if ( ! xmlStrcmp( cur->name, ( const xmlChar * )"major" ) ) {
			if ( DEBUG )printf( "Fill in 'major' list\n" );
			xml_to_config( "major", cur );

		} else if ( ! xmlStrcmp( cur->name, ( const xmlChar * )"minor" ) ) {
			if ( DEBUG )printf( "Fill in 'minor' list\n" );
			xml_to_config( "minor", cur );

		} else if ( ! xmlStrcmp( cur->name, ( const xmlChar * )"patch" ) ) {
			if ( DEBUG )printf( "Fill in 'patch' list\n" );
			xml_to_config( "patch", cur );
		}
		 
		cur = cur->next;
	}

	if (doc != NULL) {
		xmlFreeDoc( doc );
	}
	
	return 0;
}

/**
 * Puts values from XML to configuration structure
 */
int xml_to_config( char *name, xmlNodePtr root ) {
	if ( DEBUG )print_color( B_GREEN, "xml_to_config: Start" );

	guint debug = 0;
	GSList *config_item = NULL;
	GSList *copy_pointer = NULL;
	gchar *value;
	
	g_return_val_if_fail( g_hash_table_contains( config, name ), 1 );
	xmlNodePtr cur = root->xmlChildrenNode;

	while ( cur != NULL ) {
		if ( strlen( g_strstrip( g_strdup( xmlNodeGetContent( cur ) ) ) ) ) {
			value = g_strdup( xmlNodeGetContent( cur ) );

			if ( DEBUG || debug ) fprintf( stderr, "Add value '%s' to list\n", value );

			copy_pointer = g_slist_append( copy_pointer, value );
		}

		cur = cur->next;
	};

	if ( DEBUG )printf( "Re-saving configuration '%s'\n", name );

	// List's pointer has been changed - we need to re-save the data
	g_hash_table_insert( config, g_strdup( name ), copy_pointer );

	return 0;
}

/**
 * Get configuration data from config file and put it into inner storage plus fill in corresponding inputs 
 * Change combobox event handler
 */
void fill_in_config( GtkComboBox *widget, gpointer user_data ) {
	if ( DEBUG )print_color( B_CYAN, ">>>>>> PACKAGE SELECTED (fill_in_config) <<<<<<\n" );

	GSList
		*include_folder = NULL,
		*exclude_folder = NULL,
		*include_file   = NULL,
		*exclude_file   = NULL,
		*include_regex  = NULL,
		*exclude_regex  = NULL,
		*code = NULL,
		*major = NULL,
		*minor = NULL,
		*patch = NULL;

	if ( DEBUG )printf( "Fetching config data\n" );

	// Free previous data
	if ( NULL != config ) {
		if ( DEBUG )printf( "Clearing previous data\n" );
		g_hash_table_destroy( config );

	} else {
		printf( "Nothing to free\n" );
	}

	char *name = g_malloc0( path_max_size );
	config = g_hash_table_new_full( g_str_hash, g_str_equal, (GDestroyNotify)config_key_clean, (GDestroyNotify)config_value_clean );

	g_hash_table_insert( config, g_strdup( "include_folder" ), include_folder );
	g_hash_table_insert( config, g_strdup( "exclude_folder" ), exclude_folder );
	g_hash_table_insert( config, g_strdup( "include_file" ),   include_file );
	g_hash_table_insert( config, g_strdup( "exclude_file" ),   exclude_file );
	g_hash_table_insert( config, g_strdup( "include_regex" ),  include_regex );
	g_hash_table_insert( config, g_strdup( "exclude_regex" ),  exclude_regex );
	g_hash_table_insert( config, g_strdup( "code" ), code );
	g_hash_table_insert( config, g_strdup( "major" ), major );
	g_hash_table_insert( config, g_strdup( "minor" ), minor );
	g_hash_table_insert( config, g_strdup( "patch" ), patch );

	// Get file name from combobox
	name = gtk_combo_box_text_get_active_text( GTK_COMBO_BOX_TEXT( widget ) );

	if ( DEBUG )printf( "Config name from combobox: %s\n", name  );

	// Fill in config structure from config file
	parse_config( name );
	g_free( name );

	// Update configuration tab
	update_config_view();

	if ( DEBUG )printf( "Fill in config end\n" );
}

/**
 * Cleans up hash table key
 */
void config_key_clean( gpointer key ) {
	if ( DEBUG )print_color( B_GREEN, "config_key_clean: Request to destroy hash key '%s'\n", (char*)key );
	g_free( key );
}

/**
 * Cleans up hash table value
 */
void config_value_clean( gpointer value ) {
	if ( DEBUG )print_color( B_GREEN, "config_value_clean: Request to destroy hash value [%p]\n", value );
	g_slist_free_full ( value, (GDestroyNotify)g_free );

}

/**
 * Fills in view with configuration date
 */
void update_config_view() {
	if ( DEBUG )print_color( B_GREEN, "update_config_view: Updating configuration view...\n" );

	GSList
		*include_folder = g_hash_table_lookup( config, "include_folder" ),
		*exclude_folder = g_hash_table_lookup( config, "exclude_folder" ),
		*include_file   = g_hash_table_lookup( config, "include_file" ),
		*exclude_file   = g_hash_table_lookup( config, "exclude_file" ),
		*include_regex  = g_hash_table_lookup( config, "include_regex" ),
		*exclude_regex  = g_hash_table_lookup( config, "exclude_regex" ),
		*code           = g_hash_table_lookup( config, "code" ),
		*major          = g_hash_table_lookup( config, "major" ),
		*minor          = g_hash_table_lookup( config, "minor" ),
		*patch          = g_hash_table_lookup( config, "patch" );

	gchar *code_name;

	// Check view controls
	g_return_if_fail( NULL != config );
	g_return_if_fail( NULL != input_code );
	g_return_if_fail( NULL != input_major );
	g_return_if_fail( NULL != input_minor );
	g_return_if_fail( NULL != input_patch );
	g_return_if_fail( NULL != buffer_include_file );
	g_return_if_fail( NULL != buffer_exclude_file );
	g_return_if_fail( NULL != buffer_include_folder );
	g_return_if_fail( NULL != buffer_exclude_folder );
	g_return_if_fail( NULL != buffer_include_regex );
	g_return_if_fail( NULL != buffer_exclude_regex );

	// Check configurations
	g_return_if_fail( g_hash_table_contains( config, "include_file" ) );
	g_return_if_fail( g_hash_table_contains( config, "exclude_file" ) );
	g_return_if_fail( g_hash_table_contains( config, "include_folder" ) );
	g_return_if_fail( g_hash_table_contains( config, "exclude_folder" ) );
	g_return_if_fail( g_hash_table_contains( config, "include_regex" ) );
	g_return_if_fail( g_hash_table_contains( config, "exclude_regex" ) );
	g_return_if_fail( g_hash_table_contains( config, "code" ) );
	g_return_if_fail( g_hash_table_contains( config, "major" ) );
	g_return_if_fail( g_hash_table_contains( config, "minor" ) );
	g_return_if_fail( g_hash_table_contains( config, "patch" ) );

	clear_config_buffers();

	if ( DEBUG )printf("Buffers cleared\n");

	// Package code
	if ( NULL != code ) {
		code_name = code->data ?: "";
		if ( DEBUG )printf( "Set package code: %s\n", code_name );
		gtk_entry_set_text( input_code, code_name );
	}


	// Package major number
	if ( NULL != major ) {
		if ( DEBUG )printf( "Set package major version number: %f\n", (double)atoi( (char*)major->data ) );
		gtk_spin_button_set_value( input_major, (double)atoi( (char*)major->data ) );
	}

	// Package minor number
	if ( NULL != minor ) {
		if ( DEBUG )printf( "Set package minor version number: %f\n", (double)atoi( (char*)minor->data ) );
		gtk_spin_button_set_value( input_minor, (double)atoi( (char*)minor->data ) );
	}

	// Package patch number
	if ( NULL != patch ) {
		if ( DEBUG )printf( "Set package patch version number: %f\n", (double)atoi( (char*)patch->data ) );
		gtk_spin_button_set_value( input_patch, (double)atoi( (char*)patch->data ) );
	}

	// Included files
	if ( DEBUG ) {
		printf( "Set included files\n" );
		dump_slist( include_file );
	}

	g_slist_foreach( include_file, &fill_in_input_buffer, buffer_include_file );

	// Excluded files
	if ( DEBUG ) {
		printf( "Set excluded files\n" );
		dump_slist( exclude_file );
	}

	g_slist_foreach( exclude_file, &fill_in_input_buffer, buffer_exclude_file );

	// Included folders
	if ( DEBUG ) {
		printf( "Set included folders\n" );
		dump_slist( include_folder );
	}

	g_slist_foreach( include_folder, &fill_in_input_buffer, buffer_include_folder );

	// Excluded folders
	if ( DEBUG ) {
		printf( "Set excluded folders\n" );
		dump_slist( exclude_folder );
	}

	g_slist_foreach( exclude_folder, &fill_in_input_buffer, buffer_exclude_folder );

	// Included regex
	if ( DEBUG ) {
		printf( "Set included regex\n" );
		dump_slist( include_regex );
	}

	g_slist_foreach( include_regex, &fill_in_input_buffer, buffer_include_regex );

	if ( DEBUG ) {
		printf( "Set excluded regex\n" );
		dump_slist( exclude_regex );
	}

	g_slist_foreach( exclude_regex, &fill_in_input_buffer, buffer_exclude_regex );

	if ( DEBUG )printf( "View has been updated\n" ); 
}

/**
 * Fill ins text buffer with specific text
 */
void fill_in_input_buffer( void *text, void *buffer ) {
	if ( DEBUG )print_color( B_GREEN, "fill_in_input_buffer: Stat\n" );

	g_return_if_fail( buffer != NULL );
	GtkTextIter *start;
	GtkTextIter *end;
	char *new_text;

	// Empty list
	if ( text == NULL ) return;

	if ( DEBUG )printf( "Adding text '%s' to text buffer\n", (char*)text );

	start = g_malloc0( sizeof( GtkTextIter ) );
	end = g_malloc0( sizeof( GtkTextIter ) );

	gtk_text_buffer_get_bounds( buffer, start, end );

	// Not first line - prepend newline character
	if ( !gtk_text_iter_equal( start, end ) ) {
		new_text = g_malloc0( strlen( text ) + 2 );
		memcpy( new_text, "\n", 1 );
		strncat( new_text, text, strlen( text ) );
		gtk_text_buffer_insert( buffer, end, new_text, -1 );
		g_free( new_text );

	} else {
		gtk_text_buffer_insert( buffer, end, text, -1 );
	}

	g_free( start );
	g_free( end );
}

/**
 * Clears configuration view buffers' content
 */
void clear_config_buffers() {
	if ( DEBUG ) print_color( B_GREEN, "clear_config_buffers: Clearing config view buffers...\n" );

	// Check view controls
	g_return_if_fail( NULL != config );
	g_return_if_fail( NULL != input_code );
	g_return_if_fail( NULL != input_major );
	g_return_if_fail( NULL != input_minor );
	g_return_if_fail( NULL != input_patch );
	g_return_if_fail( NULL != buffer_include_file );
	g_return_if_fail( NULL != buffer_exclude_file );
	g_return_if_fail( NULL != buffer_include_folder );
	g_return_if_fail( NULL != buffer_exclude_folder );
	g_return_if_fail( NULL != buffer_include_regex );
	g_return_if_fail( NULL != buffer_exclude_regex );

	GtkTextBuffer *list[] = {
		buffer_include_file,
		buffer_exclude_file,
		buffer_include_folder,
		buffer_exclude_folder,
		buffer_include_regex,
		buffer_exclude_regex,
		NULL
	};

	GtkTextIter *start;
	GtkTextIter *end;
	int i = 0;

	start = g_malloc0( sizeof( GtkTextIter ) );
	end = g_malloc0( sizeof( GtkTextIter ) );

	while ( list[ i ] != NULL ) {
		gtk_text_buffer_get_bounds( list[ i ], start, end );
		gtk_text_buffer_delete ( list[ i ], start, end );
		i++;
	}

	// Clear code input
	gtk_entry_set_text( input_code, "" );

	// Clean version numbers
	gtk_spin_button_set_value( input_major, 0 );
	gtk_spin_button_set_value( input_minor, 0 );
	gtk_spin_button_set_value( input_patch, 0 );

	g_free( start );
	g_free( end );
}

/**
 * Fills in list of filters name for further use
 */
// void init_filter_names() {
// 	filter_names = g_slist_append( NULL, g_strdup( "include_file" ) );
// 	filter_names = g_slist_append( filter_names, g_strdup( "exclude_file" ) );
// 	filter_names = g_slist_append( filter_names, g_strdup( "include_folder" ) );
// 	filter_names = g_slist_append( filter_names, g_strdup( "exclude_folder" ) );
// 	filter_names = g_slist_append( filter_names, g_strdup( "include_regex" ) );
// 	filter_names = g_slist_append( filter_names, g_strdup( "exclude_regex" ) );
// }

/** 
 * Reloads lost of configuration files from the disk
 */
void reload_config( GtkButton *button, gpointer data ) {
	if ( DEBUG )print_color( B_CYAN, ">>>>>>> RELOAD CONFIGS (reload_config)<<<<<<<\n" );
	g_signal_handler_block( select_package, select_package_handler );
	get_package_configs();
	g_signal_handler_unblock( select_package, select_package_handler );
}

/** 
 * Saves configuration file on disk
 */
void save_config( GtkButton *button, gpointer data ) {
	if ( DEBUG )print_color( B_CYAN, ">>>>>>> SAVE CONFIG (save_config)<<<<<<<\n" );
	xmlDocPtr doc;
	xmlNodePtr root, child;
	char *package_name;
	int written;
	char *file_name;

	update_config_from_view();

	GSList
		*include_folder = g_hash_table_lookup( config, "include_folder" ),
		*exclude_folder = g_hash_table_lookup( config, "exclude_folder" ),
		*include_file   = g_hash_table_lookup( config, "include_file" ),
		*exclude_file   = g_hash_table_lookup( config, "exclude_file" ),
		*include_regex  = g_hash_table_lookup( config, "include_regex" ),
		*exclude_regex  = g_hash_table_lookup( config, "exclude_regex" ),
		*code           = g_hash_table_lookup( config, "code" ),
		*major          = g_hash_table_lookup( config, "major" ),
		*minor          = g_hash_table_lookup( config, "minor" ),
		*patch          = g_hash_table_lookup( config, "patch" );

	// Check configurations
	g_return_if_fail( g_hash_table_contains( config, "include_file" ) );
	g_return_if_fail( g_hash_table_contains( config, "exclude_file" ) );
	g_return_if_fail( g_hash_table_contains( config, "include_folder" ) );
	g_return_if_fail( g_hash_table_contains( config, "exclude_folder" ) );
	g_return_if_fail( g_hash_table_contains( config, "include_regex" ) );
	g_return_if_fail( g_hash_table_contains( config, "exclude_regex" ) );
	g_return_if_fail( g_hash_table_contains( config, "code" ) );
	g_return_if_fail( g_hash_table_contains( config, "major" ) );
	g_return_if_fail( g_hash_table_contains( config, "minor" ) );
	g_return_if_fail( g_hash_table_contains( config, "patch" ) );

	doc = xmlNewDoc( "1.0" );
	root = xmlNewNode( NULL, "config" );
	xmlDocSetRootElement( doc, root );

	xmlAddChild( root, config_to_xml( "include_file", include_file ) );
	xmlAddChild( root, config_to_xml( "exclude_file", exclude_file ) );
	xmlAddChild( root, config_to_xml( "include_folder", include_folder ) );
	xmlAddChild( root, config_to_xml( "exclude_folder", exclude_folder ) );
	xmlAddChild( root, config_to_xml( "include_regex", include_regex ) );
	xmlAddChild( root, config_to_xml( "exclude_regex", exclude_regex ) );

	child = xmlNewNode( NULL, "code" );
	xmlNodeAddContent( child, code->data );
	xmlAddChild( root, child );

	child = xmlNewNode( NULL, "major" );
	xmlNodeAddContent( child, major->data );
	xmlAddChild( root, child );

	child = xmlNewNode( NULL, "minor" );
	xmlNodeAddContent( child, minor->data );
	xmlAddChild( root, child );

	child = xmlNewNode( NULL, "patch" );
	xmlNodeAddContent( child, patch->data );
	xmlAddChild( root, child );

	file_name = get_package_name();

	if ( NULL == file_name ) {
		fprintf(stderr, "Failed to construct package name\n" );
		return;
	}

	package_name = g_build_filename( cwd, file_name, NULL );

	if ( DEBUG )printf( "Saving configuration into file %s\n", package_name );

	if ( ( written = xmlSaveFormatFile( package_name, doc, 1 ) ) != -1 ) {
		fprintf( stderr, "Configuration file '%s' was updated (%i bytes were written)\n", package_name, written );

	} else {
		fprintf( stderr, "Failed to update configuration file\n" );
	}

	if ( DEBUG )xmlDocDump( stdout, doc );

	xmlFreeDoc( doc );
	g_free( package_name );
	g_free( file_name );
}

/*
 * Returns package name
 */
char *get_package_name() {
	g_return_val_if_fail( NULL != input_code, NULL );
	g_return_val_if_fail( NULL != select_package, NULL );

	char *name;

	name = gtk_combo_box_text_get_active_text( select_package );

	if ( IS_EMPTY( name ) ) {
		name = g_strdup( gtk_entry_get_text( input_code ) );

		if ( IS_EMPTY( name ) ) {
			if( DEBUG )g_print( "Package name is empty\n" );
			return NULL;
		}

		name = g_realloc( name, strlen( name ) + 9 ); // + .package suffix
		g_strlcat( name, ".package", strlen( name ) + 9 );
	}

	return name;
}

/**
 * Shows delete package configuration dialog
 */
void delete_config_dialog_show( GtkButton *button, gpointer data ) {
	if ( DEBUG )print_color( B_CYAN, ">>>>>>>> SHOW DIALOG <<<<<<<<\n");
	gtk_widget_show( GTK_WIDGET( delete_package_confirm ) );
}

/**
 * Deletes configuration file from the disk
 */
void delete_config( GtkButton *button, gpointer data ) {
	if ( DEBUG )print_color( B_CYAN, ">>>>>>>> DELETE CONFIG <<<<<<<<\n");

	gtk_widget_hide( GTK_WIDGET( delete_package_confirm ) );

	g_return_if_fail( NULL != select_package );

	char *name = gtk_combo_box_text_get_active_text( select_package );
	if ( Unlink( name ) == 0 ) {
		fprintf( stderr, "Configuration file '%s' has been deleted\n", name );
		reload_config( NULL, NULL );
	}

	g_free( name );
}

/**
 * Hides delete package configuration dialog
 */
void delete_config_dialog_hode( GtkButton *button, gpointer data ) {
	if ( DEBUG )print_color( B_CYAN, ">>>>>>>> HIDE DIALOG <<<<<<<<\n");
	gtk_widget_hide( GTK_WIDGET( delete_package_confirm ) );
}

void update_config_from_view() {
	if ( DEBUG )print_color( B_GREEN, "update_config_from_view: Updating configuration from the view\n" );

	GSList
		*new_code           = NULL,
		*new_major          = NULL,
		*new_minor          = NULL,
		*new_patch          = NULL,
		*new_include_file   = NULL,
		*new_exclude_file   = NULL,
		*new_include_folder = NULL,
		*new_exclude_folder = NULL,
		*new_include_regex  = NULL,
		*new_exclude_regex  = NULL;

	// Check view controls
	g_return_if_fail( NULL != config );
	g_return_if_fail( NULL != input_code );
	g_return_if_fail( NULL != input_major );
	g_return_if_fail( NULL != input_minor );
	g_return_if_fail( NULL != input_patch );
	g_return_if_fail( NULL != buffer_include_file );
	g_return_if_fail( NULL != buffer_exclude_file );
	g_return_if_fail( NULL != buffer_include_folder );
	g_return_if_fail( NULL != buffer_exclude_folder );
	g_return_if_fail( NULL != buffer_include_regex );
	g_return_if_fail( NULL != buffer_exclude_regex );

	// Update package code
	new_code = g_slist_append( new_code, g_strdup( gtk_entry_get_text( input_code ) ) );
	g_hash_table_insert( config, g_strdup( "code" ), new_code );

	// Update package major number
	new_major = g_slist_append( new_major, g_strdup_printf( "%lf", gtk_spin_button_get_value( input_major ) ) );
	g_hash_table_insert( config, g_strdup( "major" ), new_major );

	// Update package minor number
	new_minor = g_slist_append( new_minor, g_strdup_printf( "%lf", gtk_spin_button_get_value( input_minor ) ) );
	g_hash_table_insert( config, g_strdup( "minor" ), new_minor );

	// Update package patch number
	new_patch = g_slist_append( new_patch, g_strdup_printf( "%lf", gtk_spin_button_get_value( input_patch ) ) );
	g_hash_table_insert( config, g_strdup( "patch" ), new_patch );

	// Update package included files
	g_hash_table_insert( config, g_strdup( "include_file" ), text_buffer_to_slist( buffer_include_file, new_include_file ) );

	// Update package excluded files
	g_hash_table_insert( config, g_strdup( "exclude_file" ), text_buffer_to_slist( buffer_exclude_file, new_exclude_file ) );

	// Update package included files
	g_hash_table_insert( config, g_strdup( "include_file" ), text_buffer_to_slist( buffer_include_file, new_include_file ) );

	// Update package included files
	g_hash_table_insert( config, g_strdup( "include_file" ), text_buffer_to_slist( buffer_include_file, new_include_file ) );

	// Update package included files
	g_hash_table_insert( config, g_strdup( "include_file" ), text_buffer_to_slist( buffer_include_file, new_include_file ) );

	// Update package included files
	g_hash_table_insert( config, g_strdup( "include_file" ), text_buffer_to_slist( buffer_include_file, new_include_file ) );
}

/**
 * Puts contents of text buffer into SList
 */
GSList *text_buffer_to_slist( GtkTextBuffer* buffer, GSList *list ) {
	if ( DEBUG )print_color( B_GREEN, "text_buffer_to_slist: Getting buffer contents...\n" );

	GtkTextIter start;
	GtkTextIter end;
	gchar *text;
	gchar **parts;
	gchar **p;

	gtk_text_buffer_get_start_iter( buffer, &start );
	gtk_text_buffer_get_end_iter( buffer, &end );

	text = gtk_text_buffer_get_text( buffer, &start, &end, FALSE );

	if ( DEBUG )g_print( "Value: %s\n", text );

	parts = g_strsplit( text, "\n", -1 );

	if ( DEBUG )dump_vector( parts );

	p = parts;

	while( *p != NULL ) {
		list = g_slist_append( list, g_strdup( *p ) );
		p++;
	}

	g_free( text );
	g_strfreev( parts );

	return list;
}



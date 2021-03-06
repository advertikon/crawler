#include "header.h"

char* code;
char *pckg_tmp_dir = ".tpm_pckg";
char *upload_folder = "upload/";
char *crawler_storage_dir = "/var/www/html/crawler/";
char *pckg_name_templ = "%s-%s-%i.%i.%i.ocmod.zip";
char *pckg_name_regex = "%s-[^-]+-([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.ocmod\\.zip";
char *lang_prefix_23 = "language/en-gb/";
char *lang_prefix_20 = "language/english/";
char* cwd;

static off_t total_size = 0;

int files_count;

size_t path_max_size;

GHashTable *config;
GSList *filter_names = NULL;

// Filters list to path each file through
GSList *filters = NULL;

// List of package's files
GSList *files;

// Lists to store filters temporary during file system iteration to boost up performance
GSList *include_file_temp;
GSList *exclude_file_temp;
GSList *include_folder_temp;
GSList *exclude_folder_temp;
GSList *include_regex_temp;
GSList *exclude_regex_temp;

// Translations
GSList *catalog_translation = NULL;
GSList *admin_translation = NULL;

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
GtkTextBuffer *buffer_file_list;

// Config manage buttons
GtkButton *button_reload_config;
GtkButton *button_save_config;
GtkButton *button_delete_config;

// Delete package config dialog
GtkDialog *delete_package_confirm;

// Delete package confirm dialog's buttons
GtkButton *button_delete_package_ok;
GtkButton *button_delete_package_cancel;

// Iterate over FS button
GtkButton *button_iterate;

// Button to make package
GtkButton *button_make_package;

// Select package event handler ID
gulong select_package_handler = 0;

// Iterate over FS button handler
gulong button_iterate_handler = 0;

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

	/*********** Main window **********/
    window = GTK_WIDGET( gtk_builder_get_object( UI_builder, "window1" ) );

    // Close the program
    g_signal_connect (window, "destroy", G_CALLBACK ( destroy ), NULL);

    /********** Select package combobox **********/
    select_package = GTK_COMBO_BOX_TEXT( gtk_builder_get_object( UI_builder, "select_package" ) );

    // Select configuration file action
    select_package_handler = g_signal_connect ( select_package, "changed", G_CALLBACK ( fill_in_config ), NULL);

    /********** Module code input **********/
    input_code = GTK_ENTRY( gtk_builder_get_object( UI_builder, "input_code" ) );
 
    /********* Version numbers **********/
    input_major = GTK_SPIN_BUTTON( gtk_builder_get_object( UI_builder, "input_major" ) );
    input_minor = GTK_SPIN_BUTTON( gtk_builder_get_object( UI_builder, "input_minor" ) );
    input_patch = GTK_SPIN_BUTTON( gtk_builder_get_object( UI_builder, "input_patch" ) );

    /********* Filter inputs **********/
    buffer_include_file   = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_include_file" ) );
    buffer_exclude_file   = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_exclude_file" ) );
    buffer_include_folder = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_include_folder" ) );
    buffer_exclude_folder = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_exclude_folder" ) );
    buffer_include_regex  = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_include_regex" ) );
    buffer_exclude_regex  = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_exclude_regex" ) );

    /********* Configuration buttons **********/
    button_reload_config = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_reload_config" ) );
    button_save_config   = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_save_config" ) );
    button_delete_config = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_delete_config" ) );

    // Reload configuration files click handler
    g_signal_connect ( button_reload_config, "clicked", G_CALLBACK ( reload_config ), NULL);

    // Save configuration files click handler
    g_signal_connect ( button_save_config, "clicked", G_CALLBACK ( save_config ), NULL);

    // Delete configuration files click handler
    g_signal_connect ( button_delete_config, "clicked", G_CALLBACK ( delete_config ), NULL);

    /******** Confirm delete package dialog ********/
    delete_package_confirm = GTK_DIALOG( gtk_builder_get_object( UI_builder, "delete_package_confirm" ) );
    button_delete_package_ok = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_delete_package_ok" ) );
    button_delete_package_cancel = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_delete_package_cancel" ) );

    // Delete configuration dialog OK button click handler
    g_signal_connect ( button_delete_package_ok, "clicked", G_CALLBACK ( _delete_config ), NULL);

    // Delete configuration dialog hide
    g_signal_connect ( button_delete_package_cancel, "clicked", G_CALLBACK ( delete_config_hide ), NULL);

    /********** Configuration buttons ********/
    button_reload_config = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_reload_config" ) );

    /********* Iterate button *********/
    button_iterate = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_iterate" ) );

    // Click button event handler
    button_iterate_handler = g_signal_connect ( button_iterate, "clicked", G_CALLBACK( get_files ), NULL);

    /********* Files list **********/
    buffer_file_list = GTK_TEXT_BUFFER( gtk_builder_get_object( UI_builder, "buffer_file_list" ) );

    /********* Make package button *********/
    button_make_package = GTK_BUTTON( gtk_builder_get_object( UI_builder, "button_make_package" ) );

    // Click button event handler
    g_signal_connect ( button_make_package, "clicked", G_CALLBACK( make_package ), NULL );
  
    gtk_widget_show( window );

    // Populate select package combobox with data
    get_package_configs();
    init_filters();
    gtk_main();

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

	g_free( list );

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

/**
 * Fills in files list. Iterate button click event handler
 */
void get_files( void* widget, void* data ) {
	int debug = 0;

	if ( DEBUG || debug ) print_color( B_CYAN, ">>>>>> GET PACKAGE FILES <<<<<<<<" );

	include_file_temp = g_hash_table_lookup( config, "include_file" );
	exclude_file_temp = g_hash_table_lookup( config, "exclude_file" );
	include_folder_temp = g_hash_table_lookup( config, "include_folder" );
	exclude_folder_temp = g_hash_table_lookup( config, "exclude_folder" );
	include_regex_temp = g_hash_table_lookup( config, "include_regex" );
	exclude_regex_temp = g_hash_table_lookup( config, "exclude_regex" );
	g_slist_free_full( files, (GDestroyNotify)g_free );
	files = NULL;

	iterate( g_strdup( cwd ), check_item, NULL, on_iterate_error, NULL );

	load_dependencies();
	files_to_view();
}

/**
 * Fills in view with package files
 */
void files_to_view() {
	int debug = 0;

	GSList *current = files;
	GtkTextIter end;

	char line[ path_max_size ];

	if ( DEBUG || debug )print_color( B_GREEN, "files to view" ); 

	gtk_text_buffer_set_text( buffer_file_list, "", -1 );

	while ( current ) {
		gtk_text_buffer_get_end_iter( buffer_file_list, &end );

		memset( line, 0, path_max_size );
		strncpy( line, current->data, strlen( current->data ) );
		strcat( line, "\n" );

		if ( DEBUG || debug )printf( "To buffer view: '%s'\n", line );

		gtk_text_buffer_insert( buffer_file_list, &end, line, -1 );

		current = current->next;
	}

	if ( DEBUG || debug )printf( "Files to view end\n" );
}

/**
 * Checks item (file) and add it to package files list on success 
 */
int check_item( char *name, void* data ) {
	if ( DEBUG )fprintf( stderr, "Start checking file %s\n", name );

	if ( check_file( (char*)name ) ) {
		if ( DEBUG )print_color( B_GREEN, "Passed check: %s\n", name );

		files = g_slist_append( files, g_strdup( name ) );
		total_size += filesize( name );
		files_count++;

	} else {
		if ( DEBUG )fprintf( stderr, "Check failed\n" );
	}

	return 0;
}

/**
 * Add files from header source section to package files list
 */
int check_source( char *name, void *data ) {
	int debug = 0;

	if ( DEBUG || debug )fprintf( stderr, "Add source file %s\n", name );

	// If file doesn't exist already in list - add it
	if ( -1 == g_slist_index( files, name ) ) {
		if ( DEBUG || debug )printf( "File is not in the list. Add it\n" );

		files = g_slist_append( files, g_strdup( name ) );
		total_size += filesize( name );
		files_count++;
	}

	return 0;
}

/**
 * Callback for FS iterator
 * Prints error in STDERR end returns status 1
 */
int on_iterate_error( char *name, void *data ) {
	fprintf( stderr, "%s: %s\n", strerror( errno ), name );

	return 1;
}

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
	int show = 0;

	if ( DEBUG || debug )fprintf( stderr, "Start checking file: '%s'\n", name );

	int max_incl_dir_length = 0;
	int max_excl_dir_length = 0;

	char *relative_file_name = &name[ strlen( cwd ) + 1 ];
	char *relative_folder_name;

	if ( DEBUG )printf( "Relative file name: '%s'\n", relative_file_name );

	// File is in the include list
	if ( NULL != g_slist_find( include_file_temp, relative_file_name ) ) {
		if ( DEBUG || debug || show )fprintf( stderr, "Is in included files list (%s)\n", name );
		return TRUE;
	}

	// File is in the exclude list
	if (  NULL != g_slist_find( exclude_file_temp, relative_file_name ) ) {
		if ( DEBUG || debug || show )fprintf( stderr, "Is in excluded files list(%s)\n", name );
		return FALSE;
	}

	relative_folder_name = g_path_get_dirname( relative_file_name );

	if ( DEBUG )printf( "Relative folder: '%s'\n", relative_folder_name );

	// If path has folder part
	if ( strncmp( ".", relative_folder_name, 1 ) != 0 ) {
		if ( DEBUG || debug )fprintf( stderr, "Check as directory: '%s'\n", relative_folder_name );

		collide_length( relative_folder_name, include_folder_temp, &max_incl_dir_length );
		collide_length( relative_folder_name, exclude_folder_temp, &max_excl_dir_length );

		g_free( relative_folder_name );

		if ( DEBUG || debug )
			fprintf(
				stderr,
				"Included directory max. length: %d, excluded directory max. length: %d\n",
				max_incl_dir_length,
				max_excl_dir_length
			);
		
		if ( max_incl_dir_length > 0 && max_incl_dir_length >= max_excl_dir_length ) {
			if ( DEBUG || debug || show )fprintf( stderr, "Passed as directory (%s)\n", name );
			return TRUE;
		}

		if ( max_excl_dir_length > 0 ) {
			if ( DEBUG || debug || show )fprintf( stderr, "Rejected as directory (%s)\n", name );
			return FALSE;
		}

	} else {
		g_free( relative_folder_name );
	}

	if ( DEBUG || debug )fprintf( stderr, "Check against regex\n");

	if ( TRUE == check_regexp( name, include_regex_temp ) && FALSE == check_regexp( name, exclude_regex_temp ) ) {
		if ( DEBUG || debug || show )fprintf( stderr, "Passed as regex (%s)\n", name );

		return TRUE;
	}

	if ( DEBUG || debug )fprintf( stderr, "No filter rules applied. Skip\n");

	return FALSE;
}

/**
 * Returns longest path
 */
void collide_length( char *name, GSList *list, int *max ) {
	int debug = 0;
	int current_str_len = 0;
	*max = 0;
	GSList *current = list;

	while ( NULL != current ) {
		if ( strstr( name, current->data ) != NULL ) {
			current_str_len = strlen( current->data );
			if ( DEBUG || debug )printf( "Str1 '%s', str2: '%s', span: %i\n", name, (char*)current->data, current_str_len );
		}

		// We already have longer match. Skip
		if ( *max < current_str_len ) {
			*max = current_str_len;
		}

		current = current->next;
	}
}

/**
 * Checks path against list of regexps
 *
 */
gboolean check_regexp( char* str, GSList *list ) {
	GSList *current = list;
	int debug = 0;

	if ( DEBUG || debug )fprintf( stderr, "Checking file '%s' name against regex\n", str );

	while ( NULL != current ) {
		if ( DEBUG || debug )fprintf( stderr, "Regex: '%s'\n", (char*)current->data );

		if ( g_regex_match_simple ( current->data, str, 0, 0 ) ) {
			if ( DEBUG || debug )fprintf( stderr, "Match\n" );

			return TRUE;
		}

		current = current->next;
	}

	if ( DEBUG || debug )fprintf( stderr, "No match found\n" );

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
 */
int load_dependencies() {
	int debug = 0;

	if ( DEBUG || debug )fprintf( stderr, "Start loading dependencies\n" );

	FILE *file;                         // File to search in

	char *line = g_malloc0( MAX_LINE ); // Line from file
	char *source_name, *sn;                  // Name of source file got from script header

	gboolean in_header = FALSE;         // Flag to tell whether we are in script header

	int max_lines = 200;                // Max depth to look at
	int c = 0;                          // Line count
	int is_free_match = 0;              // Flag to tell if matches_info structure needs to be emptied

	GRegex *regex = g_regex_new( "\\*\\s+@source\\s+([^*]+)", 0, 0, NULL );
	GMatchInfo *match_info;

	GSList *current = files;

	while ( current ) {
		if ( DEBUG || debug )fprintf( stderr, "Iterate: '%s'\n", (char*)current->data );

		// Look for sources in controller files
		if ( NULL != strstr( current->data, "/controller/" ) ) {
			if ( DEBUG || debug )fprintf( stderr, "Is controller\n" );

			if ( NULL == ( file = fopen( current->data, "r" ) ) ) {
				fprintf( stderr, "Failed to open file '%s' in %s:%i: %s\n", (char*)current->data, __FILE__, __LINE__, strerror( errno ) );
				exit( 1 );
			}

			c = 0;

			if ( DEBUG || debug  )fprintf( stderr, "Open file: '%s'\n", (char*)current->data );

			// Read up to max_line lines
			while ( NULL != ( fgets( line, MAX_LINE, file ) ) ) {
				line = g_strchug( line ); // Remove leading spaces

				if ( DEBUG || debug )fprintf( stderr, "Line: '%s'", line );

				if ( ++c > max_lines ) {
					fprintf( stderr, "Maximum depth of %d lines reached in file %s\n", max_lines, (char*)current->data );
					exit( 1 );
				}

				if ( in_header ) {
					if ( 0 == strncmp( line, "*/", 2 ) ) {
						if ( DEBUG || debug )fprintf( stderr, "Header closing tag\n" );
						goto next_file;
					}

					is_free_match = 1; // Need to free match_info

					if ( g_regex_match ( regex, line, 0, &match_info ) ) {
						sn = g_match_info_fetch( match_info, 1 ); // Will be freed in iterate function
						g_strstrip( sn );

						if ( strlen( sn ) ) {
							if ( DEBUG || debug )printf( "Found source: '%s'\n", sn );

							// Source names are always relative
							if ( strncmp( "/", sn, 1 ) != 0 ) {
								source_name = g_build_filename( cwd, sn, NULL );
								g_free( sn );

							} else {
								source_name = sn;
							}

							iterate( source_name, check_source, NULL, on_iterate_error, NULL ); // Add to files list

						} else {
							fprintf( stderr, "Failed to fetch source name from string '%s'\n", line );
							exit( 1 );
						}
					}

				} else {
					if ( 0 == strncmp( line, "/**", 3 ) ) {
						if ( DEBUG || debug )fprintf( stderr, "Header opening tag\n" );
						in_header = TRUE;
						continue;
					}
				}
			}

			if ( ferror( file ) ) {
				fprintf(
					stderr,
					"Failed to read file '%s' fetching dependencies in %s: %i: %s\n",
					(char*)current->data,
					__FILE__,
					__LINE__, 
					strerror( errno )
				);
				exit( 1 );
			}

		next_file:
			fclose( file );
			in_header = FALSE;
		}

		if ( DEBUG || debug ) {
			fprintf( stderr, "Next loop\n" );
		}

		current = current->next;
	}

	if ( is_free_match )
		g_match_info_free (match_info);

	g_free( line );
	g_regex_unref (regex);
}

/**
 * Makes package
 */
int make_package() {
	int debug = 0;

	if ( DEBUG || debug )print_color( B_CYAN, ">>>>>>> MAKE PACKAGE <<<<<<<" );

	char *upload_path = NULL;
	char *pckg_name = NULL;
	char *pckg_dir = NULL;
	char version[ VERSION_SIZE ];

	GSList *list_major, *list_minor, *list_patch;
	GSList *code;

	int s;
	int major, minor, patch;
	int status = 0;

	if ( 0 == g_slist_length( files ) ) {
		fprintf( stderr, "Files list is empty. Nothing to process\n" );
		status = 1;
		goto exit_point;
	}

	update_config_from_view();
	code = g_hash_table_lookup( config, "code" );

	if ( NULL == code ) {
		fprintf( stderr, "Extension's code is mandatory\n" );
		status = 1;
		goto exit_point;
	}

	if ( 0 != get_version( &major, &minor, &patch ) ) {
		status = 1;
		goto exit_point;
	}

	// Directory to store packages in
	pckg_dir = get_package_dir();

	if ( g_mkdir_with_parents( pckg_dir, 0775 ) < 0 ) {
		fprintf(
			stderr,
			"make_package: Failed to create folder '%s in %s:%i': %s\n",
			pckg_dir,
			__FILE__,
			__LINE__,
			strerror( errno )
		);

		status = 1;
		goto exit_point;
	}

	start_clock();
	if( fill_temp_package() ) exit( 1 );
	end_clock( "Fill in temp structure" );

	start_clock();
	if( make_vqmod() ) exit( 1 );
	end_clock( "VQMOD" );

	// Clean translation lists
	init_translation_lists();

	start_clock();
	if( run_filters() ) exit( 1 );
	end_clock( "Filters" );
	
	start_clock();
	if( php_lint() ) exit( 1 );
	end_clock( "php lint" );

	// Tun it after filters, since filters fill in translation lists
	if ( save_all_the_translations() ) exit( 1 );

	pckg_name = g_strdup_printf( pckg_name_templ, code->data, "OC23", major, minor, patch );
	run_zip( pckg_name );
	g_free( pckg_name );

	start_clock();
	if( make_oc20() ) exit( 1 );
	end_clock( "Make OC20" );

	pckg_name = g_strdup_printf( pckg_name_templ, code->data, "OC20", major, minor, patch );
	start_clock();
	if ( run_zip( pckg_name ) ) exit( 1 );

	fprintf( stderr, "Package was saved under version: %i.%i.%i\n", major, minor, patch );
	fprintf( stderr, "Saving configuration\n" );

	save_config( NULL, NULL );

exit_point:
	if ( NULL != pckg_dir) g_free( pckg_dir );
	if ( NULL != pckg_name ) g_free( pckg_name );

	return status;
}

/**
 * Returns path to folder where zipped packages stored with respect to package name
 */
char *get_package_dir() {
	int debug = 0;

	if ( DEBUG || debug )printf( "Getting package temporary directory...\n" );

	char *code = get_code();
	char *pckg_dir = NULL;

	if ( NULL == code ) {
		fprintf( stderr, "Failed to fetch package code name in %s:%i\n", __FILE__, __LINE__ );
		return NULL;
	}

	pckg_dir = g_build_filename( crawler_storage_dir, code, NULL );

	g_free( code );

	if ( DEBUG || debug )printf( "Package directory: '%s'\n", pckg_dir );

	return pckg_dir;
}

/**
 * Fetches package code name from configuration. Need to be freed
 */
char *get_code() {
	GSList *list = g_hash_table_lookup( config, "code" );

	if ( NULL == list ) {
		fprintf( stderr, "Code name is mussing from configuration in %s:%i\n", __FILE__, __LINE__ );
		return NULL;
	}

	return g_strdup( list->data );
}

/**
 * Get next version numbers for the package. Gets values form the config and checks them against
 * saved packages
 */
int get_version( int *in_major, int *in_minor, int *in_patch ) {
	int debug = 0;

	if ( DEBUG || debug )printf( "Get version\n" );

	int f_major = 0;
	int f_minor = 0;
	int f_patch = 0;

	int t_major = 0;
	int t_minor = 0;
	int t_patch = 0;

	gboolean is_empty_dir = TRUE;

	char regex_pattern[ MAX_LINE ];
	char *pckg_dir;
	char *temp;
	char *code;

	GRegex *regex;
	GMatchInfo *match_info;

	GSList *major = g_hash_table_lookup( config, "major" );
	GSList *minor = g_hash_table_lookup( config, "minor" );
	GSList *patch = g_hash_table_lookup( config, "patch" );

	int configuration_major = atoi( major->data );
	int configuration_minor = atoi( minor->data );
	int configuration_patch = atoi( patch->data );

	DIR *dir;
	struct dirent *entry;

	pckg_dir = get_package_dir();
	code = get_code();

	if ( DEBUG ||debug )printf( "Package directory '%s'\n", pckg_dir );

	if ( NULL != ( dir = opendir( pckg_dir ) ) ) {
		sprintf( regex_pattern, pckg_name_regex, code );

		if ( DEBUG || debug )printf( "Regx: '%s'\n", regex_pattern );

		regex = g_regex_new( regex_pattern, 0, 0, NULL );

		while ( NULL != ( entry = readdir( dir ) ) ) {
			if ( entry->d_name[ 0 ] == '.' ) continue;

			if ( DEBUG || debug ) fprintf( stderr, "Processing file '%s'\n", entry->d_name );
			if ( DEBUG || debug ) fprintf( stderr, "Regex '%s'\n", regex_pattern );

			if ( g_regex_match( regex, entry->d_name, 0, &match_info ) ) {
				if ( DEBUG || debug ) fprintf( stderr, "Match\n" );

				is_empty_dir = FALSE;

				if ( DEBUG || debug ) {
					temp = g_match_info_fetch( match_info, 0 );

					if ( NULL != temp ) {
						fprintf( stderr, "Full match: '%s'\n", temp );
						g_free( temp );
					}
				}

				temp = g_match_info_fetch( match_info, 1 );

				if ( NULL != temp ) {
					t_major = atoi( temp );
					g_free( temp );
				}

				temp = g_match_info_fetch( match_info, 2 );

				if ( NULL != temp ) {
					t_minor = atoi( temp );
					g_free( temp );
				}

				temp = g_match_info_fetch( match_info, 3 );

				if ( NULL != temp ) {
					t_patch = atoi( temp );
					g_free( temp );
				}

				if ( DEBUG || debug ) fprintf( stderr, "Version numbers found: %d.%d.%d\n", t_major, t_minor, t_patch );

				// Major number of current version is greater then previous
				if ( t_major > f_major ) {
					f_major = t_major;
					f_minor = t_minor;
					f_patch = t_patch;

				// Major numbers are equal, minor number of current version is greater them previous
				} else if ( t_major == f_major && t_minor > f_minor ) {
					f_minor = t_minor;
					f_patch = t_patch;

				// Patch number
				} else if ( t_major == f_major && t_minor == f_minor && t_patch > f_patch ) {
					f_patch = t_patch;
				}

				if ( DEBUG || debug ) fprintf( stderr, "Intermediate version: %d.%d.%d\n", f_major, f_minor, f_patch );

				g_match_info_free( match_info );
			}
		}

		if ( DEBUG || debug )printf( "Configuration version: %i.%i.%i, disc version: %i.%i.%i\n", configuration_major, configuration_minor, configuration_patch, f_major, f_minor, f_patch );

		// Current configuration version equals the latest package version - increment patch number
		if ( configuration_major == f_major && configuration_minor == f_minor && configuration_patch == f_patch && !is_empty_dir) {
			if ( DEBUG || debug ) fprintf( stderr, "Patch number was automatically incremented\n" );

			g_free( patch->data );
			patch->data = g_strdup_printf( "%i", ++f_patch );
			update_config_view();
			configuration_patch++;
		}

		g_free( pckg_dir );
		g_regex_unref( regex );
		g_free( code );

	} else {
		fprintf( stderr, "Package directory '%s' doesn't exist in %s:%i\n", pckg_dir, __FILE__, __LINE__ );
		exit( 1 );
	}

	if ( DEBUG || debug ) fprintf( stderr, "Version: %i.%i.%i\n", f_major, f_minor, f_patch );

	// Patch number may be equal in case of first release, version will be 0.0.0
	if (
		configuration_major < f_major ||
		( ( configuration_major == f_major ) && configuration_minor < f_minor ) ||
		( ( configuration_major == f_major ) && ( configuration_minor == f_minor ) && configuration_patch < f_patch ) ) {
		fprintf(
			stderr,
			"Can not create package with version (%i.%i.%i) which is less then existing one(%i.%i.%i)\n",
			configuration_major,
			configuration_minor,
			configuration_patch,
			f_major,
			f_minor,
			f_patch
		);

		return 1;
	}

	*in_major = configuration_major;
	*in_minor = configuration_minor;
	*in_patch = configuration_patch;

	return 0;
}

/**
 * Spawns ZIP to make zipped package
 */
int run_zip( char *package_name ) {
	int debug = 0;

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

	// strcpy( src_path, pckg_tmp_dir );

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

	path = g_build_filename( get_package_dir(), package_name, NULL );

	if ( DEBUG || debug ) fprintf( stderr, "Creating ZIP archive... '%s'\n", path );

	// ZIP everything that are in CWD and save as path
	if ( execlp( "zip", "zip", "-r", mode, path, ".", (char*)0 ) < 0 ) {
		print_error( "run_zip: Failed to exec zip" );
	}
}

/**
 * Fills in temporary package structure before ZIPping
 * Implied CWD
 */
int fill_temp_package() {
	int debug = 0;

	if ( DEBUG || debug )printf("Filling in temporary folder\n" );

	int r_count;
	int status = 0;
	int dest = -1;
	int src = -1;
	int is_copy = 0;
	int cwd_len = strlen( cwd );

	char *path;
	char *folder;
	char *copy = NULL;
	char *file_name;
	char *code;
	char *readme_file_name;
	char* relative_file_name;
	char buffer[ BUFF_SIZE ];

	GSList *current = files;

	struct stat *sb = NULL;

	code = get_code();

	readme_file_name = Strcat( code, "_readme", NULL ); // Can be anywhere in the path, lowercase only

	if ( DEBUG || debug ) fprintf( stderr, "Deleting temporary files...\n" );

	// Clean temporary folder
	folder = g_build_filename( cwd, pckg_tmp_dir, NULL ); // Will be freed in iterator
	iterate( folder, del_file_cb, del_dir_cb, on_iterate_error, NULL );

	while( NULL != current ) {
		file_name = &((char*)current->data)[ cwd_len ];

		if ( DEBUG || debug )fprintf( stderr, "Processing file '%s'\n", file_name );

		// Get file mode to be able to preserve it
		// TODO: directories in filename will inherit permissions as well
		if ( NULL == ( sb = Lstat( current->data ) ) ) {
			status = 1;
			goto exit_point;
		}

		if ( ( src = open( current->data, O_RDONLY  ) ) < 0 ) {
			fprintf(
				stderr,
				"fill_temp_package: Failed to open file '%s' in %s:%i: %s\n",
				(char*)current->data,
				__FILE__,
				__LINE__,
				strerror( errno )
			);

			status = 1;
			goto exit_point;
		}

		if ( DEBUG || debug )fprintf( stderr, "Open file '%s'\n", (char*)current->data );

		// Place OCMOD in package root
		if ( 0 == strcmp( ".ocmod.xml", &file_name[ strlen( file_name ) - 10 ] ) ) {
			if ( DEBUG || debug )fprintf( stderr, "OCMOD file found '%s'\n", (char*)current->data );

			// Create OCMOD which need to be installed via Extension Installer
			copy = g_build_filename( cwd, pckg_tmp_dir, "install.xml", NULL );

			// Create OCMOD which can be installed directly
			path = g_build_filename( cwd, pckg_tmp_dir, strrchr( file_name, '/' ), NULL );

		// Place README file in the package root
		} else if ( strstr( file_name, readme_file_name ) != NULL ) {
			if ( DEBUG || debug ) fprintf( stderr, "Readme file found: '%s'\n", (char*)current->data );

			path = g_build_filename( cwd, pckg_tmp_dir, "README.TXT", NULL );

		} else {
			path = g_build_filename( cwd, pckg_tmp_dir, upload_folder, file_name, NULL ); // Get relative path
		}

	copy:
		if ( DEBUG || debug )fprintf( stderr, "Creating file '%s'\n", path );

		if ( ( dest = make_file( path, sb->st_mode ) ) < 0 ) {
			fprintf( stderr, "fill_temp_package: Failed to save file '%s' in %s: %s\n", path, G_STRLOC, strerror( errno ) );
			status = 1;
			goto exit_point;
		}

		if ( DEBUG || debug )fprintf( stderr, "File '%s' created\n", path );

		while ( ( r_count = read( src, buffer, BUFF_SIZE ) ) > 0 ) {
			if ( -1 == write( dest, buffer, r_count ) ) {
				fprintf( stderr, "fill_temp_package: write file error in %s: %s", G_STRLOC, strerror( errno ) );
				status = 1;
				goto exit_point;
			}
		}

		if ( -1 == r_count ) {
			fprintf( stderr, "fill_temp_package: read file error in %s: %s", G_STRLOC, strerror( errno ) );
			status = 1;
			goto exit_point;
		}

		if ( DEBUG || debug )printf( "File '%s' was filled up with contents\n", (char*)current->data );

		close( dest );
		dest = -1;

		if ( NULL != copy ) {
			g_free( path );
			path = copy;
			copy = NULL;

			if ( lseek( src, 0, SEEK_SET ) == -1 ) {
				fprintf( stderr, "fill_temp_package: Failed to rewind source file in %s\n", G_STRLOC );
				goto exit_point;
			}

			goto copy;
		}

		close( src );
		src = -1;

		current = current->next;
	}

exit_point:
	if ( NULL != sb )g_free( sb );
	if ( NULL != copy )g_free( copy );
	if ( NULL != path )g_free( path );
	if ( dest > -1 )close( dest );
	if ( src > -1 )close( src );
	g_free( code );

	return status;
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

	d_name = g_path_get_dirname( name );

	if ( "." != d_name ) {
		if( -1 == g_mkdir_with_parents( d_name, 0777 ) ) {
			fprintf( stderr, "Failed to create folder '%s' in %s:%i: %s\n", d_name, __FILE__, __LINE__, strerror( errno ) );
			g_free( d_name );

			return -1;
		}
	}

	g_free( d_name );
	fd = creat( name, mode );

	if ( -1 == fd ) {
		fprintf( stderr, "Failed to create file '%s' in %s:%i: %s\n", name, __FILE__, __LINE__, strerror( errno ) );
	} 

	return fd;
}

/**
 * Scan file and fill in structures of translations
 */
int fill_translation( FILE* f, char *name ) {
	int debug = 0;

	char *p;
	name = &name[ strlen( cwd ) + 1 ]; // Make it relative to CWD

	char *catalog_prefix = g_build_filename( pckg_tmp_dir, upload_folder, "catalog", NULL );
	char *admin_prefix = g_build_filename( pckg_tmp_dir, upload_folder, "admin", NULL );

	int admin_prefix_length = strlen( admin_prefix );
	int catalog_prefix_length = strlen( catalog_prefix );

	if ( DEBUG || debug ) fprintf( stderr, "Searching translations in: '%s'\n", name );

	if ( NULL != strstr( name, "/language/" ) ) {
		if ( DEBUG || debug ) fprintf( stderr, "Is language file - skip\n" );

		return 0; // Skip language files
	}

	p = strrchr( name, '.' );

	if ( NULL == p || ( strcmp( p, ".php" ) != 0 && strcmp( p, ".xml" ) != 0 && strcmp( p, ".tpl" ) != 0 ) ) {
		if ( DEBUG || debug ) fprintf( stderr, "Wrong type - skip\n" );

		return 0; // Skip some files
	}

	if ( 0 == strncmp( name, catalog_prefix, catalog_prefix_length ) ) {
		if ( DEBUG || debug ) fprintf( stderr, "Catalog side\n" );

		fetch_translation( f, &catalog_translation, NULL, FALSE );

	} else if ( 0 == ( strncmp( name, admin_prefix, admin_prefix_length ) ) ) {
		if ( DEBUG || debug ) fprintf( stderr, "Admin side\n" );

		fetch_translation( f, &admin_translation, NULL, FALSE );

	} else {
		if ( DEBUG || debug ) fprintf( stderr, "Common side\n" );

		fetch_translation( f, &admin_translation, &catalog_translation, TRUE );
	}

	g_free( admin_prefix );
	g_free( catalog_prefix );

	return 0;
}

/**
 * Scans file pointed to by f and fills in structure l
 */
int fetch_translation( FILE* f, GSList **list1, GSList **list2, gboolean both_lists ) {
	int debug = 0;

	if ( DEBUG || debug )printf( "Parsing file for translations\n" );

	char line[ MAX_LINE ];
	char *translation;

	GError *error = NULL;
	GRegex *regex = g_regex_new( "__\\( \\s* ('|\") (.+?) (?<!\\\\)\\1 [^)]* \\)", G_REGEX_EXTENDED, 0, &error );
	GMatchInfo *match_info;

	if ( NULL != error ) {
		printf( "Regex error: %s in %s\n", error->message, G_STRLOC );
		exit( 1 );
	}

	rewind( f );

	while( NULL != fgets( line, MAX_LINE, f ) ) {
		// if ( debug ) fprintf( stderr, "Processing line '%s'", line );

		g_regex_match( regex, line, 0, &match_info );

		while ( g_match_info_matches( match_info ) ) {
			// if ( debug ) fprintf( stderr, "Processing line '%s'", line );

			translation = g_match_info_fetch( match_info, 2 );
			g_strstrip( translation );

			if ( debug ) fprintf( stderr, "Match found: '%s'\n", translation );

			if ( '\0' != translation[ 0 ] && -1 == SearchList( *list1, translation ) ) {
				*list1 = g_slist_append( *list1, g_strdup( translation ) );
			}

			if ( both_lists ) {
				if ( '\0' != translation[ 0 ] && -1 == SearchList( *list2, translation ) ) {
					*list2 = g_slist_append( *list2, g_strdup( translation ) );
				}
			}

			g_free( translation );
			g_match_info_next (match_info, NULL);
		}

		g_match_info_free ( match_info );
	}

	g_regex_unref( regex );

	if ( ferror( f ) ) {
		fprintf( stderr, "fetch_translation: read file error: %s\n", strerror( errno ) );
		exit( 1 );
	}

	return 0;
}

/**
 * Runs php linter on files under temporary folder
 */
int php_lint() {
	char *path = g_build_filename( cwd, pckg_tmp_dir, upload_folder, NULL );
	iterate( path, php_lint_cb, NULL, on_iterate_error, NULL );

	return 0;
}

/**
 * Callback for php linter iterator
 */
int php_lint_cb( char *name, void *data ) {
	int debug = 0;

	char *cmd;
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

	if ( g_str_has_suffix( name, "GdBmp.php" ) ) { // it fails to lint since rewrites gd function
		return 0;
	}

	cmd = Strcat( "php -l ", name, " > /dev/null", NULL );

	if ( DEBUG || debug ) printf( "Run command '%s'\n", cmd );

	code = system( cmd );
	g_free( cmd );

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

/**
 * Cleans translation lists
 */
void init_translation_lists() {
	int debug = 0;

	if ( DEBUG || debug ) fprintf( stderr, "Initializing translation lists\n" );

	if ( NULL != catalog_translation ) {
		if ( DEBUG || debug ) fprintf( stderr, "Freeing catalog translation list\n" );

		g_slist_free_full( catalog_translation, g_free );
		catalog_translation = NULL;
	}

	if ( NULL != admin_translation ) {
		if ( DEBUG || debug ) fprintf( stderr, "Freeing admin translation list\n" );

		g_slist_free_full( admin_translation, g_free );
		admin_translation = NULL;
	}
}

/**
 * Saves translations form lists to files
 */
int save_all_the_translations() {
	int debug = 0;

	if ( DEBUG || debug ) fprintf( stderr, "Saving translation to the disk\n" );

	if ( g_slist_length( catalog_translation ) ) {
		catalog_translation = g_slist_sort( catalog_translation, (GCompareFunc)strcmp );

		if ( DEBUG || debug ) {
			fprintf( stderr, "Catalog side translations:\n" );
			dump_slist( catalog_translation );
		}

		start_clock();
		save_translation( "catalog", catalog_translation );
		end_clock( "Translation catalog" );
		// g_slist_free_full( catalog_translation, (GDestroyNotify)g_free );
	}

	if ( g_slist_length( admin_translation ) ) {
		admin_translation = g_slist_sort( admin_translation, (GCompareFunc)strcmp );

		if ( DEBUG || debug ) {
			fprintf( stderr, "Admin side translations:\n" );
			dump_slist( admin_translation );
		}

		start_clock();
		save_translation( "admin", admin_translation );
		end_clock( "Translation admin" );
		// g_slist_free_full( admin_translation, (GDestroyNotify)g_free );
	}

	return 0;

}

// TODO: implement catalog translation maybe
/**
* Saves translation file into temporary directory
* name - admin or catalog
* l - list off all the translation for specific side
*/
int save_translation( char *name, GSList *l ) {
	int debug = 0;

	if ( DEBUG || debug ) fprintf( stderr, "Saving translations for %s...\n", name );

	int LEN = 1000; // Presume translation can have such length

	FILE *from_stream = NULL;
	FILE *to_stream = NULL;

	char buff[ LEN ];
	char *from_name;
	char *to_name;
	char *translation_folder;
	char *code;
	char *file_name;
	char *to_folder;

	GSList *current;

	translation_folder = get_common_dir();
	code = get_code();
	file_name = Strcat( code, ".php", NULL );

	from_name = g_build_filename( cwd, name, "/language/en-gb/", translation_folder, file_name, NULL );
	to_name = g_build_filename( cwd, pckg_tmp_dir, upload_folder, name, "/language/en-gb/", translation_folder, file_name, NULL );

	to_folder = g_path_get_dirname( to_name );

	if( -1 == g_mkdir_with_parents( to_folder, 0777 ) ) {
		fprintf( stderr, "Failed to create folder '%s' in %s:%i: %s\n", to_folder, __FILE__, __LINE__, strerror( errno ) );
		exit( 1 );
	}

	if ( DEBUG || debug ) {
		fprintf( stderr, "Source path: '%s'\nTarget path: '%s'\n", from_name, to_name );
	}

	if ( NULL == ( to_stream = fopen( to_name, "w+" ) ) ) {
		fprintf( stderr, "save_translation: failed to open file '%s': %s\n", to_name, strerror( errno ) );
		exit( 1 );
	}

	fputs( "<?php\n", to_stream );

	if ( DEBUG || debug ) fprintf( stderr, "File '%s' opened\n", to_name );

	if ( NULL != ( from_stream  = fopen( from_name, "r" ) ) ) {
		if ( DEBUG || debug ) fprintf( stderr, "File '%s' opened\n", from_name );

		while ( NULL != fgets( buff, LEN, from_stream ) ) {

			// Save predefined translations
			if (
				!g_regex_match_simple( "\\$_\\[\\s*'\\s*heading_title\\s*'", buff, 0, 0 ) &&
				!g_regex_match_simple( "\\$_\\[\\s*'\\s*text_advertikon_stripe\\s*'", buff, 0, 0 )
			) {
				continue;
			}

			if ( EOF == fputs( buff, to_stream ) ) {
				fprintf( stderr, "save_translation: file '%s' writing error: %s\n",to_name, strerror( errno ) );
				exit( 1 );
			}
		}

		if ( ferror( from_stream ) ) {
			fprintf( stderr, "save_translation: file '%s' reading error: %s\n", from_name, strerror( errno ) );
			exit( 1 );
		}

	} else if ( ENOENT != errno ) {
		fprintf( stderr, "save_translation: failed to open file '%s': %s\n", from_name, strerror( errno ) );
		exit( 1 );
	}

	current = l;

	if ( DEBUG || debug ) fprintf( stderr, "Writing translations....\n" );

	while ( NULL != current ) {
		if ( debug ) fprintf( stderr, "$_['%1$s'] = '%1$s';\n", (char*)current->data );

		sprintf( buff, "$_['%1$s'] = '%1$s';\n", (char*)current->data );
		fputs( buff, to_stream );
		current = current->next;
	}

	if ( debug ) {
		fprintf( stderr, "From FILE: %p, to FILE %p\n", from_stream, to_stream );
	}


	if ( NULL != from_stream ) {
		fclose( from_stream );
	}

	fclose( to_stream );

	php_lint_cb( to_name, NULL );

	g_free( translation_folder );
	g_free( code );
	g_free( file_name );
	g_free( to_folder );
	g_free( to_name );
	g_free( from_name );

	if ( debug ) fprintf( stderr, "Exit save translation\n" );

	return 0;
}

/**
 * Runs all registered filters on each package files in turn
 */
int run_filters() {
	if ( DEBUG ) fprintf( stderr, "Start filtering...\n" );

	// There are no files to process upon or there are no filters to process with
	if ( NULL == filters || NULL == files ) {
		if ( DEBUG ) fprintf( stderr, "There is no filters to be run\n" );
		return 0;
	}

	iterate( g_build_filename( cwd, pckg_tmp_dir, NULL ), run_filters_cb, NULL, on_iterate_error, NULL );

	return 0;
}

/**
 * Run filters callback
 */
int run_filters_cb( char* file, void* data ) {
	int debug = 0;

	FILE *f;
	char *path;
	char *ext;

	GSList *current_filter;

	path = g_build_filename( cwd, pckg_tmp_dir, upload_folder, NULL );

	if ( DEBUG || debug ) fprintf( stderr, "File '%s'\n", file );

	ext = strrchr( file, '.' );

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
	) return 0;

	if ( NULL == ( f = fopen( file, "r+" ) ) ) {
		fprintf(
			stderr,
			"run_filters: failed to open file '%s' in %s:%i: %s\n",
			file,
			__FILE__,
			__LINE__,
			strerror( errno )
		);

		return 1;
	}

	current_filter = filters;

	while( current_filter ) {
		((callback)current_filter->data)( f, file );
		current_filter = current_filter->next;
	}

	fclose( f );

	return 0;
}

/**
 * Registers filters
 */
int init_filters() {
	if ( DEBUG ) fprintf( stderr, "Initializing filters\n" );

	filters = g_slist_append( filters, fill_translation );
	filters = g_slist_append( filters, add_version );

	return 0;
}

/**
 * FS iterator callback to empty a folder. Unlinks files
 */
int del_file_cb( char *name, void *data ) {
	if ( 0 != unlink( name ) ) {
		fprintf( stderr, "Failed to unlink file '%s' in %s:%i: %s\n", name, __FILE__, __LINE__, strerror( errno ) );

		exit( 1 );
	}

	return 0;
}

/**
 * FS iterator callback to empty a folder. Deletes files
 */
int del_dir_cb( char *name, void *data ) {
	if ( 0 != rmdir( name ) ) {
		fprintf( stderr, "Failed to remove directory '%s' in %s:%i: %s\n", name, __FILE__, __LINE__, strerror( errno ) );

		exit( 1 );
	}

	return 0;
}

/**
 * Returns common part for package files. Eg extension/module. Needs to be freed
 */
char *get_common_dir() {
	char *p;
	char *end;
	GSList *current;

	char lang_dir[ path_max_size ];

	if ( 0 == g_slist_length( files ) ) {
		fprintf( stderr, "get_common_dir: Files list is empty\n" );
		return NULL;
	}

	current = files;
	memset( lang_dir, 0, path_max_size );

	while ( current ) {
		if ( NULL != ( p = strstr( current->data, "/controller/" ) ) ) {
			if ( NULL != ( end = strrchr( p, '/' ) ) ) {

				// With leading slash
				strncpy( lang_dir, p + 12, end - p - 11 );
				return g_strdup( lang_dir );
				
			} else {
				fprintf( stderr, "get_common_dir: failed to fetch common part from controller path '%s'\n", (char*)current->data );
			}
		}

		current = current->next;
	}

	return NULL;
}

/**
 * Makes changes to temporary files structure to make package conforms OC20 restrictions
 */
int make_oc20() {
	int debug = 0;

	void *temp_files = NULL;

	if ( DEBUG || debug ) fprintf( stderr, "Making structure for OC20...\n" );

	// Collect all the all the files under temporary folder to a list
	iterate( g_build_filename( cwd, pckg_tmp_dir, NULL ),(cb)add_file_to_list , NULL, on_iterate_error, &temp_files );

	if ( DEBUG || debug )printf( "List contains %i records\n", g_slist_length( temp_files ) );

	g_slist_foreach( temp_files, make_oc20_cb, NULL );
	g_slist_free_full( temp_files, g_free );

	return 0;
}

/**
 * Adds files to a list
 */
int add_file_to_list( char *file, void **list ) { 
	*((GSList**)list) = g_slist_append( *list, g_strdup( file ) );

	return 0;
}

/**
 * iterator callback to process single file
 */
void make_oc20_cb( void *name, void *data ) {
	int debug = 0;

	char *p;
	char new_name[ path_max_size ];
	char *dir;
	char *dir_to_remove;
	int delete = 0;

	if ( DEBUG || debug )printf( "Current file to process: '%s'\n", (char*)name );

	if ( NULL != ( p = strstr( name, "/controller/extension/" ) ) ) {
		memset( new_name, '\0', path_max_size );
		strncpy( new_name, name, p - (char*)name  + 12 ); // from the start up to leading slash
		strcat( new_name, p + 22 );

		if ( DEBUG || debug ) fprintf( stderr, "Changing '%s' => '%s'\n", (char*)name, new_name );

		dir = g_path_get_dirname( new_name );
		Mkdir( dir, 0777 );
		g_free( dir );

		if ( -1 == rename( name, new_name ) ) {
			fprintf( stderr, "make_oc20: failed to rename '%s' to '%s': %s\n", (char*)name, new_name, strerror( errno ) );
			exit( 1 );
		}

		content_to_oc20( new_name );
		php_lint_cb( new_name, NULL );
		delete = 1;

	} else if ( NULL != ( p = strstr( name, "/en-gb/" ) ) ) {
		memset( new_name, '\0', path_max_size );
		strncpy( new_name, name, p - (char*)name ); // from the start up to leading slash
		strcat( new_name, "/english" );

		if ( NULL != strstr( name, "/en-gb/extension/" ) ) {
			strcat( new_name, p + 16 );

		} else {
			strcat( new_name, p + 6 );
		}

		if ( DEBUG || debug ) fprintf( stderr, "Changing '%s' => '%s'\n", (char*)name, new_name );

		dir = g_path_get_dirname( new_name );
		Mkdir( dir, 0777 );
		g_free( dir );
		
		if ( -1 == rename( name, new_name ) ) {
			fprintf( stderr, "make_oc20: failed to rename '%s' to '%s': %s\n", (char*)name, new_name, strerror( errno ) );
			exit( 1 );
		}

		delete = 1;

	} else if ( NULL != ( p = strstr( name, "/model/extension/" ) ) ) {
		memset( new_name, '\0', path_max_size );
		strncpy( new_name, name, p - (char*)name  + 7 ); // from the start up to leading slash
		strcat( new_name, p + 17 );

		if ( DEBUG || debug ) fprintf( stderr, "Changing '%s' => '%s'\n", (char*)name, new_name );

		dir = g_path_get_dirname( new_name );
		Mkdir( dir, 0777 );
		g_free( dir );
		
		if ( -1 == rename( name, new_name ) ) {
			fprintf( stderr, "make_oc20: failed to rename '%s' to '%s': %s\n", (char*)name, new_name, strerror( errno ) );
			exit( 1 );
		}

		content_to_oc20( new_name );
		php_lint_cb( new_name, NULL );
		delete = 1;

	} else if ( NULL != ( p = strstr( name, "/template/extension/" ) ) ) {
		memset( new_name, '\0', path_max_size );
		strncpy( new_name, name, p - (char*)name  + 10 ); // from the start up to leading slash
		strcat( new_name, p + 20 );

		if ( DEBUG || debug ) fprintf( stderr, "Changing '%s' => '%s'\n", (char*)name, new_name );

		dir = g_path_get_dirname( new_name );
		Mkdir( dir, 0777 );
		g_free( dir );

		if ( -1 == rename( name, new_name ) ) {
			fprintf( stderr, "make_oc20: failed to rename '%s' to '%s': %s\n", (char*)name, new_name, strerror( errno ) );
			exit( 1 );
		}

		delete = 1;
	}

	if ( delete ) {
		dir_to_remove = g_path_get_dirname( name );

		if ( DEBUG || debug )printf( "Removing directory '%s'\n", dir_to_remove );

		if ( -1 == Rmdir_upward( dir_to_remove, 4 ) ) {
			fprintf( stderr, "Failed to remove directory '%s': %s in %s\n", dir_to_remove, strerror( errno ), G_STRLOC );
			exit( 1 );
		}

		g_free( dir_to_remove );
	}
}

/**
 * Directory callback for delete empty folders iterator
 */
int del_empty_dirs_cb( char *path, void *data ) {
		if ( rmdir( path ) && ENOTEMPTY != errno ) {
		fprintf( stderr, "del_empty_dirs_cb: path: '%s', error: %s\n", path, strerror( errno ) );
		return 1;
	}

	return 0;
}

/**
 * Changes classes names for controller and model to be OC20 compliant
 */
int content_to_oc20( char *name ) {
	int debug = 0;

	int f;
	char buff[ MAX_LINE ];
	char new_line[ MAX_LINE ];
	size_t len;
	int match_start;
	int match_end;
	char *matched_string;

	GRegex *regex = g_regex_new( "class\\s+\\w+?(extension)", G_REGEX_CASELESS, 0, NULL );
	GMatchInfo *match_info;

	if ( -1 == ( f = open( name, O_RDWR ) ) ) {
		fprintf( stderr, "content_to_oc20: failed to open file '%s': %s", name, strerror( errno ) );
		exit( 1 );
	}

	if ( DEBUG || debug ) fprintf( stderr,  "File '%s' opened for modifications\n", name );

	memset( new_line, 0, MAX_LINE );

	while (  File_get_line( f, buff, MAX_LINE ) > 0 ) {
		if ( DEBUG || debug ) fprintf( stderr,  "'%s'\n", buff );
		if ( DEBUG || debug ) fprintf( stderr,  "Current offset: %ld\n", lseek( f, 0, SEEK_CUR ) );

		if ( g_regex_match( regex, buff, 0, &match_info ) ) {
			if ( DEBUG || debug ) {
				fprintf( stderr, "Matched string \n" );
				dump_string( buff );
			}

			g_match_info_fetch_pos( match_info, 1, &match_start, &match_end );
			len = strlen( buff );

			strncpy( new_line, buff, match_start ); // Copy everything up to "extension"
			strcat( new_line, &buff[ match_end ] ); // After "extension"

			for( int i = strlen( new_line ) - 1; i < len - 1; i++ ) {
				new_line[ i ] = ' ';
			}

			new_line[ len - 1 ] = '\n';
 
			if ( DEBUG || debug ) {
				fprintf( stderr, "Result to be saved:\n" );
				dump_string( new_line );
				printf( "Match start: %i, match end: %i, buffer length: %li\n", match_start, match_end, len );
			}

			// Get 1 line back in file stream
			if( -1 == lseek( f, -1 * len, SEEK_CUR ) ) {
				fprintf( stderr, "content_to_oc20: failed to set new position on stream: %s\n", strerror( errno ) );
				exit( 1 );
			}

			if ( DEBUG || debug ) fprintf( stderr,  "Stream position after: %ld\n", lseek( f, 0, SEEK_CUR ) );

			if ( -1 == write( f, new_line, len ) ) {
				fprintf( stderr, "content_to_oc20: failed to write back string to a file '%s': %s\n", name, strerror( errno ) );
				exit( 1 );
			}

			if ( DEBUG || debug ) fprintf( stderr,  "File '%s' was modified\n", name );

			g_match_info_unref( match_info );

			break;
		}
	}

	close( f );
	g_regex_unref( regex );

	return 0;
}

/**
 * Adds versions numbers to package files
 */
int add_version( FILE *f ) {
	int debug = 0;

	char buff[ MAX_LINE ];
	char prev[ MAX_LINE ];
	char new_buff[ MAX_LINE ];
	char *matched_string;
	char *config_version;
	struct llist *matches;
	size_t len, matched_len, buff_len, new_len;
	int c = 0;
	char *p;
	int i;
	int match_start;
	int match_end;

	GRegex *regex = g_regex_new( "@version \\s ( \\s* [0-9]+ \\. [0-9]+ \\. [0-9]+ \\s* ) $", G_REGEX_EXTENDED, 0, NULL );
	GMatchInfo *match_info;

	GSList *list_major, *list_minor, *list_patch;

	int major, minor, patch;

	list_major = g_hash_table_lookup( config, "major" );
	list_minor = g_hash_table_lookup( config, "minor" );
	list_patch = g_hash_table_lookup( config, "patch" );
	g_return_val_if_fail( NULL != list_major, 1 );
	g_return_val_if_fail( NULL != list_minor, 1 );
	g_return_val_if_fail( NULL != list_patch, 1 );

	major = atoi( list_major->data );
	minor = atoi( list_minor->data );
	patch = atoi( list_patch->data );

	config_version = g_strdup_printf( "%i.%i.%i", major, minor, patch );

	rewind( f );

	while ( NULL != fgets( buff, MAX_LINE, f ) ) {
		if ( c > 10 ) break;

		if ( g_regex_match( regex, buff, 0, &match_info ) ) {
			if ( DEBUG || debug ) {
				printf( "Prev: '%s'\n", prev );
				fprintf( stderr,  "Current offset: %ld\n", ftello( f ) );
				fprintf( stderr, "Matched string\n" );

				dump_string( buff );

				matched_string = g_match_info_fetch( match_info, 1 );
				fprintf( stderr, "Match: '%s'\n", matched_string );
				g_free( matched_string );
			}

			g_match_info_fetch_pos( match_info, 1, &match_start, &match_end );

			// Length of package current version in characters.
			len = strlen( config_version );

			// Matched version length
			matched_len = match_end - match_start - 1;

			// Buffer length
			buff_len = strlen( buff );

			if ( DEBUG || debug ) fprintf( stderr,  "Package version length (%i.%i.%i) is : %ld\nMatched version length is: %ld\nBuffer length: %ld\nMatch start: %d\nMatch end: %d\n", major, minor, patch, len, matched_len, buff_len, match_start, match_end );

			memset( new_buff, '\0', MAX_LINE );
			strncpy( new_buff, buff, match_start );

			// Add version numbers right after '@version
			// Insufficient space but no less then 5 characters to accommodate 0.0.0
			if ( matched_len < len && matched_len >= 5 ) {
				strcat( new_buff, "0.0.0" );
				len = 5;

			// Space is OK
			} else if ( matched_len >= len ) {
				strcat( &new_buff[ match_start ], config_version );

			} else {
				break;
			}

			if ( debug ) {
				dump_string( new_buff );
			}
			
			new_len = strlen( new_buff );

			if ( DEBUG || debug ) fprintf( stderr, "New buffer length: %ld\n", new_len );

			if ( matched_len > len ) {
				if ( debug ) fprintf( stderr, "Padding right with %ld spaces\n", matched_len - len );
				memset( &new_buff[ new_len ], ' ', matched_len - len );
			}

			strcat( new_buff, "\n" );

			if ( debug ) {
				dump_string( new_buff );

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
	
	g_free( config_version );
}

/**
 * Tries to find XML node
 */
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

/**
 * Translate OCMOD entries to VQMOD
 */
int fix_vqmod_file( xmlDocPtr doc, xmlNodePtr cur ) {
	xmlNodePtr add, search;

	// file path => name
	xmlSetProp( cur, "name", xmlGetProp( cur, "path" ) );
	xmlUnsetProp( cur, "path" );

	add = find_node( cur, "add" );
	search = find_node( cur, "search" );

	if ( NULL == add ) {
		fprintf( stderr, "fix_vqmod_file: file node doesn't contain node 'add' in %s\n", G_STRLOC );
		exit( 1 );
	}

	if ( NULL == search ) {
		fprintf( stderr, "fix_vqmod_file: file node doesn't contain nod 'search' in %s\n", G_STRLOC );
		exit( 1 );
	}

	xmlSetProp( search, "position", xmlGetProp( add, "position" ) );
	xmlUnsetProp( add, "position" );

    return 1;
}

/**
 * Opens OCMOD files and returns XML VQMOD representation
 */
xmlDocPtr parseXMLDoc( char *path ) {
	xmlDocPtr doc;
	xmlNodePtr cur, prev, mod;

	doc = xmlParseFile( path );
	
	if ( doc == NULL ) {
		fprintf( stderr,"XML Document not parsed successfully in %s\n", G_STRLOC );
		return (NULL);
	}
	
	mod = xmlDocGetRootElement( doc );
	
	if (mod == NULL) {
		fprintf(stderr,"parseDoc: empty document in %s\n", G_STRLOC );
		xmlFreeDoc(doc);
		return (NULL);
	}
	
	if (xmlStrcmp(mod->name, (const xmlChar *) "modification")) {
		fprintf(stderr,"parseDoc: document of the wrong type, root node != modification in %s\n", G_STRLOC );
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

/**
 * Makes VQMOD file from existing OCMOD file
 */
int make_vqmod() {
	char *name_ocmod;
	char *name_vqmod;
	char *code;
	char *path_ocmod;
	char *path_vqmod;
	char *keyword;
	xmlDocPtr doc;

	code = get_code();
	name_ocmod = g_strdup_printf( "%s.ocmod.xml", code );
	path_ocmod = g_build_filename( cwd, pckg_tmp_dir, name_ocmod, NULL );
	name_vqmod = g_strdup_printf( "%s.vqmod.xml", code );
	path_vqmod = g_build_filename( cwd, pckg_tmp_dir, name_vqmod, NULL );

	doc = parseXMLDoc( path_ocmod );

	if (doc != NULL) {
		xmlSaveFormatFile( path_vqmod, doc, 0 );
		xmlFreeDoc( doc );
	}

	g_free( code );
	g_free( name_vqmod );
	g_free( path_vqmod );
	g_free( name_ocmod );
	g_free( path_ocmod );
	
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
		value = g_strdup( xmlNodeGetContent( cur ) );

		if ( strlen( g_strstrip( value ) ) ) {

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
 * Initializes hash config to store package configuration data
 */
void init_config_hash() {
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

	if ( DEBUG )printf( "Initializing configuration... \n" );

	if ( NULL != config ) {
		if ( DEBUG )printf( "Destroying hash table... \n" );
		g_hash_table_destroy( config );
	}

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
 * Get configuration data from config file and put it into inner storage plus fill in corresponding inputs 
 * Change combobox event handler
 */
void fill_in_config( GtkComboBox *widget, gpointer user_data ) {
	if ( DEBUG )print_color( B_CYAN, ">>>>>> PACKAGE SELECTED (fill_in_config) <<<<<<\n" );

	init_config_hash();

	if ( DEBUG )printf( "Fetching config data\n" );

	char *name = g_malloc0( path_max_size );

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

	g_slist_foreach( include_file, fill_in_input_buffer, buffer_include_file );

	// Excluded files
	if ( DEBUG ) {
		printf( "Set excluded files\n" );
		dump_slist( exclude_file );
	}

	g_slist_foreach( exclude_file, fill_in_input_buffer, buffer_exclude_file );

	// Included folders
	if ( DEBUG ) {
		printf( "Set included folders\n" );
		dump_slist( include_folder );
	}

	g_slist_foreach( include_folder, fill_in_input_buffer, buffer_include_folder );

	// Excluded folders
	if ( DEBUG ) {
		printf( "Set excluded folders\n" );
		dump_slist( exclude_folder );
	}

	g_slist_foreach( exclude_folder, fill_in_input_buffer, buffer_exclude_folder );

	// Included regex
	if ( DEBUG ) {
		printf( "Set included regex\n" );
		dump_slist( include_regex );
	}

	g_slist_foreach( include_regex, fill_in_input_buffer, buffer_include_regex );

	if ( DEBUG ) {
		printf( "Set excluded regex\n" );
		dump_slist( exclude_regex );
	}

	g_slist_foreach( exclude_regex, fill_in_input_buffer, buffer_exclude_regex );

	if ( DEBUG )printf( "View has been updated\n" ); 
}

/**
 * Fill ins text buffer with specific text
 */
void fill_in_input_buffer( void *text, void *buffer ) {
	if ( DEBUG )print_color( B_GREEN, "fill_in_input_buffer: Stat\n" );

	g_return_if_fail( buffer != NULL );
	GtkTextIter start;
	GtkTextIter end;
	char *new_text;

	// Empty list
	if ( text == NULL ) return;

	if ( DEBUG )printf( "Adding text '%s' to text buffer\n", (char*)text );

	gtk_text_buffer_get_start_iter( buffer, &start );
	gtk_text_buffer_get_end_iter( buffer, &end );

	// Not first line - prepend newline character
	if ( !gtk_text_iter_equal( &start, &end ) ) {
		new_text = g_malloc0( strlen( text ) + 2 );
		memcpy( new_text, "\n", 1 );
		strncat( new_text, text, strlen( text ) );
		gtk_text_buffer_insert( buffer, &end, new_text, -1 );
		g_free( new_text );

	} else {
		gtk_text_buffer_insert( buffer, &end, text, -1 );
	}
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

	GtkTextIter start;
	GtkTextIter end;
	int i = 0;

	while ( list[ i ] != NULL ) {
		gtk_text_buffer_get_start_iter( list[ i ], &start );
		gtk_text_buffer_get_end_iter( list[ i ], &end );
		gtk_text_buffer_delete ( list[ i ], &start, &end );
		i++;
	}

	// Clear code input
	gtk_entry_set_text( input_code, "" );

	// Clean version numbers
	gtk_spin_button_set_value( input_major, 0 );
	gtk_spin_button_set_value( input_minor, 0 );
	gtk_spin_button_set_value( input_patch, 0 );
}

/** 
 * Reloads list of configuration files from the disk
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

	if ( DEBUG )printf( "Adding code value '%s'...\n", (char*)code->data );
	child = xmlNewNode( NULL, "code" );
	xmlNodeAddContent( child, code->data );
	xmlAddChild( root, child );

	if ( DEBUG )printf( "Adding major number value '%s'...\n", (char*)major->data );
	child = xmlNewNode( NULL, "major" );
	xmlNodeAddContent( child, major->data );
	xmlAddChild( root, child );

	if ( DEBUG )printf( "Adding minor number value '%s'...\n", (char*)minor->data );
	child = xmlNewNode( NULL, "minor" );
	xmlNodeAddContent( child, minor->data );
	xmlAddChild( root, child );

	if ( DEBUG )printf( "Adding patch number value '%s'...\n", (char*)patch->data );
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
	if ( DEBUG )printf( "Getting package name\n" );

	g_return_val_if_fail( NULL != input_code, NULL );
	g_return_val_if_fail( NULL != select_package, NULL );

	char *name, *hidden_name;

	name = gtk_combo_box_text_get_active_text( select_package );

	if ( IS_EMPTY( name ) ) {
		name = g_strdup( gtk_entry_get_text( input_code ) );

		if ( IS_EMPTY( name ) ) {
			if( DEBUG )g_print( "Package name is empty\n" );
			return NULL;
		}

		hidden_name = Strcat( ".", name, ".package", NULL );
		g_free( name );
		name = hidden_name;
	}

	if ( DEBUG )printf("Package name is: '%s'\n", name );

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
 * Delete configuration file button click handler
 */
void delete_config( GtkButton *button, gpointer data ) {
	if ( DEBUG )print_color( B_CYAN, ">>>>>>>> DELETE CONFIG <<<<<<<<\n");
	gtk_widget_show( GTK_WIDGET( delete_package_confirm ) );
	
}

/**
 * Delete configuration dialog OK button click handler
 */
void _delete_config( GtkButton *button, gpointer data ) {
	if ( DEBUG )print_color( B_CYAN, ">>>>>>>> DELETE CONFIG CONFIRM <<<<<<<<\n");
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
 * Delete configuration dialog CANCEL button click handler
 */
void delete_config_hide( GtkButton *button, gpointer data ) {
	if ( DEBUG )print_color( B_CYAN, ">>>>>>>> DELETE CONFIG HIDE <<<<<<<<\n");
	gtk_widget_hide( GTK_WIDGET( delete_package_confirm ) );
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

	init_config_hash();

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
	g_hash_table_insert( config, g_strdup( "include_folder" ), text_buffer_to_slist( buffer_include_folder, new_include_folder ) );

	// Update package included files
	g_hash_table_insert( config, g_strdup( "exclude_folder" ), text_buffer_to_slist( buffer_exclude_folder, new_exclude_folder ) );

	// Update package included files
	g_hash_table_insert( config, g_strdup( "include_regex" ), text_buffer_to_slist( buffer_include_regex, new_include_regex ) );

	// Update package included files
	g_hash_table_insert( config, g_strdup( "exclude_regex" ), text_buffer_to_slist( buffer_exclude_regex, new_exclude_regex ) );
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
	char *line;

	gtk_text_buffer_get_start_iter( buffer, &start );
	gtk_text_buffer_get_end_iter( buffer, &end );

	text = gtk_text_buffer_get_text( buffer, &start, &end, FALSE );

	if ( DEBUG )g_print( "Value: '%s'\n", text );

	parts = g_strsplit( text, "\n", -1 );

	if ( DEBUG )dump_vector( parts );

	p = parts;

	while( *p != NULL ) {
		line = g_strstrip( *p );

		if ( IS_EMPTY( line ) ) {
			if( DEBUG )printf( "Buffer row is empty. Skip\n" );
			p++;

			continue;
		}

		list = g_slist_append( list, g_strdup( line ) );
		p++;
	}

	g_free( text );
	g_strfreev( parts );

	return list;
}

void print_error( char *error ) {
	fprintf( stderr, "%s\n", error );
}

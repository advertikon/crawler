#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <regex.h>
#include <time.h>
#include <sys/times.h>
#include <termios.h>
#ifndef TIOCGWINSZ
#include <sys/ioctl.h>
#endif
#include <sys/wait.h>
#include <fcntl.h>
#include <math.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <assert.h>
#include <gtk/gtk.h>

#include "error.h"
#include "args.h"
#include "path.h"
#include "structure.h"

typedef void Sigfunc( int );
typedef int It_file(  char*, struct stat* );
typedef int It_dir(  char*, struct stat* );
typedef int It_error(  char* );

int usage( void );
int iterate(  char*, It_file*, It_dir*, It_error* );
int parse_config( char* );
int add_to(  char* );
int del_from(  char*, struct llist* );
int start_add( struct llist* );
int start_del( struct llist* );
int show_commands( void );
int add_to_config( struct llist* );
int remove_from_config( struct llist* );
int abort_config( void );
int print_del_list( struct llist* );
int confirmed_operation( void );
int end_operation( void );
int write_config_section( char*, FILE*, struct llist* );
void int_handler( int );
int check_file(  char* );
int collide_length(  char*, struct llist*, int* );
int collide_span(  char*,  char* );
char* get_regerror( int, regex_t* );
int match(  char*,  char*, regmatch_t*, int );
int check_regexp(  char*, struct llist* );
void start_clock( void );
void end_clock( char* );
int print_config( void );
int print_files( void );
static void set_winsize( void );
static void sig_winch( int );
int load_dependencies( void );
char *ltrim( char*,  char* );
char *trim( char*,  char* );
char *rtrim( char*,  char* );
int is_file(  char* );
int is_dir(  char* );
int check_item( char*, struct stat* );
int check_source( char*, struct stat* );
int on_iterate_error( char* );
char* add_cwd( char* );
int make_package( void );
int run_zip( char* );
void sig_cld( int );
int make_dir( char*, mode_t );
int fill_temp_package();
int make_file( char*, mode_t );
char *file_name( char* );
char *dir_name( char* );
int fill_translation( FILE*, char* );
int run_filters( void );
int fetch_translation( FILE*, struct llist* );
struct llist* get_matches( const char* );
int init_filters( void );
int get_version( void );
char *get_package_dir( void );
int del_file_cb( char*, struct stat* );
int del_dir_cb( char*, struct stat* );
int save_translation( char*, struct llist* );
char *get_common_dir( void );
int make_oc20( void );
int del_empty_dirs_cb( char*, struct stat* );
int content_to_oc20( char* );
int add_version( FILE * );
int make_vqmod();
int fix_vqmod_file( xmlDocPtr, xmlNodePtr );
int set_config_name( void );
int xml_to_config( char*, xmlNodePtr );
xmlNodePtr config_to_xml( char*, GSList* );
int php_lint();
int php_lint_cb( char*, struct stat* );

void destroy( GtkWidget *, gpointer );
void show_error( char * );
int get_package_configs( void );
int filter_package_config_name( const struct dirent* );
void fill_in_config( GtkComboBox *widget, gpointer user_data );
void update_config_view();
void fill_in_input_buffer( void*, void* );
void destroy_list( gpointer );
void clear_config_buffers( void );
// void init_filter_names( void );
void config_value_clean( gpointer );
void config_key_clean( gpointer );
void reload_config( GtkButton *button, gpointer );
void save_config( GtkButton *button, gpointer );
void delete_config( GtkButton *button, gpointer );
void update_config_from_view( void );


#define MAX_LINE 400
#define DEBUG 1
#define REGEX_MATCH_COUNT 10
#define VERSION_SIZE 10
#define BUFF_SIZE 4096

#define IS_EMPTY( p ) ( NULL == p || '\0' == *p )
#define N_LEN( n ) ( ( ( 0 == n ) ? 1 : ( ( 0 == ( n % 10 ) ) ? ceil( log10( n ) ) + 1 : ceil( log10( n ) ) ) ) )

enum COMMANDS {
	C_ADD_INCL_FOLDER = 1,
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
	C_SET_NAME,
	C_ITERATE,
	C_PRINT_FILES,
	C_PRINT_CONFIG,
	C_MAKE,
	C_SET_MAJOR,
	C_SET_MINOR,
	C_SET_PATCH
};

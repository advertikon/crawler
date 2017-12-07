#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
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

#include "path.h"

typedef int(*callback)();

int parse_config( char* );
int abort_config( void );
void int_handler( int );
gboolean check_file(  char* );
void collide_length(  char*, GSList*, int* );
gboolean check_regexp(  char*, GSList* );
void start_clock( void );
void end_clock( char* );
static void set_winsize( void );
static void sig_winch( int );
int load_dependencies( void );
int check_item( char*, void* );
int check_source( char*, void* );
int on_iterate_error( char*, void* );
int make_package( void );
int run_zip( char* );
void sig_cld( int );
int fill_temp_package();
int make_file( char*, mode_t );
int fill_translation( FILE*, char* );
int run_filters( void );
int fetch_translation( FILE*, GSList**, GSList**, gboolean );
int init_filters( void );
int get_version( int*, int*, int* );
char *get_package_dir( void );
int del_file_cb( char*, void* );
int del_dir_cb( char*, void* );
int save_translation( char*, GSList* );
char *get_common_dir( void );
int make_oc20( void );
int del_empty_dirs_cb( char*, void* );
int content_to_oc20( char* );
int add_version( FILE * );
int make_vqmod();
int fix_vqmod_file( xmlDocPtr, xmlNodePtr );
int xml_to_config( char*, xmlNodePtr );
xmlNodePtr config_to_xml( char*, GSList* );
int php_lint();
int php_lint_cb( char*, void* );
void destroy( GtkWidget *, gpointer );
void show_error( char * );
int get_package_configs( void );
int filter_package_config_name( const struct dirent* );
void fill_in_config( GtkComboBox *widget, gpointer user_data );
void update_config_view();
void fill_in_input_buffer( void*, void* );
void destroy_list( gpointer );
void clear_config_buffers( void );
void config_value_clean( gpointer );
void config_key_clean( gpointer );
void reload_config( GtkButton *button, gpointer );
void save_config( GtkButton *button, gpointer );
void delete_config( GtkButton *button, gpointer );
void update_config_from_view( void );
GSList *text_buffer_to_slist( GtkTextBuffer*, GSList* );
char *get_package_name( void );
void delete_config_dialog_show( GtkButton *, gpointer );
void delete_config_dialog_hide( GtkButton *, gpointer );
void get_files( void*, void* );
char *get_code( void );
int run_filters_cb( char*, void* );
void _delete_config( GtkButton*, gpointer );
void delete_config_hide( GtkButton*, gpointer );
void init_config_hash( void );
void print_error( char* );
void files_to_view( void );
xmlDocPtr parseXMLDoc( char* );
void make_oc20_cb( void*, void* );
int add_file_to_list( char*, void** );
void init_translation_lists( void );
int save_all_the_translations( void );


#define MAX_LINE 400
#define DEBUG 0
#define REGEX_MATCH_COUNT 10
#define VERSION_SIZE 10
#define BUFF_SIZE 4096

#define N_LEN( n ) ( ( ( 0 == n ) ? 1 : ( ( 0 == ( n % 10 ) ) ? ceil( log10( n ) ) + 1 : ceil( log10( n ) ) ) ) )

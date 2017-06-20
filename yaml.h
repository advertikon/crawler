int yaml( char*, LL* );
static int MAX = 100;

int yaml( char* name, LL* structure ) {
	FILE* config;
	char line[ MAX ];

	if ( NULL == ( config = fopen( name, "r" ) ) ) {
		print_error( "Failed to open configuration file %s\n", name );
	}

	while ( NULL != fgets( line, MAX, config  ) ) {
		printf( "%s\n", line );
	}
}
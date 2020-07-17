//
// Created by jonathan.hanks on 7/15/20.
//

#include <stdio.h>
#include "local_dc_utils.h"

int
test_extract_dcu_rate_from_name( const char* input,
                                 int         expected_dcu,
                                 int         expected_rate )
{
    int dcu = 0;
    int rate = 0;
    extract_dcu_rate_from_name( input, &dcu, &rate );
    if ( dcu != expected_dcu || rate != expected_rate )
    {
        fprintf( stderr,
                 "Given input '%s' expected %d %d, got %d %d\n",
                 input,
                 expected_dcu,
                 expected_rate,
                 dcu,
                 rate );
        return 1;
    }
    return 0;
}

int
test_extract_dcu_rate_from_name_bad_params( )
{
    int dcu = 52;
    int rate = 256;

    extract_dcu_rate_from_name( NULL, &dcu, &rate );
    if ( dcu != 0 || rate != 0 )
    {
        fprintf( stderr, "dcu or rate was not cleared on bad input" );
        return 1;
    }
    dcu = 52;
    rate = 256;
    extract_dcu_rate_from_name( "", &dcu, &rate );
    if ( dcu != 0 || rate != 0 )
    {
        fprintf( stderr, "dcu or rate not cleared on short input" );
        return 1;
    }
    dcu = 52;
    rate = 256;
    extract_dcu_rate_from_name( ":42:128", NULL, &rate );
    if ( dcu != 52 || rate != 128 )
    {
        fprintf( stderr, "error when dcu dest was NULL" );
        return 1;
    }
    dcu = 52;
    rate = 256;
    extract_dcu_rate_from_name( ":42:128", &dcu, NULL );
    if ( dcu != 42 || rate != 256 )
    {
        fprintf( stderr, "error when rate dest was NULL" );
        return 1;
    }
    dcu = 52;
    rate = 256;
    extract_dcu_rate_from_name( ":42:128", NULL, NULL );
    if ( dcu != 52 || rate != 256 )
    {
        fprintf( stderr, "error when dcu and rate dest were NULL" );
        return 1;
    }
    return 0;
}

void
do_extract_tests( )
{
    if ( test_extract_dcu_rate_from_name( "model", 0, 0 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( "model:52", 52, 16 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( ":52", 52, 16 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( ":52:100", 52, 100 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( "model:52:100", 52, 100 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( ":blah:100", 0, 100 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( "model:blah:blah", 0, 0 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( ":blah:blah", 0, 0 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( ":42:blah", 42, 0 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( ":42a:blah", 42, 0 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( ":42a:128b", 42, 128 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( ":", 0, 16 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( "::", 0, 16 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name_bad_params( ) != 0 )
    {
        exit( 1 );
    }
}

typedef struct Trim_tests
{
    const char* input;
    const char* output;
} Trim_tests;

void
do_trim_tests( )
{
#define TRIM_BUFFER_SIZE 100
    int    i = 0;
    size_t input_len = 0;

    char       buffer[ TRIM_BUFFER_SIZE ];
    Trim_tests tests[] = {
        { "model", "model" },     { "model:dcu:rate", "model" },
        { "model:dcu", "model" }, { "", "" },
        { NULL, NULL },
    };
    for ( i = 0; tests[ i ].input != NULL; ++i )
    {
        input_len = strlen( tests[ i ].input );

        if ( input_len + 1 >= TRIM_BUFFER_SIZE )
        {
            fprintf( stderr,
                     "Invalid test, the destination buffer is too small to run "
                     "the test" );
            exit( 1 );
        }

        strncpy( buffer, tests[ i ].input, TRIM_BUFFER_SIZE );

        trim_dcuid_and_rate_from_name( buffer );

        if ( strcmp( buffer, tests[ i ].output ) != 0 )
        {
            fprintf( stderr,
                     "Trim failure, input of '%s' with expected output of "
                     "'%s', got '%s' instead.\n",
                     tests[ i ].input,
                     tests[ i ].output,
                     buffer );
            exit( 1 );
        }
    }
    /* should be safe to call with a NULL pointer */
    trim_dcuid_and_rate_from_name( NULL );
}

int
main( int argc, char* argv[] )
{
    do_extract_tests( );
    do_trim_tests( );
    return 0;
}
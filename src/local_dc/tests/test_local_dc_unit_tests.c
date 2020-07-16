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
    if ( dcu != 52 || rate != 256 )
    {
        fprintf( stderr, "dcu or rate changed unexpectedly on bad input" );
        return 1;
    }
    extract_dcu_rate_from_name( "", &dcu, &rate );
    if ( dcu != 52 || rate != 256 )
    {
        fprintf( stderr, "dcu or rate changed unexpectedly on short input" );
        return 1;
    }
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

int
main( int argc, char* argv[] )
{
    if ( test_extract_dcu_rate_from_name( ":52", 52, 16 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( ":52:100", 52, 100 ) != 0 )
    {
        exit( 1 );
    }
    if ( test_extract_dcu_rate_from_name( ":blah:100", 0, 100 ) != 0 )
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
    return 0;
}
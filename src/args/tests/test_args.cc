//
// Created by jonathan.hanks on 1/17/20.
//
#include "catch.hpp"
#include "args.h"
#include "args_internal.hh"

#include <stdio.h>
#include <cstring>
#include <algorithm>
#include <vector>

namespace
{
    class file_closer
    {
    public:
        explicit file_closer( FILE* f ) : f_{ f }
        {
        }
        ~file_closer( )
        {
            if ( f_ )
            {
                fclose( f_ );
            }
        }

    private:
        FILE* f_;
    };
} // namespace

TEST_CASE( "You can create an argument parser" )
{
    args_handle args = args_create_parser( "a test program" );
    REQUIRE( args != nullptr );
    args_destroy( &args );
    REQUIRE( args == nullptr );
}

TEST_CASE( "You cannot duplicate argument names in an argument parser" )
{
    int         flag_val = 0;
    int         int_val = 0;
    const char* string_val = "";

    args_handle args = args_create_parser( "a test program" );
    REQUIRE( args_add_flag( args, 'f', "flag1", "a flag", &flag_val ) );
    REQUIRE( !args_add_flag( args, 'f', "flag_other", "a flag", &flag_val ) );
    REQUIRE( !args_add_flag( args, 'F', "flag1", "a flag", &flag_val ) );

    REQUIRE(
        !args_add_int( args, 'f', "flag_other", "n", "a flag", &int_val, 5 ) );
    REQUIRE( !args_add_int(
        args, ARGS_NO_SHORT, "flag1", "n", "a flag", &int_val, 5 ) );

    REQUIRE( !args_add_string_ptr(
        args, 'f', ARGS_NO_LONG, "n", "a flag", &string_val, "n" ) );
    REQUIRE( !args_add_string_ptr(
        args, ARGS_NO_SHORT, "flag1", "n", "a flag", &string_val, "n" ) );
    args_destroy( &args );
}

TEST_CASE( "An arg parser can output its own help" )
{
    int         flag1;
    int         int1;
    const char* string1;
    const char* string2;

    args_handle args = args_create_parser( "a test program" );
    args_add_flag( args, 'f', "flag1", "A flag variable", &flag1 );
    args_add_int(
        args, 'i', "integer", "COUNTS", "An integer field", &int1, 0 );
    args_add_string_ptr(
        args, 's', "string", "NAME", "A file name", &string1, "none" );
    args_add_string_ptr( args,
                         'S',
                         ARGS_NO_LONG,
                         "NAME",
                         "Another file name",
                         &string2,
                         "none" );

    std::vector< char > buf;
    buf.resize( 20000 );
    std::fill( buf.begin( ), buf.end( ), 0 );

    FILE*       f = fmemopen( buf.data( ), buf.size( ) - 1, "wb" );
    file_closer f_( f );
    args_fprint_usage( args, "test_prog", f );
    REQUIRE( std::strstr( buf.data( ), "test_prog" ) != nullptr );
    REQUIRE( std::strstr( buf.data( ), "a test program" ) != nullptr );
    REQUIRE( std::strstr( buf.data( ), "--flag1" ) != nullptr );
    REQUIRE( std::strstr( buf.data( ), "-f" ) != nullptr );
    REQUIRE( std::strstr( buf.data( ), "A flag variable" ) != nullptr );

    args_destroy( &args );
}

TEST_CASE(
    "You can generate a short options string from a sequence of arguments" )
{
    std::vector< args_detail::argument > args;

    args.emplace_back( ARGS_NO_SHORT,
                       "long",
                       "N",
                       "a thing",
                       args_detail::argument_class::FLAG_ARG,
                       []( ) {},
                       []( const char* ) {} );
    args.emplace_back( 'a',
                       "long2",
                       "N",
                       "a thing",
                       args_detail::argument_class::FLAG_ARG,
                       []( ) {},
                       []( const char* ) {} );
    args.emplace_back( 'b',
                       "long3",
                       "N",
                       "a thing",
                       args_detail::argument_class::INT_ARG,
                       []( ) {},
                       []( const char* ) {} );
    args.emplace_back( 'c',
                       "long4",
                       "N",
                       "a thing",
                       args_detail::argument_class::STRING_PTR_ARG,
                       []( ) {},
                       []( const char* ) {} );
    auto short_opts = args_detail::generate_short_options( args );
    REQUIRE( short_opts == "ab:c:" );
}

TEST_CASE( "You can generate a set of ::options from a sequence of arguments" )
{
    std::vector< args_detail::argument > args;

    args.emplace_back( ARGS_NO_SHORT,
                       "long",
                       "N",
                       "a thing",
                       args_detail::argument_class::FLAG_ARG,
                       []( ) {},
                       []( const char* ) {} );
    args.emplace_back( 'a',
                       "long2",
                       "N",
                       "a thing",
                       args_detail::argument_class::FLAG_ARG,
                       []( ) {},
                       []( const char* ) {} );
    args.emplace_back( 'b',
                       "",
                       "N",
                       "a thing",
                       args_detail::argument_class::INT_ARG,
                       []( ) {},
                       []( const char* ) {} );
    args.emplace_back( 'c',
                       "long4",
                       "N",
                       "a thing",
                       args_detail::argument_class::STRING_PTR_ARG,
                       []( ) {},
                       []( const char* ) {} );
    auto long_opts = args_detail::generate_long_options( args );
    for ( const auto& lopt : long_opts )
    {
        REQUIRE( lopt.flag == nullptr );
    }
    REQUIRE( strcmp( long_opts[ 0 ].name, "long" ) == 0 );
    REQUIRE( strcmp( long_opts[ 1 ].name, "long2" ) == 0 );
    REQUIRE( strcmp( long_opts[ 2 ].name, "long4" ) == 0 );

    REQUIRE( long_opts[ 0 ].has_arg == no_argument );
    REQUIRE( long_opts[ 1 ].has_arg == no_argument );
    REQUIRE( long_opts[ 2 ].has_arg == required_argument );

    REQUIRE( long_opts[ 0 ].val == args_detail::ARG_INDEX_BASE + 0 );
    REQUIRE( long_opts[ 1 ].val == args_detail::ARG_INDEX_BASE + 1 );
    REQUIRE( long_opts[ 2 ].val == args_detail::ARG_INDEX_BASE + 3 );

    REQUIRE( long_opts.size( ) == 4 );
    REQUIRE( long_opts.back( ).name == nullptr );
    REQUIRE( long_opts.back( ).has_arg == no_argument );
    REQUIRE( long_opts.back( ).flag == nullptr );
    REQUIRE( long_opts.back( ).val == 0 );
}

TEST_CASE( "Given an getopt_long return value you can look up the entry in the "
           "argument list" )
{
    std::vector< args_detail::argument > args;

    args.emplace_back( ARGS_NO_SHORT,
                       "long",
                       "N",
                       "a thing",
                       args_detail::argument_class::FLAG_ARG,
                       []( ) {},
                       []( const char* ) {} );
    args.emplace_back( 'a',
                       "long2",
                       "N",
                       "a thing",
                       args_detail::argument_class::FLAG_ARG,
                       []( ) {},
                       []( const char* ) {} );
    args.emplace_back( 'b',
                       "",
                       "N",
                       "a thing",
                       args_detail::argument_class::INT_ARG,
                       []( ) {},
                       []( const char* ) {} );
    args.emplace_back( 'c',
                       "long4",
                       "N",
                       "a thing",
                       args_detail::argument_class::STRING_PTR_ARG,
                       []( ) {},
                       []( const char* ) {} );

    REQUIRE( args_detail::find_arg( args, static_cast< int >( 'd' ) ) ==
             args.end( ) );
    REQUIRE( args_detail::find_arg( args, static_cast< int >( 0 ) ) ==
             args.end( ) );
    REQUIRE( args_detail::find_arg( args, static_cast< int >( -1 ) ) ==
             args.end( ) );
    REQUIRE( args_detail::find_arg( args, 999 ) == args.end( ) );
    REQUIRE( args_detail::find_arg( args, 1000 ) == args.begin( ) );
    REQUIRE( args_detail::find_arg( args, args_detail::ARG_INDEX_BASE + 4 ) ==
             args.end( ) );
}

TEST_CASE( "You can parse arguments" )
{
    int         flag1 = 0;
    int         int1 = 0;
    const char* string1 = "";
    const char* string2 = "";

    args_handle args = args_create_parser( "a test program" );
    args_add_flag( args, 'f', "flag1", "A flag variable", &flag1 );
    args_add_int(
        args, 'i', "integer", "COUNTS", "An integer field", &int1, 0 );
    args_add_string_ptr(
        args, 's', "string", "NAME", "A file name", &string1, "none" );
    args_add_string_ptr( args,
                         'S',
                         ARGS_NO_LONG,
                         "NAME",
                         "Another file name",
                         &string2,
                         "none" );

    const char* const argv_good[] = {
        "program", "-f", "-i5", "--string", "s1", "-Sabc",
    };

    REQUIRE( flag1 == 0 );
    REQUIRE( int1 == 0 );
    REQUIRE( strcmp( string1, "" ) == 0 );
    REQUIRE( strcmp( string2, "" ) == 0 );
    REQUIRE( args_parse( args, 6, const_cast< char** >( argv_good ) ) );
    REQUIRE( flag1 == 1 );
    REQUIRE( int1 == 5 );
    REQUIRE( string1 != nullptr );
    REQUIRE( string2 != nullptr );
    REQUIRE( strcmp( string1, "s1" ) == 0 );
    REQUIRE( strcmp( string2, "abc" ) == 0 );

    flag1 = 0;
    int1 = 0;
    const char* const argv_bad[] = {
        "program", "-f5", "-i5", "--string", "s1", "-S", "abc",
    };
    REQUIRE( args_parse( args, 7, const_cast< char** >( argv_bad ) ) == 0 );
    REQUIRE( flag1 == 0 );
    REQUIRE( int1 == 0 );

    REQUIRE( args_parse( args, 0, nullptr ) == 1 );

    args_destroy( &args );
}
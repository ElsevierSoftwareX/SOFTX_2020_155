//
// Created by jonathan.hanks on 1/17/20.
//

#include "args.h"

#include <cstring>
#include <cstdlib>

#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include "args_internal.hh"

namespace
{

    // std::make_unique didn't make it into C++11, so
    // to allow this to work in a pre C++14 world, we
    // provide a simple replacement.
    //
    // A make_unique<> for C++11.  Taken from
    // "Effective Modern C++ by Scott Meyers (O'Reilly).
    // Copyright 2015 Scott Meyers, 978-1-491-90399-5"
    //
    // Permission given in the book to reuse code segments.
    //
    // @tparam T The type of the object to be managed by the unique_ptr
    // @tparam Ts The type of the arguments to T's constructor
    // @param params The arguments to forward to the constructor
    // @return a std::unique_ptr<T>
    template < typename T, typename... Ts >
    std::unique_ptr< T >
    make_unique_ptr( Ts&&... params )
    {
        return std::unique_ptr< T >( new T( std::forward< Ts >( params )... ) );
    }

    struct arg_parser
    {
        explicit arg_parser( std::string program_description )
            : description( std::move( program_description ) ), show_help( 0 )
        {
        }

        bool
        should_show_help( ) const
        {
            return show_help > 0;
        }

        std::string                          description;
        std::vector< args_detail::argument > arguments;
        int                                  show_help{ 0 };
    };

    bool
    duplicate_names( const arg_parser& args,
                     const char        short_form,
                     const char*       long_form )
    {
        if ( short_form != ARGS_NO_SHORT )
        {
            auto it = std::find_if(
                args.arguments.begin( ),
                args.arguments.end( ),
                [short_form]( const args_detail::argument& arg ) -> bool {
                    return arg.short_form == short_form;
                } );
            if ( it != args.arguments.end( ) )
            {
                return true;
            }
        }
        if ( long_form != ARGS_NO_LONG )
        {
            auto it = std::find_if(
                args.arguments.begin( ),
                args.arguments.end( ),
                [long_form]( const args_detail::argument& arg ) -> bool {
                    return arg.long_form == long_form;
                } );
            if ( it != args.arguments.end( ) )
            {
                return true;
            }
        }
        return false;
    }

    arg_parser*
    to_parser( args_handle handle )
    {
        return reinterpret_cast< arg_parser* >( handle );
    }

    args_handle
    to_handle( arg_parser* parser )
    {
        return reinterpret_cast< args_handle >( parser );
    }

    std::string
    to_string( const char* s )
    {
        return std::string( ( s ? s : "" ) );
    }

    std::string
    to_string( int val )
    {
        std::stringstream os;
        os << val;
        return os.str( );
    }

    std::string
    to_description( std::string        base_description,
                    const std::string& default_val )
    {
        if ( !default_val.empty( ) )
        {
            if ( !base_description.empty( ) && base_description.back( ) != '.' )
            {
                base_description.append( "." );
            }
            base_description.append( " Default [" );
            base_description.append( default_val );
            base_description.append( "]" );
        }
        return std::move( base_description );
    }
} // namespace

extern "C" {

const char  ARGS_NO_SHORT = 0;
const char* ARGS_NO_LONG = nullptr;

args_handle
args_create_parser( const char* program_description ) try
{
    auto parser = make_unique_ptr< arg_parser >( program_description );
    args_add_flag( to_handle( parser.get( ) ),
                   'h',
                   "help",
                   "Show this help.",
                   &( parser->show_help ) );
    return to_handle( parser.release( ) );
}
catch ( ... )
{
    return to_handle( nullptr );
}

void
args_destroy( args_handle* handle_ ) try
{
    if ( !handle_ )
    {
        return;
    }
    if ( *handle_ )
    {
        std::unique_ptr< arg_parser > parser( to_parser( *handle_ ) );
    }
    *handle_ = nullptr;
}
catch ( ... )
{
}

void
args_fprint_usage( args_handle args, const char* program_name, FILE* out ) try
{
    if ( !out || !args )
    {
        return;
    }
    arg_parser& args_ = *to_parser( args );
    fprintf(
        out, "Usage: %s [options]\n", ( program_name ? program_name : "" ) );
    fprintf( out, "%s\n", args_.description.c_str( ) );
    fprintf( out, "\nOptions are as follows:\n" );
    for ( const auto& arg : args_.arguments )
    {
        bool had_short{ false };

        int chars_printed = 0;
        if ( arg.short_form != ARGS_NO_SHORT )
        {
            chars_printed = fprintf( out, " -%c", arg.short_form );
            had_short = true;
            if ( arg.long_form.empty( ) && !arg.units.empty( ) )
            {
                chars_printed += fprintf( out, " %s", arg.units.c_str( ) );
            }
        }
        else
        {
            chars_printed = fprintf( out, "   " );
        }

        if ( !arg.long_form.empty( ) )
        {
            chars_printed += fprintf( out,
                                      "%s--%s",
                                      ( had_short ? ", " : "  " ),
                                      arg.long_form.c_str( ) );
            if ( !arg.units.empty( ) )
            {
                chars_printed += fprintf( out, "=%s", arg.units.c_str( ) );
            }
        }
        int spaces = 1 + ( chars_printed > 25 ? 0 : 25 - chars_printed );
        for ( int i = 0; i < spaces; ++i )
        {
            fprintf( out, " " );
        }
        fprintf( out, "%s\n", arg.description.c_str( ) );
    }
    fflush( out );
}
catch ( ... )
{
}

int
args_add_flag( args_handle args,
               char        short_name,
               const char* long_name,
               const char* description,
               int*        destination ) try
{
    if ( !args || ( short_name == 0 && !long_name ) || !destination )
    {
        return 0;
    }
    arg_parser& args_ = *to_parser( args );
    if ( duplicate_names( args_, short_name, long_name ) )
    {
        return 0;
    }
    args_.arguments.emplace_back(
        short_name,
        to_string( long_name ),
        "",
        to_string( description ),
        args_detail::argument_class::FLAG_ARG,
        [destination]( ) { *destination = 0; },
        [destination]( const char* opt ) { ++( *destination ); } );
    return 1;
}
catch ( ... )
{
    return 0;
}

int
args_add_string_ptr( args_handle  args,
                     char         short_name,
                     const char*  long_name,
                     const char*  units,
                     const char*  description,
                     const char** destination,
                     const char*  default_value ) try
{
    if ( !args || ( short_name == 0 && !long_name ) || !destination )
    {
        return 0;
    }
    arg_parser& args_ = *to_parser( args );
    if ( duplicate_names( args_, short_name, long_name ) )
    {
        return 0;
    }
    args_.arguments.emplace_back(
        short_name,
        to_string( long_name ),
        to_string( units ),
        to_description( to_string( description ), to_string( default_value ) ),
        args_detail::argument_class::STRING_PTR_ARG,
        [destination, default_value]( ) { *destination = default_value; },
        [destination]( const char* opt ) {
            if ( opt )
            {
                *destination = opt;
            }
        } );
    return 1;
}
catch ( ... )
{
    return 0;
}

int
args_add_int( args_handle args,
              char        short_name,
              const char* long_name,
              const char* units,
              const char* description,
              int*        destination,
              int         default_value ) try
{
    if ( !args || ( short_name == 0 && !long_name ) || !destination )
    {
        return 0;
    }
    arg_parser& args_ = *to_parser( args );
    if ( duplicate_names( args_, short_name, long_name ) )
    {
        return 0;
    }
    args_.arguments.emplace_back(
        short_name,
        to_string( long_name ),
        to_string( units ),
        to_description( to_string( description ), to_string( default_value ) ),
        args_detail::argument_class::INT_ARG,
        [destination, default_value]( ) { *destination = default_value; },
        [destination]( const char* opt ) { *destination = std::atoi( opt ); } );
    return 1;
}
catch ( ... )
{
    return 0;
}

int
args_parse( args_handle args, int argc, char* argv[] ) try
{
    if ( !args )
    {
        return 0;
    }
    bool in_error = false;
    optind = 1;
    arg_parser& args_ = *( to_parser( args ) );
    std::string short_opts =
        args_detail::generate_short_options( args_.arguments );
    auto long_opts = args_detail::generate_long_options( args_.arguments );

    std::for_each( args_.arguments.begin( ),
                   args_.arguments.end( ),
                   []( args_detail::argument& arg ) { arg.initializer( ); } );
    while ( true )
    {
        int opt = ::getopt_long(
            argc, argv, short_opts.c_str( ), long_opts.data( ), nullptr );
        if ( opt < 0 )
        {
            break;
        }
        auto it = args_detail::find_arg( args_.arguments, opt );
        if ( it == args_.arguments.end( ) )
        {
            in_error = true;
            break;
        }
        else
        {
            it->handler( optarg );
        }
    }
    if ( in_error || args_.should_show_help( ) )
    {
        std::for_each(
            args_.arguments.begin( ),
            args_.arguments.end( ),
            []( args_detail::argument& arg ) { arg.initializer( ); } );

        args_fprint_usage( args, argv[ 0 ], stderr );
        return ( in_error ? 0 : -1 );
    }
    return 1;
}
catch ( ... )
{
    return 0;
}
}

//
// Created by jonathan.hanks on 1/28/20.
//

#ifndef DAQD_TRUNK_ARGS_INTERNAL_HH
#define DAQD_TRUNK_ARGS_INTERNAL_HH

#include <algorithm>
#include <functional>
#include <string>
#include <type_traits>

#include <args.h>
#include <getopt.h>

namespace args_detail
{
    enum class argument_class
    {
        FLAG_ARG,
        INT_ARG,
        STRING_PTR_ARG,
    };

    struct argument
    {
        using initializer_type = std::function< void( void ) >;
        using handler_type = std::function< void( const char* opt ) >;

        argument( char             short_opt,
                  std::string      long_opt,
                  std::string      unit_text,
                  std::string      help,
                  argument_class   arg_type,
                  initializer_type arg_init,
                  handler_type     arg_handler )
            : short_form{ short_opt },
              long_form( std::move( long_opt ) ), units{ std::move(
                                                      unit_text ) },
              description{ std::move( help ) }, arg_class{ arg_type },
              initializer{ std::move( arg_init ) }, handler{ std::move(
                                                        arg_handler ) }
        {
        }

        char                          short_form{ 0 };
        std::string                   long_form{ "" };
        std::string                   units{ "" };
        std::string                   description{ "" };
        argument_class                arg_class{ argument_class::FLAG_ARG };
        std::function< void( void ) > initializer;
        std::function< void( const char* opt ) > handler;
    };

    const int ARG_INDEX_BASE = 1000;

    template < typename C >
    std::string
    generate_short_options( C& args )
    {
        static_assert(
            std::is_same< typename C::value_type, argument >::value,
            "generate_short_options must be passed a container of arguments" );

        std::string results;
        results.reserve( args.size( ) * 2 );
        for ( const auto& arg : args )
        {
            if ( arg.short_form == ARGS_NO_SHORT )
            {
                continue;
            }
            results.push_back( arg.short_form );
            if ( arg.arg_class != argument_class::FLAG_ARG )
            {
                results.push_back( ':' );
            }
        }
        return std::move( results );
    }

    template < typename C >
    std::vector< ::option >
    generate_long_options( C& args )
    {
        static_assert(
            std::is_same< typename C::value_type, argument >::value,
            "generate_long_options must be passed a container of arguments" );
        std::vector< ::option > results;
        int                     index = ARG_INDEX_BASE - 1;
        for ( const auto& arg : args )
        {
            ++index;
            if ( arg.long_form.empty( ) )
            {
                continue;
            }
            results.emplace_back(
                ::option{ arg.long_form.c_str( ),
                          ( arg.arg_class == argument_class::FLAG_ARG
                                ? no_argument
                                : required_argument ),
                          nullptr,
                          index } );
        }
        results.emplace_back( ::option{ nullptr, no_argument, nullptr, 0 } );
        return std::move( results );
    }

    template < typename C >
    typename C::iterator
    find_arg( C& args, int index )
    {
        static_assert( std::is_same< typename C::value_type, argument >::value,
                       "find_arg must be passed a container of arguments" );
        if ( index <= 0 )
        {
            return args.end( );
        }
        if ( index >= ARG_INDEX_BASE )
        {
            int new_index = index - ARG_INDEX_BASE;
            if ( new_index < args.size( ) )
            {
                return args.begin( ) + new_index;
            }
            return args.end( );
        }
        return std::find_if(
            args.begin( ), args.end( ), [index]( argument& arg ) -> bool {
                return static_cast< int >( arg.short_form ) == index;
            } );
    }
} // namespace args_detail

#endif // DAQD_TRUNK_ARGS_INTERNAL_HH

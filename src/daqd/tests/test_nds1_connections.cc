//
// Created by jonathan.hanks on 11/1/19.
//
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <string>
#include <vector>

#include "nds.hh"

std::string
select_channel_name( NDS::channels_type& channels )
{
    return channels[ rand( ) % channels.size( ) ].Name( );
}

NDS::channels_type
get_channels( )
{
    return NDS::find_channels(
        NDS::channel_predicate( NDS::channel::CHANNEL_TYPE_ONLINE ) );
}

void
join_thread( std::thread& th )
{
    th.join( );
}

void
thread_loop( int                  index,
             const std::string&   channel_name,
             std::atomic< bool >* done_flag,
             std::atomic< int >*  success_counter,
             std::atomic< int >*  error_counter )
{
    bool                                first = true;
    bool                                quiting = false;
    NDS::request_period                 period;
    NDS::connection::channel_names_type channels;
    channels.push_back( channel_name );
    try
    {
        auto stream = NDS::iterate( period, channels );
        for ( const auto& buf : stream )
        {
            if ( first )
            {
                ( *success_counter )++;
                first = false;
            }
            if ( done_flag->load( ) )
            {
                quiting = true;
                break;
            }
        }
    }
    catch ( ... )
    {
        if ( !quiting )
        {
            ( *error_counter )++;
        }
    }
}

int
main( int argc, char* argv[] )
{
    NDS::channels_type channels = get_channels( );

    std::atomic< bool > done{ false };
    std::atomic< int >  success{ 0 };
    std::atomic< int >  error{ 0 };

    const int                  THREADS = 80;
    std::vector< std::string > selected_chans;
    std::vector< std::thread > thread_objs;
    selected_chans.reserve( THREADS );
    thread_objs.reserve( THREADS );
    for ( int i = 0; i < THREADS; ++i )
    {
        selected_chans.emplace_back( select_channel_name( channels ) );
    }
    for ( int i = 0; i < THREADS; ++i )
    {
        thread_objs.emplace_back(
            thread_loop, i, selected_chans[ i ], &done, &success, &error );
    }
    std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
    done = true;
    std::for_each( thread_objs.begin( ), thread_objs.end( ), join_thread );
    std::cout << std::endl;
    std::cout << "We had " << success.load( ) << " successful threads."
              << std::endl;
    std::cout << "We had " << error.load( )
              << " unexpected errors in the threads." << std::endl;
    if ( error.load( ) < 20 && success.load( ) > 32 )
    {
        return 0;
    }
    return 1;
}

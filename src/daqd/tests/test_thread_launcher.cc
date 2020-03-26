#include "catch.hpp"

#include <atomic>
#include <chrono>
#include <thread>

#include "thread_launcher.hh"

TEST_CASE( "You can launch a pthread with the launch_pthread call" )
{
    std::atomic< bool >      flag{ false };
    std::atomic< pthread_t > handler_tid{};

    pthread_t      tid;
    pthread_attr_t attr;
    pthread_attr_init( &attr );

    launch_pthread( tid, attr, [&flag, &handler_tid]( ) {
        flag = true;
        handler_tid = pthread_self( );
    } );
    pthread_attr_destroy( &attr );

    while ( !flag )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }
    REQUIRE( handler_tid != pthread_self( ) );
}
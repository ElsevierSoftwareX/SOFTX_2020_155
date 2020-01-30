//
// Created by jonathan.hanks on 12/20/19.
//
#include "simple_epics.hh"
#include "fdManager.h"

#include <cstdio>
#include <chrono>
#include <cmath>
#include <thread>

int
main( int argc, char** argv )
{
    const double pi = std::acos( -1 );

    int  sin_val = 0;
    int  cos_val = 100;
    int  delay_ms = 0;
    char buffer1[ 1024 ] = "Hello World!";
    char buffer2[ 1024 ] = "0";

    int                  i = 0;
    bool                 done = false;
    simple_epics::Server server;
    server.addPV(
        simple_epics::pvIntAttributes( "TEST_SIN",
                                       &sin_val,
                                       std::make_pair( -200, 200 ),
                                       std::make_pair( -198, 198 ) ) );
    server.addPV(
        simple_epics::pvIntAttributes( "TEST_COS",
                                       &cos_val,
                                       std::make_pair( -200, 200 ),
                                       std::make_pair( -198, 198 ) ) );
    server.addPV( simple_epics::pvIntAttributes( "TEST_DELAY",
                                                 &delay_ms,
                                                 std::make_pair( -1, 200 ),
                                                 std::make_pair( 0, 198 ) ) );
    server.addPV( simple_epics::pvStringAttributes( "TEST_TEXT", buffer1 ) );
    server.addPV( simple_epics::pvStringAttributes( "TEST_MSG", buffer2 ) );

    while ( !done )
    {
        auto now = std::chrono::system_clock::now( );
        auto end_by = now + std::chrono::milliseconds( 1000 / 4 );

        fileDescriptorManager.process( 1 );

        double rad = ( pi * static_cast< double >( i ) ) / 180.0;

        sin_val = static_cast< int >( 100 * std::sin( rad ) );
        cos_val = static_cast< int >( 100 * std::cos( rad ) );

        i = ( i + 5 ) % 360;

        std::snprintf( buffer2, 1024, "%d", i );

        server.update( );

        auto sleep_for =
            std::chrono::duration_cast< std::chrono::milliseconds >(
                end_by - std::chrono::system_clock::now( ) );
        if ( sleep_for.count( ) > 0 )
        {
            std::this_thread::sleep_for( sleep_for );
        }

        delay_ms = sleep_for.count( );
    }
    return 0;
}
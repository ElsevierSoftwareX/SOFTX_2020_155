#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include "nds.hh"
//#include "framecpp/dictionary.hh"

using namespace CDS_NDS;

std::string programname; // Set to the program's executable name during run time

//
// Set log level 2 in the production system
//

int nds_log_level = 4; // Controls volume of log messages
#ifndef NDEBUG
int _debug = 4; // Controls volume of the debugging messages that is printed out
#endif

// FrameCPP::Dictionary *dict = FrameCPP::library.getCurrentVersionDictionary();
//

void
usage( int c )
{
    system_log( 1, "nds usage: nds [-l logfilename ] <pipe file name>" );
    exit( c );
}

int
parse_args( int argc, char* argv[] )
{
    int          c;
    extern char* optarg;
    extern int   optind;

    while ( ( c = getopt( argc, argv, "Hhl:" ) ) != -1 )
    {
        FILE* f;
        switch ( c )
        {
        case 'H':
        case 'h':
            usage( 0 );
            break;
        case 'l':
            f = freopen( optarg, "w", stdout );
            setvbuf( stdout, NULL, _IOLBF, 0 );
            stderr = stdout;
            break;
        default:
            usage( 1 );
        }
    }
    return optind;
}

int
main( int argc, char* argv[] )
{
    programname = Nds::basename( argv[ 0 ] );
    openlog( programname.c_str( ), LOG_PID | LOG_CONS, LOG_USER );
    int optind = parse_args( argc, argv );
    if ( argc != optind + 1 )
        usage( 1 );
    Nds nds( argv[ optind ] );
    int res = nds.run( );
    return res;
}

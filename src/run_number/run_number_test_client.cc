#include <cassert>
#include <iostream>
#include <string>

#include "run_number_client.hh"

bool
parse_args( int argc, char* argv[], std::string& target, std::string& hash )
{
    assert( argc >= 1 );
    std::string prog_name( argv[ 0 ] );

    bool need_help = false;
    for ( int i = 1; i < argc; ++i )
    {
        std::string arg( argv[ i ] );
        if ( arg == "-h" || arg == "--help" )
            need_help = true;
    }
    if ( need_help || ( argc != 3 && argc != 2 ) )
    {
        std::cerr << "Usage:\n\t" << prog_name << " [target] <hash>\n\n";
        std::cerr << "Where target defaults to '" << target
                  << "' if not specified." << std::endl;
        return false;
    }
    if ( argc == 2 )
    {
        hash = argv[ 1 ];
    }
    if ( argc == 3 )
    {
        target = argv[ 1 ];
        hash = argv[ 2 ];
    }
    return true;
}

int
main( int argc, char* argv[] )
{
    std::string target = "tcp://localhost:5556";
    std::string hash = "";

    if ( !parse_args( argc, argv, target, hash ) )
    {
        return 1;
    }

    int number = daqd_run_number::get_run_number( target, hash );
    std::cout << "The new run number is " << number << std::endl;

    return 0;
}
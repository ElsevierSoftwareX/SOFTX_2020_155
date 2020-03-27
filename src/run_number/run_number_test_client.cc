#include <cassert>
#include <iostream>
#include <string>

#include "args.h"
#include "run_number_client.hh"

bool
parse_args( int argc, char* argv[], std::string& target, std::string& hash )
{
    const char* default_target = "127.0.0.1:5556";
    const char* default_hash = "1111111111111111111";

    const char* target_dest = nullptr;
    const char* hash_dest = nullptr;

    auto parser =
        args_create_parser( "Simple test of the run number server, give the "
                            "destination and a hash value." );
    args_add_string_ptr( parser,
                         't',
                         "target",
                         "ip:port",
                         "address and port of the server",
                         &target_dest,
                         default_target );
    args_add_string_ptr( parser,
                         ARGS_NO_SHORT,
                         "hash",
                         "",
                         "hash value to send",
                         &hash_dest,
                         default_hash );

    if ( args_parse( parser, argc, argv ) <= 0 )
    {
        return false;
    }
    target = target_dest;
    hash = hash_dest;
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
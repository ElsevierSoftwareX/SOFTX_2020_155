#include <iostream>
#include <string>
#include <fstream>

#include "args.h"

#include "run_number.hh"
#include "run_number_server.hh"

static const char* default_db = "run-number.db";
static const char* default_endpoint = "0.0.0.0:5556";

struct config
{
    std::string db_path;
    std::string endpoint;
    bool        verbose;

    config( )
        : db_path( default_db ), endpoint( default_endpoint ), verbose( false )
    {
    }
    config( const config& other ) = default;
    config& operator=( const config& other ) = default;
};

bool
parse_args( int argc, char* argv[], config& cfg )
{
    auto parser = args_create_parser(
        "LIGO run number server.  The run number server listens "
        "for requests from the frame writers and helps to "
        "synchronize the run number based on configuration "
        "hashes." );
    int         verbose = 0;
    const char* file_dest = nullptr;
    const char* endpoint_dest = nullptr;

    args_add_flag( parser, 'v', "verbose", "verbose output", &verbose );
    args_add_string_ptr( parser,
                         'f',
                         "file",
                         "",
                         "run number database",
                         &file_dest,
                         default_db );
    args_add_string_ptr( parser,
                         'l',
                         "listen",
                         "ip:port",
                         "address and port to listen on",
                         &endpoint_dest,
                         default_endpoint );
    if ( args_parse( parser, argc, argv ) <= 0 )
    {
        return false;
    }
    cfg.verbose = ( verbose > 0 );
    cfg.db_path = file_dest;
    cfg.endpoint = endpoint_dest;
    return true;
}

int
main( int argc, char* argv[] )
{
    config cfg;
    if ( !parse_args( argc, argv, cfg ) )
    {
        return 1;
    }
    std::ofstream dummy{};
    std::ostream* log = &dummy;
    if ( cfg.verbose )
    {
        log = &std::cerr;
    }

    daqdrn::file_backing_store                       db( cfg.db_path );
    daqdrn::run_number< daqdrn::file_backing_store > run_number_generator( db );

    *log << "Binding to " << cfg.endpoint << std::endl;
    auto server_address =
        daqd_run_number::parse_connection_string( cfg.endpoint );

    daqd_run_number::server< decltype( run_number_generator ) > server(
        run_number_generator, server_address, *log );
    server.run( );
    return 0;
}
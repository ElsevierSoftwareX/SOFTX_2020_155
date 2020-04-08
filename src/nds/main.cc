#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include "nds.hh"
#include "args.h"

#include <boost/filesystem.hpp>

using namespace CDS_NDS;
namespace fs = boost::filesystem;

struct NDSConfigOpts
{
    fs::path run_dir{};
    fs::path log_path{};
    bool     exit{ false };

    fs::path
    socket_path( ) const
    {
        return run_dir / "daqd_socket";
    }

    fs::path
    jobs_dir( ) const
    {
        return run_dir / "jobs";
    };
};
//
// Set log level 2 in the production system
//

int nds_log_level = 4; // Controls volume of log messages
#ifndef NDEBUG
int _debug = 4; // Controls volume of the debugging messages that is printed out
#endif

NDSConfigOpts
parse_args( int argc, char* argv[] )
{
    NDSConfigOpts opts;

    const char* log_dest = nullptr;
    const char* run_dir = nullptr;
    const char* default_run_dir = "/var/run/nds";

    auto arg_parser = args_create_parser(
        "The archive data retrieval service for daqd data" );
    args_add_string_ptr( arg_parser,
                         'l',
                         ARGS_NO_LONG,
                         "filename",
                         "Send log information to the given file",
                         &log_dest,
                         nullptr );
    args_add_string_ptr(
        arg_parser,
        'd',
        "rundir",
        "path",
        "Directory that the jobs dir and daqd_socket reside in",
        &run_dir,
        default_run_dir );
    opts.exit = ( args_parse( arg_parser, argc, argv ) < 1 );
    args_destroy( &arg_parser );

    opts.run_dir = run_dir;
    opts.log_path = ( log_dest ? log_dest : "" );
    return opts;
}

void
setup_logging( const std::string& programname, const NDSConfigOpts& opts )
{
    openlog( programname.c_str( ), LOG_PID | LOG_CONS, LOG_USER );
    if ( !opts.log_path.empty( ) )
    {
        FILE* f = freopen( opts.log_path.c_str( ), "w", stdout );
        setvbuf( stdout, NULL, _IOLBF, 0 );
        stderr = stdout;
    }
}

void
ensure_jobs_dir( const NDSConfigOpts& opts )
{
    namespace fs = boost::filesystem;

    fs::path jobs_dir = opts.jobs_dir( );
    if ( fs::exists( jobs_dir ) )
    {
        if ( fs::is_directory( jobs_dir ) )
        {
            return;
        }
        std::ostringstream os;
        os << "The jobs directory '" << jobs_dir
           << "' exists and is not a directory";
        throw std::runtime_error( os.str( ) );
    }
    std::cout << "The jobs directory does not exist, attempting to create '"
              << jobs_dir << "'" << std::endl;
    fs::create_directories( jobs_dir );
}

int
main( int argc, char* argv[] )
{
    auto opts = parse_args( argc, argv );
    if ( opts.exit )
    {
        return 1;
    }

    std::string programname = Nds::basename( argv[ 0 ] );
    setup_logging( programname, opts );

    std::string socket_path = opts.socket_path( ).string( );
    std::cout << "NDS server starting\nRun dir = " << opts.run_dir;
    std::cout << "\nSocket path = " << socket_path;
    std::cout << "\nJobs dir = " << opts.jobs_dir( ) << std::endl;

    fs::current_path( opts.run_dir );
    ensure_jobs_dir( opts );

    Nds nds( socket_path );
    int res = nds.run( );
    return res;
}

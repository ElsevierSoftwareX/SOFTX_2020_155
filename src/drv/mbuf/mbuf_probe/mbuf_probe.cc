//
// Created by jonathan.hanks on 1/19/18.
//
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <sstream>

#include <stdio.h>
#include <string.h>

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <unistd.h>

#include "drv/shmem.h"
#include "mbuf.h"

#include "mbuf_probe.hh"
#include "analyze_daq_multi_dc.hh"
#include "analyze_rmipc.hh"

void
usage( const char* progname )
{
    std::cout << progname << " a tool to manipulate mbuf buffers\n";
    std::cout << "\nUsage " << progname << " options.\n";
    std::cout << "Where options are:\n";
    std::cout << "\tlist - request a list of mbufs and status\n";
    std::cout << "\tcreate - create a mbuf\n";
    std::cout << "\tcopy - copy a mbuf to a file\n";
    std::cout << "\tdelete - decrement the usage count of an mbuf\n";
    std::cout << "\tanalyze - continually read the mbuf and do some analysis\n";
    std::cout << "\t-b <buffer name> - The name of the buffer to act on\n";
    std::cout
        << "\t-m <buffer size in MB> - The size of the buffer in megabytes\n";
    std::cout << "\t-S <buffer size> - Buffer size in bytes (must be a "
                 "multiple of 4k)\n";
    std::cout << "\t-o <filename> - Output file for the copy operation "
                 "(defaults to probe_out.bin)\n";
    std::cout
        << "\t--struct <type> - Type of structure to analyze [rmIpcStr]\n";
    std::cout << "\t--dcu <dcuid> - Optional DCU id used to select a dcu for "
                 "analysis\n";
    std::cout << "\t\twhen analyzing daq_multi_cycle buffers.\n";
    std::cout << "\t-d <offset:format> - Decode the data section, optional\n";
    std::cout
        << "\t\tPart of the analyze command.  Decode a specified stretch\n";
    std::cout << "\t\tof data, with a given format specifier (same as python "
                 "struct)\n";
    std::cout << "\t-h|--help - This help\n";
    std::cout << "\n";
    std::cout << "Analysis modes:\n";
    std::cout << "\trmIpcStr (or rmipcstr) Analyze a models output buffer\n";
    std::cout
        << "\tdaq_multi_cycle Analyze the output of a streamer/local_dc\n";
}

ConfigOpts
parse_options( int argc, char* argv[] )
{
    ConfigOpts                            opts;
    std::map< std::string, MBufCommands > command_lookup;

    command_lookup.insert( std::make_pair( "list", LIST ) );
    command_lookup.insert( std::make_pair( "create", CREATE ) );
    command_lookup.insert( std::make_pair( "copy", COPY ) );
    command_lookup.insert( std::make_pair( "delete", DELETE ) );
    command_lookup.insert( std::make_pair( "analyze", ANALYZE ) );

    std::map< std::string, MBufStructures > struct_lookup;
    struct_lookup.insert( std::make_pair( "rmIpcStr", MBUF_RMIPC ) );
    struct_lookup.insert( std::make_pair( "rmipcstr", MBUF_RMIPC ) );
    struct_lookup.insert(
        std::make_pair( "daq_multi_cycle", MBUF_DAQ_MULTI_DC ) );

    std::deque< std::string > args;
    for ( int i = 1; i < argc; ++i )
    {
        args.push_back( argv[ i ] );
    }
    while ( !args.empty( ) )
    {
        std::string arg = args.front( );
        args.pop_front( );
        if ( arg == "-h" || arg == "--help" )
        {
            opts.action = INVALID;
            opts.error_msg = "";
            return opts;
        }
        else if ( arg == "-b" )
        {
            if ( args.empty( ) )
            {
                opts.set_error( "You must specify a buffer name after -b" );
                return opts;
            }
            opts.buffer_name = args.front( );
            args.pop_front( );
        }
        else if ( arg == "-m" || arg == "-S" )
        {
            if ( args.empty( ) )
            {
                opts.set_error( "You must specify a size when using -m or -S" );
                return opts;
            }
            std::size_t        multiplier = ( arg == "-m" ? 1024 * 1024 : 1 );
            std::istringstream os( args.front( ) );
            os >> opts.buffer_size;
            args.pop_front( );
            opts.buffer_size *= multiplier;
        }
        else if ( arg == "-o" )
        {
            if ( args.empty( ) )
            {
                opts.set_error( "You must specify a filename when using -o" );
                return opts;
            }
            opts.output_fname = args.front( );
            args.pop_front( );
        }
        else if ( arg == "-d" )
        {
            if ( args.empty( ) )
            {
                opts.set_error(
                    "You must specify a format string when using -d" );
                return opts;
            }
            std::string format = args.front( );
            args.pop_front( );
            std::string::size_type split = format.find( ':' );
            if ( split == std::string::npos )
            {
                opts.set_error( "You must have a format strip when using -d" );
                return opts;
            }
            std::string        offset_str = format.substr( 0, split );
            std::string        format_spec = format.substr( split + 1 );
            std::istringstream is( offset_str );
            std::size_t        offset = 0;
            is >> offset;
            opts.decoder = DataDecoder( offset, format_spec );
        }
        else if ( arg == "--struct" )
        {
            if ( args.empty( ) )
            {
                opts.set_error(
                    "You must specify a structure type when using --struct" );
                return opts;
            }
            std::map< std::string, MBufStructures >::iterator it;
            it = struct_lookup.find( args.front( ) );
            if ( it == struct_lookup.end( ) )
            {
                opts.set_error( "Invalid structure type passed to --struct" );
                return opts;
            }
            opts.analysis_type = it->second;
            args.pop_front( );
        }
        else if ( arg == "--dcu" )
        {
            if ( args.empty( ) )
            {
                opts.set_error( "You must specify a dcu id when using --dcu" );
                return opts;
            }
            std::istringstream is( args.front( ) );
            is >> opts.dcu_id;
            args.pop_front( );
        }
        else
        {
            std::map< std::string, MBufCommands >::iterator it;
            it = command_lookup.find( arg );
            if ( it == command_lookup.end( ) )
            {
                std::ostringstream os;
                os << "Unknown argument " << args.front( );
                opts.set_error( os.str( ) );
                return opts;
            }
            else
            {
                if ( !opts.select_action( it->second ) )
                {
                    return opts;
                }
            }
        }
    }
    opts.validate_options( );
    return opts;
};

void
shmem_inc_segment_count( const char* sys_name )
{
    int    fd = -1;
    size_t name_len = 0;

    if ( !sys_name )
        return;
    name_len = strlen( sys_name );
    if ( name_len == 0 || name_len > MBUF_NAME_LEN )
    {
        return;
    }

    if ( ( fd = open( "/dev/mbuf", O_RDWR | O_SYNC ) ) < 0 )
    {
        fprintf( stderr, "Couldn't open /dev/mbuf read/write\n" );
        return;
    }
    struct mbuf_request_struct req;
    req.size = 1;
    strcpy( req.name, sys_name );
    ioctl( fd, IOCTL_MBUF_ALLOCATE, &req );
    ioctl( fd, IOCTL_MBUF_ALLOCATE, &req );
    close( fd );
}

void
shmem_dec_segment_count( const char* sys_name )
{
    int    fd = -1;
    size_t name_len = 0;

    if ( !sys_name )
        return;
    name_len = strlen( sys_name );
    if ( name_len == 0 || name_len > MBUF_NAME_LEN )
    {
        return;
    }

    if ( ( fd = open( "/dev/mbuf", O_RDWR | O_SYNC ) ) < 0 )
    {
        fprintf( stderr, "Couldn't open /dev/mbuf read/write\n" );
        return;
    }
    struct mbuf_request_struct req;
    req.size = 1;
    strcpy( req.name, sys_name );
    ioctl( fd, IOCTL_MBUF_DEALLOCATE, &req );
    close( fd );
}

/**
 * @brief Wrapper to make sure we always close C style FILE*
 */
class safe_file
{
    FILE* _f;
    safe_file( );
    safe_file( const safe_file& other );
    safe_file& operator=( const safe_file& other );

public:
    safe_file( FILE* f ) : _f( f ){};
    ~safe_file( )
    {
        if ( _f )
        {
            fclose( _f );
            _f = NULL;
        }
    }
    FILE*
    get( ) const
    {
        return _f;
    }
};

int
copy_shmem_buffer( const volatile void* buffer,
                   const std::string&   output_fname,
                   size_t               req_size )
{
    std::vector< char > dest_buffer( req_size );

    safe_file f( fopen( output_fname.c_str( ), "wb" ) );
    if ( !f.get( ) )
        return 1;
    const volatile char* src =
        reinterpret_cast< const volatile char* >( buffer );
    memcpy( dest_buffer.data( ), const_cast< const char* >( src ), req_size );
    fwrite( dest_buffer.data( ), 1, dest_buffer.size( ), f.get( ) );
    return 0;
}

void
list_shmem_segments( )
{
    std::vector< char > buf( 100 );
    std::ifstream       f( "/sys/kernel/mbuf/status" );
    while ( f.read( buf.data( ), buf.size( ) ) )
    {
        std::cout.write( buf.data( ), buf.size( ) );
    }
    if ( f.gcount( ) > 0 )
    {
        std::cout.write( buf.data( ), f.gcount( ) );
    }
}

int
handle_analyze( const ConfigOpts& opts )
{
    const int OK = 0;
    const int ERROR = 1;

    volatile void* buffer =
        shmem_open_segment( opts.buffer_name.c_str( ), opts.buffer_size );
    switch ( opts.analysis_type )
    {
    case MBUF_RMIPC:
        analyze::analyze_rmipc( buffer, opts.buffer_size, opts );
        break;
    case MBUF_DAQ_MULTI_DC:
        analyze::analyze_multi_dc( buffer, opts.buffer_size, opts );
        break;
    default:
        std::cout << "Unknown analysis type\n";
        return ERROR;
    }
    return OK;
}

int
main( int argc, char* argv[] )
{
    ConfigOpts opts = parse_options( argc, argv );
    if ( opts.should_show_help( ) )
    {
        usage( argv[ 0 ] );
        return ( opts.is_in_error( ) ? 1 : 0 );
    }

    switch ( opts.action )
    {
    case LIST:
        list_shmem_segments( );
        break;
    case CREATE:
        if ( !shmem_open_segment( opts.buffer_name.c_str( ),
                                  opts.buffer_size ) )
        {
            std::cout << "Unable to create/open mbuf buffer "
                      << opts.buffer_name << "\n";
            return 1;
        }
        shmem_inc_segment_count( opts.buffer_name.c_str( ) );
        break;
    case COPY:
    {
        volatile void* buffer =
            shmem_open_segment( opts.buffer_name.c_str( ), opts.buffer_size );
        if ( !buffer )
        {
            std::cout << "Unable to create/open mbuf buffer "
                      << opts.buffer_name << "\n";
            return 1;
        }
        return copy_shmem_buffer( buffer, opts.output_fname, opts.buffer_size );
    }
    break;
    case DELETE:
        shmem_dec_segment_count( opts.buffer_name.c_str( ) );
        break;
    case ANALYZE:
        return handle_analyze( opts );
    }
    return 0;
}

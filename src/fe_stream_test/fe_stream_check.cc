//
// Created by jonathan.hanks on 7/26/19.
//
#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <utility>

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <drv/shmem.h>

#include "fe_stream_generator.hh"

#include "daq_core.h"

extern "C" {

#include "../drv/crc.c"
}

/*!
 * @brief Generic buffer typedef
 */
typedef std::vector< char > buffer_t;

/*!
 * @brief Pair of config files for a model the .ini file and .par file
 */
typedef std::pair< std::string, std::string > model_config_pair;

int
to_int( const std::string& s )
{
    int                tmp = 0;
    std::istringstream is( s );
    is >> tmp;
    return tmp;
}

int
extract_int_param( const std::string& s )
{
    std::string::size_type start = s.find( '=' );
    std::string            param = s.substr( start + 1 );
    return to_int( param );
}

struct model_meta_data
{
    model_meta_data( ) : dcuid( 0 ), rate( 0 )
    {
    }

    int dcuid;
    int rate;
};

/*!
 * @brief Represents the channel/test point list for a model
 */
struct model_channel_list
{
    model_channel_list( ) : meta_data( ), channels( ), test_points( )
    {
    }

    model_meta_data meta_data;

    std::vector< GeneratorPtr > channels;
    std::vector< GeneratorPtr > test_points;
};

void
wait_for_file_to_exist( const char* fname )
{
    if ( !fname )
    {
        throw std::runtime_error( "Bad file name in wait_for_file_to_exist" );
    }
    int tries = 0;
    do
    {
        struct stat fInfo;
        if ( stat( fname, &fInfo ) == 0 )
        {
            return;
        }
        sleep( 1 );
        ++tries;
    } while ( tries < 5 );
    std::cerr << "Cannot access " << fname << "\n";
    throw std::runtime_error( "Cannot access file" );
}

/*!
 * @brief Read the channel list from a ini or par file
 * @param fname the file to read from
 * @param[out] meta_data Pointer to option meta data info, to record dcu id and
 * such...
 * @param[out] output destination for channel lists
 */
void
read_channels_from_file( const std::string&           fname,
                         model_meta_data*             meta_data,
                         std::vector< GeneratorPtr >& output )
{
    wait_for_file_to_exist( fname.c_str( ) );
    std::vector< GeneratorPtr > channels;
    std::ifstream               is( fname.c_str( ), std::ios_base::in );
    bool                        in_default = 0;

    std::string line;
    while ( std::getline( is, line ) )
    {
        if ( line.empty( ) )
        {
            continue;
        }

        if ( line.front( ) != '[' || line.back( ) != ']' )
        {
            if ( !in_default || !meta_data )
            {
                continue;
            }
            if ( line.size( ) > 6 and
                 strncmp( "dcuid=", line.c_str( ), 6 ) == 0 )
            {
                meta_data->dcuid = extract_int_param( line );
            }
            if ( line.size( ) > 9 and
                 strncmp( "datarate=", line.c_str( ), 9 ) == 0 )
            {
                meta_data->rate = extract_int_param( line );
            }
        }
        else
        {
            in_default = false;
            if ( line == "[default]" )
            {
                in_default = true;
            }
            else
            {
                std::string chan_name = line.substr( 1, line.size( ) - 2 );
                channels.push_back( create_generator( chan_name ) );
            }
        }
    }

    output.swap( channels );
}

/*!
 * @brief Given a model_config_pair return the channel list associated with the
 * model
 * @return Channel listing information for the given model
 */
model_channel_list
load_channels_for_model( const model_config_pair& info )
{
    model_channel_list results;
    read_channels_from_file( info.first, &results.meta_data, results.channels );
    if ( results.meta_data.dcuid == 0 )
    {
        std::cerr << "Unable to extract dcuid from " << info.first << "\n";
        throw std::runtime_error( "Unable to extract dcuid from ini file" );
    }
    read_channels_from_file( info.second, 0, results.test_points );
    return results;
}

/*!
 * @brief Simple RAII wrapper around a c-style FILE*
 */
class SafeFILE
{
    SafeFILE( const SafeFILE& other );
    SafeFILE& operator=( const SafeFILE& other );

public:
    explicit SafeFILE( FILE* f ) : f_( f )
    {
    }
    ~SafeFILE( )
    {
        if ( f_ )
        {
            fclose( f_ );
        }
    }

    operator bool( ) const
    {
        return f_ != 0;
    }

    FILE*
    get( )
    {
        return f_;
    }

    const FILE*
    get( ) const
    {
        return f_;
    }

private:
    FILE* f_;
};

/*!
 * @brief Holds the application configuration
 */
struct config_t
{
    config_t( )
        : mbuf_name( "" ), file_name( "" ), mbuf_size( 0 ), verbose( false ),
          pause_before_checks( false ), show_help( false ), configs( ),
          error_message( "" )
    {
    }

    std::string                      mbuf_name;
    std::string                      file_name;
    int                              mbuf_size;
    std::vector< model_config_pair > configs;
    bool                             verbose;
    bool                             pause_before_checks;
    bool                             show_help;
    std::string                      error_message;

    void
    set_error( const char* msg )
    {
        show_help = true;
        error_message = msg;
    }

    /*!
     * @brief Make sure options are internally consistent.  Set show_help and
     * error_message if they are not.
     */
    void
    consistency_check( )
    {
        if ( show_help )
        {
            return;
        }
        if ( mbuf_name.empty( ) && file_name.empty( ) )
        {
            set_error( "You must select either mbuf input or file input" );
            return;
        }
        if ( !mbuf_name.empty( ) && !file_name.empty( ) )
        {
            set_error( "You may only select one of mbuf input or file input" );
            return;
        }
        if ( !mbuf_name.empty( ) && mbuf_size <= 0 )
        {
            set_error( "You must specify a valid mbuf size to use mbuf input" );
            return;
        }
        if ( configs.empty( ) )
        {
            set_error( "You must include at least one config to check" );
            return;
        }
    }
};

void
usage( const char* prog_name )
{
    std::cout << "Given a data dump or mbuf buffer, verify the contents of the "
                 "data\n\n";
    std::cout << "Usage\n";
    std::cout << prog_name << " [options]\n";
    std::cout << "\nWhere options are:\n";
    std::cout << "\t-h,--help\t\tThis help\n";
    std::cout << "\t-v\t\tVerbose output\n";
    std::cout
        << "\t-m,--mbuf [buffer name]\tRead input from the given mbuf buffer\n";
    std::cout
        << "\t-s,--mbuf-size [size in MB]\tSize of the mbuf buffer to read\n";
    std::cout
        << "\t-f,--input-file [file name]\tRead input from a local file\n";
    std::cout << "\t-p\t\tPause before checks\n";
    std::cout << "\t-c [ini file] [par file]\tRead model config from the given "
                 "ini and par file\n";
    std::cout << "\n";
    std::cout << "You must specify either mbuf or local file input.  If you "
                 "specify mbuf input\n";
    std::cout << "you must then also specify the size of the mbuf.\n";
    std::cout << "\n";
    std::cout << "You must specify at least one pair of configuration files.  "
                 "For each pair of\n";
    std::cout << "config files specified all channels listed in the config "
                 "will be looked for in\n";
    std::cout << "data buffer.  Repeating the -c option is how you specify "
                 "multiple sets of config\n";
    std::cout << "files.\n";
    std::cout << "\nIt should be noted that this only works with the newer "
                 "daq_multi_cycle_data_t\n";
    std::cout << "structure, not the rmIpcStr as the input buffer.\n";
}

config_t
parse_args( int argc, char* argv[] )
{
    config_t                  opts;
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
            opts.show_help = true;
        }
        else if ( arg == "-v" )
        {
            opts.verbose = true;
        }
        else if ( arg == "-m" || arg == "--mbuf" )
        {
            if ( args.empty( ) )
            {
                opts.set_error( "You must specify a buffer name to use mbuf" );
                break;
            }
            opts.mbuf_name = args.front( );
            args.pop_front( );
        }
        else if ( arg == "-s" || arg == "--mbuf-size" )
        {
            if ( args.empty( ) )
            {
                opts.set_error( "You must specify a size in MB" );
                break;
            }
            opts.mbuf_size = to_int( args.front( ) ) * 1024 * 1024;
            args.pop_front( );
        }
        else if ( arg == "-f" || arg == "--input-file" )
        {
            if ( args.empty( ) )
            {
                opts.set_error(
                    "You must specify a file name to use file input" );
                break;
            }
            opts.file_name = args.front( );
            args.pop_front( );
        }
        else if ( arg == "-p" )
        {
            opts.pause_before_checks = true;
        }
        else if ( arg == "-c" )
        {
            if ( args.size( ) < 2 )
            {
                opts.set_error( "When specifying a model config to check you "
                                "must include two files [.ini and .par]" );
                break;
            }
            opts.configs.push_back( std::make_pair( args[ 0 ], args[ 1 ] ) );
            args.pop_front( );
            args.pop_front( );
        }
    }
    opts.consistency_check( );
    return opts;
}

long
get_file_size( FILE* f )
{
    long results = 0;

    if ( !f )
    {
        return 0;
    }
    long pos = ftell( f );
    if ( pos < 0 )
    {
        return 0;
    }
    if ( fseek( f, 0, SEEK_END ) < 0 )
    {
        return 0;
    }
    results = ftell( f );
    if ( results < 0 )
    {
        return 0;
    }
    if ( fseek( f, pos, SEEK_SET ) < 0 )
    {
        return 0;
    }
    return results;
}

void
read_n_bytes( FILE* f, long length, char* dest )
{

    while ( length )
    {
        long count = fread( dest, 1, length, f );
        if ( count <= 0 )
        {
            throw std::runtime_error( "Error reading the file" );
        }
        length -= count;
    }
}

/*!
 * @brief Retrieve a copy of the data buffer as described in the config object
 * @param opts The configuration object
 * @return A copy of the buffer in the form of a vector<char>
 */
std::vector< char >
load_buffer( config_t& opts )
{
    buffer_t buffer;

    if ( !opts.mbuf_name.empty( ) )
    {
        int tries = 5;

        volatile void* data =
            shmem_open_segment( opts.mbuf_name.c_str( ), opts.mbuf_size );
        buffer.resize( opts.mbuf_size );

        daq_multi_cycle_header_t* header =
            (daq_multi_cycle_header_t*)( &buffer[ 0 ] );

        std::copy(
            (char*)data, ( (char*)data ) + opts.mbuf_size, buffer.begin( ) );
        // when run in an automated way, the read happens too quick, wait
        // for data to show up (with a timeout)
        while ( tries && header->curCycle >= 16 )
        {
            std::cout << "Waiting for good data\n";
            sleep( 1 );
            --tries;
            std::copy( (char*)data,
                       ( (char*)data ) + opts.mbuf_size,
                       buffer.begin( ) );
        }
    }
    else if ( !opts.file_name.empty( ) )
    {
        buffer_t tmp;
        SafeFILE f( fopen( opts.file_name.c_str( ), "rb" ) );
        long     fsize = get_file_size( f.get( ) );
        if ( fsize > 0 )
        {
            tmp.resize( fsize );
            read_n_bytes( f.get( ), fsize, &tmp[ 0 ] );
            tmp.swap( buffer );
        }
    }
    return buffer;
}

/*!
 * @breif This class looks for a models data in a specifided
 * daq_multi_cycle_data_t buffer.  It ensures that all channels are available in
 * the buffer with their expected values.  It also checks test points, if a test
 * point is marked as active in the data block it will verify the test point
 * data layout and contents for the model.
 */
class CheckDCUData
{
public:
    CheckDCUData( config_t& opts, const daq_multi_cycle_data_t* buffer )
        : opts_( opts ), buffer_( buffer )
    {
    }

    void
    operator( )( const model_channel_list& model )
    {
        if ( opts_.verbose )
        {
            std::cout << "Checking on dcu " << model.meta_data.dcuid << "\n";
        }
        int         cycle = buffer_->header.curCycle;
        std::size_t data_stride = buffer_->header.cycleDataSize;
        if ( data_stride % 8 != 0 )
        {
            std::cerr << "cycleDataSize is not 64bit aligned!\n";
            std::cerr << "cycleDataSize = " << data_stride << "\n";
            throw std::runtime_error( "cycleDataSize is not 64bit aligned" );
        }

        const daq_multi_dcu_data_t* slice_header =
            (const daq_multi_dcu_data_t*)( &(
                buffer_->dataBlock[ data_stride * cycle ] ) );
        unsigned int dcu_data_offset = 0;

        const daq_msg_header_t* dcu_header = 0;
        for ( int model_index = 0;
              model_index < slice_header->header.dcuTotalModels;
              ++model_index )
        {
            if ( slice_header->header.dcuheader[ model_index ].dcuId ==
                 model.meta_data.dcuid )
            {
                dcu_header = &( slice_header->header.dcuheader[ model_index ] );
                break;
            }
            dcu_data_offset +=
                slice_header->header.dcuheader[ model_index ].dataBlockSize;
            dcu_data_offset +=
                slice_header->header.dcuheader[ model_index ].tpBlockSize;
        }
        if ( !dcu_header )
        {
            std::cerr << "Unable to find dcuid " << model.meta_data.dcuid;
            std::cerr << " in the data\n";
            throw std::runtime_error( "Could not find a required DCUID" );
        }
        std::vector< char > tmp( model.meta_data.rate * 8 );

        const char* data = &( slice_header->dataBlock[ dcu_data_offset ] );
        char*       data_start = const_cast< char* >( data );
        for ( int i = 0; i < model.channels.size( ); ++i )
        {
            std::fill( tmp.begin( ), tmp.end( ), 1 );
            char* gen_start = &tmp[ 0 ];
            char* gen_end = model.channels[ i ]->generate(
                dcu_header->timeSec, dcu_header->timeNSec, gen_start );
            int gen_size = ( gen_end - gen_start );

            if ( !std::equal( gen_start, gen_end, data ) )
            {
                std::cerr << "Data mismatch on "
                          << model.channels[ i ]->full_channel_name( ) << "\n";
                throw std::runtime_error( "Data mismatch" );
            }

            data += gen_size;
        }

        int          dcu_data_size = static_cast< int >( data - data_start );
        unsigned int data_crc =
            crc_len( dcu_data_size, crc_ptr( data_start, dcu_data_size, 0 ) );
        if ( data_crc != dcu_header->dataCrc )
        {
            std::cerr << "CRC mismatch on dcu id " << dcu_header->dcuId << "\n";
            throw std::runtime_error( "CRC Mismatch" );
        }

        for ( int i = 0; i < dcu_header->tpCount; ++i )
        {
            int expected_len = ( model.meta_data.rate * sizeof( float ) ) / 16;
            std::fill( tmp.begin( ), tmp.end( ), 0 );
            int tp_index = dcu_header->tpCount;
            if ( tp_index != 0 )
            {
                if ( tp_index - 1 >= model.test_points.size( ) )
                {
                    std::cerr << "Requesting an invalid test point " << tp_index
                              << "\n";
                    throw std::runtime_error(
                        "Requesting an invalid test point" );
                }
                char* gen_start = &tmp[ 0 ];
                char* gen_end = model.test_points[ tp_index - 1 ]->generate(
                    dcu_header->timeSec, dcu_header->timeNSec, gen_start );
                int gen_size = ( gen_end - gen_start );
                if ( gen_size != expected_len )
                {
                    std::cerr << "Unexpected TP length of " << gen_size
                              << " bytes\n";
                    throw std::runtime_error( "Unexpected TP length" );
                }
            }
            if ( !std::equal( &tmp[ 0 ], ( &tmp[ 0 ] ) + expected_len, data ) )
            {
                std::cerr << "TP Data mismatch on #" << tp_index;
                if ( tp_index > 0 )
                {
                    std::cerr << " "
                              << model.test_points[ tp_index - 1 ]
                                     ->full_channel_name( );
                }
                std::cerr << "\n";
                throw std::runtime_error( "TP Data mismatch" );
            }
        }
    }

private:
    config_t&                     opts_;
    const daq_multi_cycle_data_t* buffer_;
};

int
main( int argc, char* argv[] )
{
    config_t opts = parse_args( argc, argv );
    if ( opts.show_help )
    {
        usage( argv[ 0 ] );
        if ( !opts.error_message.empty( ) )
        {
            std::cout << "\n" << opts.error_message << "\n";
            return 1;
        }
        return 0;
    }

    std::vector< model_channel_list > models;
    std::transform( opts.configs.begin( ),
                    opts.configs.end( ),
                    std::back_inserter( models ),
                    load_channels_for_model );

    if ( opts.verbose )
    {
        std::cout << "Loaded " << models.size( ) << " models for checking\n";
    }

    std::vector< char > data_buffer = load_buffer( opts );

    if ( opts.verbose )
    {
        std::cout << "Loaded a data buffer with a size of "
                  << data_buffer.size( ) << " bytes\n";
    }

    daq_multi_cycle_data_t* header =
        (daq_multi_cycle_data_t*)( &data_buffer[ 0 ] );

    std::size_t max_size =
        ( header->header.cycleDataSize * header->header.maxCycle ) +
        sizeof( daq_multi_cycle_header_t );
    if ( data_buffer.size( ) < sizeof( daq_multi_cycle_header_t ) ||
         data_buffer.size( ) < max_size )
    {
        std::cerr << "Buffer size is too small to accomodate the data";
    }

    if ( opts.verbose )
    {
        std::cout << "Header info:\n\tcurCycle: " << header->header.curCycle
                  << "\n";
        std::cout << "\tmaxCycle: " << header->header.maxCycle << "\n";
        std::cout << "\tcycleDataSize: " << header->header.cycleDataSize
                  << "\n";
        std::cout << "\tmsgcrc: " << header->header.msgcrc << "\n";
    }

    if ( opts.pause_before_checks )
    {
        std::cout << "Pausing before initiating checks on the buffer.\n";
        std::cout << "Press enter to continue..." << std::endl;
        std::string dummy;
        std::cin >> dummy;
    }

    CheckDCUData check_dcu_data( opts, header );
    std::for_each( models.begin( ), models.end( ), check_dcu_data );
    if ( opts.verbose )
    {
        std::cout << "Tests passed\n";
    }
    return 0;
}
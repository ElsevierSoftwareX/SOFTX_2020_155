#include <arpa/inet.h>

#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <nds.hh>

#include "fe_stream_generator.hh"

struct Config
{
    std::string                  hostname{ "localhost" };
    unsigned short               port{ 8088 };
    int                          random_channels{ 10 };
    NDS::buffer::gps_second_type gps{ 0 };
    std::vector< std::string >   channels{};
    std::int64_t                 seed{};
};

void
usage( const char* progname )
{
    using namespace std;
    cout << "Usage:\n";
    cout << progname << " [options]\n\nWhere options are:\n";
    cout << "\t-n <hostname> - server hostname [localhost]\n";
    cout << "\t-p <port>     - server port [8088]\n";
    cout << "\t-t <time>     - 0 for live, else a gps time [0]\n";
    cout << "\t-c <number>   - number of random channels to sample [10]\n";
    cout << "\t-C <channels> - 1 or more channel names to test, optional\n";
    cout << "\t-s <seed>     - seed to use for channel selection, optional\n";

    cout << "\nOnly specify -c or -C.  -C takes priority.\n";
    cout << "If -c is specified a the random number generator can be seeded\n";
    cout << "manually with the -s option.  If not specified it will be\n";
    cout << "randomly seeded.\n";

    exit( 1 );
}

Config
get_config( int argc, char* argv[] )
{
    Config cfg;

    std::random_device rd;
    cfg.seed = rd( );

    std::deque< std::string > args;
    for ( int i = 1; i < argc; ++i )
    {
        args.push_back( argv[ i ] );
    }
    while ( !args.empty( ) )
    {
        std::string arg = args.front( );
        args.pop_front( );

        // all args take options, so ...
        if ( args.empty( ) )
        {
            usage( argv[ 0 ] );
        }

        if ( arg == "-n" )
        {
            cfg.hostname = args.front( );
            args.pop_front( );
        }
        else if ( arg == "-p" )
        {
            {
                std::istringstream is( args.front( ) );
                is >> cfg.port;
            }
            args.pop_front( );
        }
        else if ( arg == "-t" )
        {
            {
                std::istringstream is( args.front( ) );
                is >> cfg.gps;
            }
            args.pop_front( );
        }
        else if ( arg == "-c" )
        {
            {
                std::istringstream is( args.front( ) );
                is >> cfg.random_channels;
            }
            args.pop_front( );
        }
        else if ( arg == "-x" )
        {
            {
                std::istringstream is( args.front( ) );
                is >> cfg.seed;
            }
            args.pop_front( );
        }
        else if ( arg == "-C" )
        {
            while ( !args.empty( ) )
            {
                cfg.channels.push_back( args.front( ) );
                args.pop_front( );
            }
        }
        else
        {
            usage( argv[ 0 ] );
        }
    }
    return cfg;
}

NDS::channels_type
get_nds_channels( Config& cfg )
{
    NDS::parameters params = NDS::parameters(
        cfg.hostname, cfg.port, NDS::connection::PROTOCOL_ONE );
    return NDS::find_channels(
        params,
        NDS::channel_predicate( "*", NDS::channel::CHANNEL_TYPE_ONLINE ) );
}

bool
is_16bit_int_channel( const NDS::channel& ch )
{
    return ch.DataType( ) == NDS::channel::DATA_TYPE_INT16;
}

void
add_16bit_int_channel( const NDS::channels_type& channels,
                       std::vector< int >&       indexes )
{
    for ( int i = 0; i < channels.size( ); ++i )
    {
        if ( is_16bit_int_channel( channels[ i ] ) &&
             std::find( indexes.begin( ), indexes.end( ), i ) ==
                 indexes.end( ) )
        {
            indexes.push_back( i );
            break;
        }
    }
}

std::vector< GeneratorPtr >
load_generators( Config& cfg )
{
    std::vector< GeneratorPtr > generators{};

    if ( cfg.channels.empty( ) )
    {
        auto                                 channels = get_nds_channels( cfg );
        std::mt19937                         robj( cfg.seed );
        std::uniform_int_distribution< int > dist( 0, channels.size( ) - 1 );

        std::cout << "Generating " << cfg.random_channels << " from seed "
                  << cfg.seed << "\n";
        std::vector< int > indexes{};
        indexes.reserve( cfg.random_channels );
        add_16bit_int_channel( channels, indexes );
        add_16bit_int_channel( channels, indexes );
        for ( int i = 0; i < indexes.size( ); ++i )
        {
            cfg.channels.push_back( channels[ indexes[ i ] ].Name( ) );
        }
        for ( int i = indexes.size( ); i < cfg.random_channels; ++i )
        {
            int index = 0;
            do
            {
                index = dist( robj );
            } while ( std::find( indexes.begin( ), indexes.end( ), index ) !=
                      indexes.end( ) );
            indexes.push_back( index );
            cfg.channels.push_back( channels[ index ].Name( ) );
        }

        std::ostream_iterator< std::string > out_it( std::cout, "\n" );
        std::copy( cfg.channels.begin( ), cfg.channels.end( ), out_it );
    }

    generators.reserve( cfg.channels.size( ) );
    for ( int i = 0; i < cfg.channels.size( ); ++i )
    {
        generators.push_back( create_generator( cfg.channels[ i ] ) );
    }

    return generators;
}

int
main( int argc, char* argv[] )
{
    Config                      cfg;
    std::vector< GeneratorPtr > generators;
    try
    {
        cfg = get_config( argc, argv );
    }
    catch ( ... )
    {
        usage( argv[ 0 ] );
    }

    generators = load_generators( cfg );

    int status = 0;

    std::cout << "Starting check at " << cfg.gps << std::endl;

    NDS::buffers_type data_from_nds;
    NDS::parameters   params(
        cfg.hostname, cfg.port, NDS::connection::PROTOCOL_ONE );

    std::cout << "Requesting data from " << cfg.gps << " to " << cfg.gps + 1
              << std::endl;

    data_from_nds.reserve( cfg.channels.size( ) );
    int remaining_channels = cfg.channels.size( );
    int start_channel = 0;
    int chunk_index = 0;
    while ( remaining_channels > 0 )
    {
        static const int                    CHUNK = 128;
        NDS::connection::channel_names_type channels;
        channels.reserve( CHUNK );

        std::cout << "Chunk " << chunk_index << "\n";

        int chunk = std::min( CHUNK, remaining_channels );
        std::copy( cfg.channels.begin( ) + start_channel,
                   cfg.channels.begin( ) + start_channel + chunk,
                   std::back_inserter( channels ) );
        remaining_channels -= chunk;
        start_channel += chunk;
        ++chunk_index;

        auto stream = NDS::iterate(
            params, NDS::request_period( cfg.gps, cfg.gps + 1 ), channels );
        int iterations = 0;
        for ( const auto& bufs : stream )
        {
            if ( iterations != 0 )
            {
                std::cerr << "This is unexpected, more buffer segments than "
                             "anticipated\n";
                exit( 1 );
            }
            std::copy( bufs->begin( ),
                       bufs->end( ),
                       std::back_inserter( data_from_nds ) );
            ++iterations;
        }
    }

    for ( int i = 0; i < generators.size( ); ++i )
    {
        Generator& cur_gen = *( generators[ i ].get( ) );

        NDS::buffer& sample_buf = data_from_nds[ i ];
        if ( sample_buf.samples_to_bytes( sample_buf.Samples( ) ) !=
             cur_gen.bytes_per_sec( ) )
        {
            std::cerr << "Length mismatch on " << cur_gen.full_channel_name( )
                      << std::endl;
            status = 1;
            continue;
        }
        std::vector< char > ref_buf( cur_gen.bytes_per_sec( ) );

        {
            char* out = &ref_buf[ 0 ];
            int   gps_sec = sample_buf.Start( );
            int   gps_nano = sample_buf.StartNano( );
            int   step = 1000000000 / 16;
            for ( int j = 0; j < 16; ++j )
            {
                out = cur_gen.generate( gps_sec, gps_nano, out );
                gps_nano += step;
            }
        }

        if ( std::memcmp( &ref_buf[ 0 ],
                          reinterpret_cast< const char* >(
                              sample_buf.cbegin< void >( ) ),
                          ref_buf.size( ) ) != 0 )
        {
            std::cerr << "There is a difference in "
                      << cur_gen.full_channel_name( ) << std::endl;
            status = 1;
            std::vector< int > bswap_buff( ref_buf.size( ) / 4 );
            int*               cur = (int*)( &ref_buf[ 0 ] );
            for ( int j = 0; j < bswap_buff.size( ); ++j )
            {
                bswap_buff[ j ] = htonl( *cur );
                ++cur;
            }
            if ( std::memcmp( &bswap_buff[ 0 ],
                              reinterpret_cast< const char* >(
                                  sample_buf.cbegin< void >( ) ),
                              ref_buf.size( ) ) == 0 )
            {
                std::cerr << "Byte order difference!!!!" << std::endl;
            }
        }
    }
    return status;
}

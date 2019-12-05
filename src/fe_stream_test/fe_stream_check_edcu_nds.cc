//
// Created by jonathan.hanks on 9/24/19.
//
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

#include <daq_data_types.h>

#include <nds.hh>

#include "fe_stream_generator.hh"

struct Config
{
    std::string                  hostname{ "localhost" };
    unsigned short               port{ 8088 };
    int                          random_channels{ 10 };
    NDS::buffer::gps_second_type gps_start{ 0 };
    NDS::buffer::gps_second_type gps_stop{ 0 };
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
    cout << "\t-start <time>     - a gps start time [0]\n";
    cout << "\t-stop <time>     - a gps end time [0]\n";
    cout << "\t-c <number>   - number of random channels to sample [10]\n";
    cout << "\t-C <channels> - 1 or more channel names to test, optional\n";
    cout << "\t-s <seed>     - seed to use for channel selection, optional\n";

    cout << "\nOnly specify -c or -C.  -C takes priority.\n";
    cout << "If -c is specified a the random number generator can be seeded\n";
    cout << "manually with the -s option.  If not specified it will be\n";
    cout << "randomly seeded.\n";
    cout << "\nTo read live data, specify a start time of 0 and use stop as a "
            "duration\n";

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
        else if ( arg == "-start" )
        {
            {
                std::istringstream is( args.front( ) );
                is >> cfg.gps_start;
            }
            args.pop_front( );
        }
        else if ( arg == "-stop" )
        {
            {
                std::istringstream is( args.front( ) );
                is >> cfg.gps_stop;
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
is_generated_channel( const NDS::channel& chan )
{
    static const std::string slow( "--16" );
    static const std::string spacer( "--" );

    auto pos = chan.Name( ).rfind( slow );
    if ( pos == std::string::npos )
    {
        return false;
    }
    return pos == chan.Name( ).size( ) - slow.size( );
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
        for ( int i = 0; i < cfg.random_channels; ++i )
        {
            int index = 0;
            do
            {
                index = dist( robj );
            } while ( std::find( indexes.begin( ), indexes.end( ), index ) !=
                          indexes.end( ) ||
                      !is_generated_channel( channels[ index ] ) );
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

template < typename It,
           typename T = typename std::iterator_traits< It >::value_type >
bool
contains_only( It it1, It it2, T val )
{
    for ( ; it1 != it2; ++it1 )
    {
        if ( *it1 != val )
        {
            return false;
        }
    }
    return true;
}

bool
buffer_contains_only( const NDS::buffer& buf, float value )
{
    if ( buf.DataType( ) == NDS::channel::DATA_TYPE_FLOAT32 )
    {
        return contains_only(
            buf.cbegin< float >( ), buf.cend< float >( ), value );
    }
    return contains_only( buf.cbegin< std::int32_t >( ),
                          buf.cend< std::int32_t >( ),
                          static_cast< std::int32_t >( value ) );
}

template < typename It1, typename It2 >
bool
blended_compare( It1 begin1, It1 end1, It1 begin2, It2 begin3 )
{
    It1 cur1 = begin1;
    It1 cur2 = begin2;
    It2 cur3 = begin3;

    It1  cur = cur1;
    bool switched = false;

    for ( ; cur1 != end1; )
    {
        if ( *cur != *cur3 )
        {
            if ( switched )
            {
                return false;
            }
            switched = true;
            cur = cur2;

            if ( *cur != *cur3 )
            {
                return false;
            }
        }

        cur++;
        cur1++;
        cur2++;
        cur3++;
    }
    return true;
}

template < typename T >
void
test_channel_by_type( const NDS::buffer& buf, GeneratorPtr& gen, T tag )
{
    NDS::buffer::gps_second_type start = buf.Start( );
    NDS::buffer::gps_second_type end = buf.Stop( );

    for ( NDS::buffer::gps_second_type cur = start; cur != end; ++cur )
    {
        std::array< T, 16 > buf1{};
        std::array< T, 16 > buf2{};

        char* ptr1 = reinterpret_cast< char* >( buf1.data( ) );
        char* ptr2 = reinterpret_cast< char* >( buf2.data( ) );

        int nano = 0;
        int step = 1000000000 / 16;
        for ( int i = 0; i < 16; ++i )
        {
            ptr1 = gen->generate( cur - 1, nano, ptr1 );
            ptr2 = gen->generate( cur, nano, ptr2 );
        }

        const T* ptr3 = buf.cbegin< T >( ) + ( 16 * ( cur - start ) );
        if ( !blended_compare(
                 buf1.begin( ), buf1.end( ), buf2.begin( ), ptr3 ) )
        {
            std::cerr << "Unexpected data found on " << buf.Name( ) << "\n";
            std::cerr << "prev: ";
            for ( int j = 0; j < buf1.size( ); ++j )
            {
                std::cerr << buf1[ j ] << " ";
            }
            std::cerr << "\ncur: ";
            for ( int j = 0; j < buf2.size( ); ++j )
            {
                std::cerr << buf2[ j ] << " ";
            }
            std::cerr << "\nact: ";
            for ( int j = 0; j < buf2.size( ); ++j )
            {
                std::cerr << ptr3[ j ] << " ";
            }
            exit( 1 );
        }
    }
}

void
require_channel_type( daq_data_t req_type, GeneratorPtr gen )
{
    if ( static_cast< int >( req_type ) != gen->data_type( ) )
    {
        throw std::runtime_error( "Mismatch of channel and generator types" );
    }
}

void
test_channel( const NDS::buffer& buf, GeneratorPtr& gen )
{
    switch ( buf.DataType( ) )
    {
    case NDS::channel::DATA_TYPE_FLOAT64:
        require_channel_type( _64bit_double, gen );
        test_channel_by_type< double >( buf, gen, 0. );
        break;
    case NDS::channel::DATA_TYPE_FLOAT32:
        require_channel_type( _32bit_float, gen );
        test_channel_by_type< float >( buf, gen, 0.f );
        break;
    case NDS::channel::DATA_TYPE_INT32:
        require_channel_type( _32bit_integer, gen );
        test_channel_by_type< std::int32_t >( buf, gen, 0 );
        break;
    case NDS::channel::DATA_TYPE_INT16:
        require_channel_type( _16bit_integer, gen );
        test_channel_by_type< std::int16_t >( buf, gen, 0 );
        break;
    default:
        throw std::runtime_error( "Unsupported channel type" );
    }
}

void
test_channels( Config&                      cfg,
               NDS::parameters&             params,
               std::vector< GeneratorPtr >& generators )
{
    if ( cfg.channels.size( ) != generators.size( ) )
    {
        std::cerr << "Mismatch between the channels and the generators\n";
    }
    auto stream =
        NDS::iterate( params,
                      NDS::request_period( cfg.gps_start, cfg.gps_stop ),
                      cfg.channels );
    for ( const auto& bufs : stream )
    {
        for ( int i = 0; i < generators.size( ); ++i )
        {
            test_channel( bufs->at( i ), generators[ i ] );
        }
    }
}

/*!
 * @brief Query the NDS server for the EDC CONN/CNT/NOCON channels and make
 * sure they make sense over the configured time range.
 * @param params
 */
void
test_channel_counts( NDS::parameters&             params,
                     NDS::buffer::gps_second_type gps_start,
                     NDS::buffer::gps_second_type gps_stop )
{

    NDS::channels_type chans = NDS::find_channels(
        params,
        NDS::channel_predicate( "*EDCU_CHAN_CONN",
                                NDS::channel::CHANNEL_TYPE_ONLINE ) );
    if ( chans.size( ) != 1 )
    {
        std::cerr
            << "Unexpected channel count when looking up chan_conn channel\n";
        for ( const auto& entry : chans )
        {
            std::cerr << "\n" << entry.NameLong( );
        }
        std::cerr << "\n";
        exit( 1 );
    }
    std::string chan_conn = chans[ 0 ].Name( );

    chans = NDS::find_channels(
        params,
        NDS::channel_predicate( "*EDCU_CHAN_NOCON",
                                NDS::channel::CHANNEL_TYPE_ONLINE ) );
    if ( chans.size( ) != 1 )
    {
        std::cerr
            << "Unexpected channel count when looking up chan_nocon channel\n";
        exit( 1 );
    }
    std::string chan_nocon = chans[ 0 ].Name( );

    chans = NDS::find_channels(
        params,
        NDS::channel_predicate( "*EDCU_CHAN_CNT",
                                NDS::channel::CHANNEL_TYPE_ONLINE ) );
    if ( chans.size( ) != 1 )
    {
        std::cerr
            << "Unexpected channel count when looking up chan_cnt channel\n";
        exit( 1 );
    }
    std::string chan_cnt = chans[ 0 ].Name( );

    NDS::connection::channel_names_type channels;
    channels.push_back( chan_conn );
    channels.push_back( chan_nocon );
    channels.push_back( chan_cnt );
    std::cout << "Channels:\n\t";
    std::copy( channels.begin( ),
               channels.end( ),
               std::ostream_iterator< std::string >( std::cout, "\n\t" ) );
    std::cout << "\n";

    float expected_count = -1;

    auto stream = NDS::iterate(
        params, NDS::request_period( gps_start, gps_stop ), channels );
    for ( const auto& bufs : stream )
    {
        if ( bufs->size( ) != 3 )
        {
            std::cerr << "size is wrong " << bufs->size( ) << "\n";
        }
        std::cerr << "sample count = " << bufs->at( 2 ).Samples( ) << "\n";

        if ( expected_count < 0.0 )
        {
            if ( bufs->at( 2 ).DataType( ) == NDS::channel::DATA_TYPE_FLOAT32 )
            {
                expected_count = bufs->at( 2 ).at< float >( 0 );
            }
            else
            {
                expected_count = static_cast< float >(
                    bufs->at( 2 ).at< std::int32_t >( 0 ) );
            }
            std::cerr << "Expected count == " << expected_count << "\n";
        }

        if ( !buffer_contains_only( bufs->at( 2 ), expected_count ) )
        {
            std::cout << "The channel count changed during the test timespan, "
                         "it was not always "
                      << expected_count << "\n";
            exit( 1 );
        }

        if ( !buffer_contains_only( bufs->at( 1 ), 0.0f ) )
        {
            std::cout << "The channel noconn count changed during the test "
                         "timespan, it was not always 0.\n";
            exit( 1 );
        }

        if ( !buffer_contains_only( bufs->at( 0 ), expected_count ) )
        {
            std::cout << "The connected count changed during the test "
                         "timespan, it was not always "
                      << expected_count << "\n";
            exit( 1 );
        }
    }
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

    std::cout << "Checking data over " << cfg.gps_start << " - " << cfg.gps_stop
              << std::endl;

    std::vector< std::shared_ptr< NDS::buffers_type > > data_from_nds;
    NDS::parameters                                     params(
        cfg.hostname, cfg.port, NDS::connection::PROTOCOL_ONE );

    test_channel_counts( params, cfg.gps_start, cfg.gps_stop );

    test_channels( cfg, params, generators );

    return status;
}

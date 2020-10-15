//
// Created by jonathan.hanks on 10/12/20.
//
#include "dc_stats.hh"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include <boost/algorithm/string/predicate.hpp>

#include "checksum_crc32.hh"

#include "daqmap.h"
extern "C" {
#include "param.h"
}

namespace
{
    template < typename C, typename V >
    void
    fill( C& c, const V& val )
    {
        std::fill( std::begin( c ), std::end( c ), val );
    }

    template < typename C, typename F >
    void
    for_each( C& c, F&& f )
    {
        std::for_each( std::begin( c ), std::end( c ), std::forward< F >( f ) );
    }

    using Channel_cb_func =
        std::function< int( const char*, const CHAN_PARAM* ) >;

    std::string
    remove_spaces( const std::string& input )
    {
        std::ostringstream os{};
        std::copy_if( input.begin( ),
                      input.end( ),
                      std::ostream_iterator< char >( os ),
                      []( const char ch ) -> bool { return ch != ' '; } );
        return os.str( );
    }

    int
    channel_callback_trampoline( char*              channel_name,
                                 struct CHAN_PARAM* params,
                                 void*              user )
    {
        if ( !user || !channel_name || !params )
        {
            return 0;
        }
        auto cb = reinterpret_cast< Channel_cb_func* >( user );
        return ( *cb )( channel_name, params );
    }

    int
    recv_data_type_size( int dtype )
    {
        switch ( dtype )
        {
        case _16bit_integer: // 16 bit integer
            return 2;
        case _32bit_integer: // 32 bit integer
        case _32bit_float: // 32 bit float
        case _32bit_uint: // 32 bit unsigned int
            return 4;
        case _64bit_integer: // 64 bit integer
        case _64bit_double: // 64 bit double
            return 8;
        case _32bit_complex: // 32 bit complex
            return 4 * 2;
        default:
            return _undefined;
        }
    }

    void
    concat_into_vector( std::vector< char >& dest,
                        const std::string&   in1,
                        const std::string&   in2 )
    {
        dest.resize( in1.size( ) + in2.size( ) + 1 );
        std::copy( in1.begin( ), in1.end( ), dest.begin( ) );
        std::copy( in2.begin( ), in2.end( ), dest.begin( ) + in1.size( ) );
        dest.back( ) = '\0';
    }

} // namespace

void
DCUStats::setup_pv_names( )
{
    if ( !full_model_name.empty( ) )
    {
        concat_into_vector(
            expected_config_crc_name, full_model_name, "_CFG_CRC" );
        concat_into_vector( status_name, full_model_name, "_status" );
        concat_into_vector( crc_per_sec_name, full_model_name, "_CRC_CPS" );
        concat_into_vector( crc_sum_name, full_model_name, "_CRC_SUM" );
    }
}

DCStats::DCStats( std::vector< SimplePV >& pvs,
                  const std::string&       channel_list_path )
    : queue_{}
{
    if ( channel_list_path.empty( ) )
    {
        return;
    }
    std::vector< channel_t > channels{};
    int                      ini_file_dcu_id = -1;
    std::ifstream            f( channel_list_path );
    while ( !f.eof( ) && !f.bad( ) )
    {
        std::string line{};
        std::getline( f, line );
        auto stripped_line = remove_spaces( line );
        if ( stripped_line.empty( ) || stripped_line.front( ) == '#' )
        {
            continue;
        }
        bool test_point = boost::ends_with( stripped_line, ".par" );

        Channel_cb_func cb = [this, &channels, &ini_file_dcu_id](
                                 const char*       channel_name,
                                 const CHAN_PARAM* param ) -> int {
            return process_channel(
                channels, ini_file_dcu_id, channel_name, param );
        };
        ini_file_dcu_id = -1;
        unsigned long fileCrc{ 0 };
        if ( !parseConfigFile( const_cast< char* >( stripped_line.c_str( ) ),
                               &fileCrc,
                               channel_callback_trampoline,
                               static_cast< int >( test_point ),
                               nullptr,
                               reinterpret_cast< void* >( &cb ) ) )
        {
            throw std::runtime_error( "Error parsing the channel list" );
        }
        if ( ini_file_dcu_id > 0 && ini_file_dcu_id < DCU_COUNT )
        {
            dcu_status_[ ini_file_dcu_id ].expected_config_crc = fileCrc;
        }
        if ( boost::ends_with( stripped_line, ".ini" ) && ini_file_dcu_id > 0 &&
             ini_file_dcu_id < DCU_COUNT )
        {
            auto last_slash = stripped_line.find_last_of( '/' );
            auto start_point =
                ( last_slash == std::string::npos ? 1 : last_slash + 1 );
            std::string full_name( stripped_line.begin( ) + start_point,
                                   stripped_line.end( ) - 4 );

            dcu_status_[ ini_file_dcu_id ].full_model_name =
                std::move( full_name );
        }
    }
    checksum_crc32 crc{};
    std::for_each(
        channels.begin( ), channels.end( ), [&crc]( const channel_t& chan ) {
            hash_channel( crc, chan );
        } );
    channel_config_hash_ = static_cast< int >( crc.result( ) );
    std::cerr << "Loaded " << channels.size( ) << " channels" << std::endl;

    std::for_each(
        dcu_status_.begin( ), dcu_status_.end( ), [&pvs]( DCUStats& cur ) {
            cur.setup_pv_names( );
            if ( cur.full_model_name.empty( ) )
            {
                pvs.emplace_back( SimplePV{
                    cur.expected_config_crc_name.data( ),
                    SIMPLE_PV_INT,
                    reinterpret_cast< void* >( &cur.expected_config_crc ),
                    std::numeric_limits< int >::max( ),
                    std::numeric_limits< int >::min( ),
                    std::numeric_limits< int >::max( ),
                    std::numeric_limits< int >::min( ),
                } );
                pvs.emplace_back( SimplePV{
                    cur.status_name.data( ),
                    SIMPLE_PV_INT,
                    reinterpret_cast< void* >( &cur.status ),
                    1,
                    0,
                    1,
                    0,
                } );
                pvs.emplace_back( SimplePV{
                    cur.crc_per_sec_name.data( ),
                    SIMPLE_PV_INT,
                    reinterpret_cast< void* >( &cur.crc_per_sec ),
                    1,
                    0,
                    1,
                    0,
                } );
                pvs.emplace_back( SimplePV{
                    cur.crc_sum_name.data( ),
                    SIMPLE_PV_INT,
                    reinterpret_cast< void* >( &cur.crc_sum ),
                    1,
                    0,
                    1,
                    0,
                } );
            }
        } );
    pvs.emplace_back( SimplePV{
        "CHANNEL_LIST_CHECK_SUM",
        SIMPLE_PV_INT,
        reinterpret_cast< void* >( &channel_config_hash_ ),
        std::numeric_limits< int >::max( ),
        std::numeric_limits< int >::min( ),
        std::numeric_limits< int >::max( ),
        std::numeric_limits< int >::min( ),
    } );

    valid_ = true;
}

int
DCStats::process_channel( std::vector< channel_t >& channels,
                          int&                      ini_file_dcu_id,
                          const char*               channel_name,
                          const CHAN_PARAM*         params )
{
    if ( channels.size( ) >= MAX_CHANNELS )
    {
        std::cerr << "Too many channels.  The hard limit is " << MAX_CHANNELS
                  << "\n";
        return 0;
    }
    channels.emplace_back( );
    channel_t& cur = channels.back( );
    cur.seq_num = static_cast< int >( channels.size( ) - 1 );
    cur.id = nullptr;
    if ( params->dcuid >= DCU_COUNT || params->dcuid < 0 )
    {
        std::cerr << "Channel '" << channel_name << "' has bad DCU id "
                  << params->dcuid << std::endl;
        return 0;
    }
    ini_file_dcu_id = cur.dcu_id = params->dcuid;
    cur.ifoid = ( params->ifoid == 0 || params->ifoid == 1 ? 0 : 1 );

    cur.tp_node = params->rmid;

    strncpy( cur.name, channel_name, channel_t::channel_name_max_len - 1 );
    cur.name[ channel_t::channel_name_max_len - 1 ] = 0;
    cur.chNum = params->chnnum;
    cur.bps = recv_data_type_size( params->datatype );
    cur.data_type = static_cast< daq_data_t >( params->datatype );
    cur.sample_rate = params->datarate;

    cur.active = 0;
    if ( cur.sample_rate > 1 )
    {
        cur.active = params->acquire;
    }
    else if ( cur.sample_rate == 1 )
    {
        cur.sample_rate = 16;
    }
    cur.group_num = 0;

    auto is_gds_alias = false;
    auto is_gds_signal = false;
    if ( IS_TP_DCU( cur.dcu_id ) )
    {
        if ( params->testpoint )
        {
            is_gds_alias = true;
        }
        else
        {
            is_gds_signal = true;
        }
    }
    cur.trend = ( ( is_gds_alias || is_gds_signal ) ? 0 : cur.active );

    if ( is_gds_alias )
    {
        cur.active = 0;
    }

    cur.signal_gain = params->gain;
    cur.signal_slope = params->slope;
    cur.signal_offset = params->offset;
    strncpy(
        cur.signal_units, params->units, channel_t::engr_unit_max_len - 1 );
    cur.signal_units[ channel_t::engr_unit_max_len - 1 ] = 0;

    return 1;
}

static void
clear_processed( DCUStats& cur )
{
    cur.processed = false;
}

static void
clear_processed_this_cycle( DCUStats& cur )
{
    cur.processed_this_cycle = false;
}

static void
add_crc_err( DCUStats& cur )
{
    cur.crc_sum++;
    cur.crc_per_sec++;
}

static void
clear_entries( DCUStats& cur )
{
    cur.crc_per_sec = 0;
    cur.status = 0;
}

static void
dcu_skipped( DCUStats& cur )
{
    add_crc_err( cur );
    cur.status |= 0xbad;
}

static void
mark_dcu_if_skipped( DCUStats& cur )
{
    if ( !cur.processed )
    {
        dcu_skipped( cur );
    }
}

static void
mark_dcu_if_skipped_this_cycle( DCUStats& cur )
{
    if ( !cur.processed_this_cycle )
    {
        dcu_skipped( cur );
    }
}

static void
mark_bad_config( DCUStats& cur )
{
    cur.status |= 0x2000;
}

static void
mark_bad_data_crc( DCUStats& cur )
{
    add_crc_err( cur );
    cur.status |= 0x4000;
}

void
DCStats::run( simple_pv_handle epics_server )
{
    if ( !valid_ )
    {
        return;
    }

    checksum_crc32 crc;

    for_each( dcu_status_, clear_entries );
    for ( std::uint64_t cycles = 1;; ++cycles )
    {
        dc_queue::value_type entry{ queue_.pop( ) };

        for_each( dcu_status_, clear_processed_this_cycle );

        char* data = &( entry->dataBlock[ 0 ] );
        char* data_end = data + entry->header.fullDataBlockSize;

        if ( entry->header.dcuTotalModels <= 0 ||
             entry->header.dcuTotalModels > DCU_COUNT ||
             entry->header.fullDataBlockSize <= 0 )
        {
            for_each( dcu_status_, add_crc_err );
            simple_pv_server_update( epics_server );
            continue;
        }
        bool first{ true };
        for ( int i = 0; i < entry->header.dcuTotalModels; ++i )
        {
            auto& cur_header = entry->header.dcuheader[ i ];
            auto  dcu_id = cur_header.dcuId;
            if ( dcu_id <= 0 || dcu_id >= DCU_COUNT )
            {
                if ( cur_header.dataBlockSize > 0 ||
                     cur_header.tpBlockSize > 0 )
                {
                    // cannot safely skip this dcu
                    for_each( dcu_status_, mark_dcu_if_skipped_this_cycle );
                    break;
                }
                continue;
            }
            auto& cur_status = dcu_status_[ dcu_id ];
            cur_status.processed = true;
            cur_status.processed_this_cycle = true;
            if ( first )
            {
                gpstime_ = cur_header.timeSec;
                first = false;
            }
            if ( cur_header.fileCrc != cur_status.expected_config_crc )
            {
                mark_bad_config( cur_status );
            }
            auto total_data_size =
                cur_header.dataBlockSize + cur_header.tpBlockSize;
            if ( data + total_data_size > data_end )
            {
                // overflow
                for_each( dcu_status_, mark_dcu_if_skipped_this_cycle );
                break;
            }

            crc.reset( );
            crc.add( data, cur_header.dataBlockSize );
            if ( crc.result( ) != cur_header.dataCrc )
            {
                mark_bad_data_crc( cur_status );
            }
            data += total_data_size;
        }
        for_each( dcu_status_, mark_dcu_if_skipped );

        // we can either do this off of our counter or off of the
        // data cycle counter.
        if ( cycles % 16 == 0 )
        {
            ++uptime_;
            simple_pv_server_update( epics_server );
            for_each( dcu_status_, clear_entries );
        }
    }
}
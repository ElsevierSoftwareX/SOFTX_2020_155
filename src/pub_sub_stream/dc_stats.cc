//
// Created by jonathan.hanks on 10/12/20.
//
#include "dc_stats.hh"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>

#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>
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

    bool
    is_testpoint( const channel_t& chan )
    {
        return chan.tp_node >= 0;
    }

    std::uint64_t
    accumulate_data_rate( std::uint64_t total, const channel_t& cur_chan )
    {
        return total + ( is_testpoint( cur_chan ) ? 0 : cur_chan.bytes );
    }

    int
    count_data_channels( int cur_value, const channel_t& cur_chan )
    {
        return cur_value + ( is_testpoint( cur_chan ) ? 0 : 1 );
    }

} // namespace

void
DCUStats::setup_pv_names( )
{
    if ( !full_model_name.empty( ) )
    {
        concat_into_vector(
            expected_config_crc_name, full_model_name, "_CFG_CRC" );
        concat_into_vector( status_name, full_model_name, "_STATUS" );
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
    std::set< int > dcus{};

    int           ini_file_dcu_id = -1;
    std::ifstream f( channel_list_path );
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

        Channel_cb_func cb =
            [this, &ini_file_dcu_id]( const char*       channel_name,
                                      const CHAN_PARAM* param ) -> int {
            return process_channel( ini_file_dcu_id, channel_name, param );
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
            dcus.emplace( ini_file_dcu_id );
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
    for_each( channels_, [&crc]( const channel_t& chan ) {
        hash_channel( crc, chan );

        chan.bps;
    } );
    {
        data_rate_ = static_cast< int >(
                boost::accumulate(
                        channels_, std::int64_t{ 0 }, accumulate_data_rate ) /
                1024 );
    }
    total_chans_ = boost::accumulate( channels_, 0, count_data_channels );

    channel_config_hash_ = static_cast< int >( crc.result( ) );
    std::cerr << "Loaded " << channels_.size( ) << " tp + channels" << std::endl;

    for_each( dcu_status_, [&pvs]( DCUStats& cur ) {
        cur.setup_pv_names( );
        if ( !cur.full_model_name.empty( ) )
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
                -1,
                1,
                -1,
            } );
            pvs.emplace_back( SimplePV{
                cur.crc_per_sec_name.data( ),
                SIMPLE_PV_INT,
                reinterpret_cast< void* >( &cur.crc_per_sec ),
                1,
                -1,
                1,
                -1,
            } );
            pvs.emplace_back( SimplePV{
                cur.crc_sum_name.data( ),
                SIMPLE_PV_INT,
                reinterpret_cast< void* >( &cur.crc_sum ),
                1,
                -1,
                1,
                -1,
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
    pvs.emplace_back( SimplePV{
        "PRDCR_NOT_STALLED",
        SIMPLE_PV_INT,
        reinterpret_cast< void* >( &not_stalled_ ),
        2,
        0,
        2,
        0,
    } );
    pvs.emplace_back( SimplePV{
        "UPTIME_SECONDS",
        SIMPLE_PV_INT,
        reinterpret_cast< void* >( &uptime_ ),
        std::numeric_limits< int >::max( ),
        0,
        std::numeric_limits< int >::max( ),
        0,
    } );
    pvs.emplace_back( SimplePV{
        "GPS",
        SIMPLE_PV_INT,
        reinterpret_cast< void* >( &gpstime_ ),
        std::numeric_limits< int >::max( ),
        0,
        std::numeric_limits< int >::max( ),
        0,
    } );
    pvs.emplace_back( SimplePV{
        "PRDCR_UNIQUE_DCU_REPORTED_PER_S",
        SIMPLE_PV_INT,
        reinterpret_cast< void* >( &unique_dcus_per_sec_ ),
        static_cast< int >( dcus.size( ) ) + 1,
        static_cast< int >( dcus.size( ) ) - 1,
        static_cast< int >( dcus.size( ) ) + 1,
        static_cast< int >( dcus.size( ) ) - 1,
    } );
    pvs.emplace_back( SimplePV{
        "DATA_RATE",
        SIMPLE_PV_INT,
        reinterpret_cast< void* >( &data_rate_ ),
        std::numeric_limits< int >::max( ),
        -1,
        std::numeric_limits< int >::max( ),
        -1,
    } );
    pvs.emplace_back( SimplePV{
        "TOTAL_CHANS",
        SIMPLE_PV_INT,
        reinterpret_cast< void* >( &total_chans_ ),
        total_chans_ + 1,
        total_chans_ - 1,
        total_chans_ + 1,
        total_chans_ - 1,
    } );
    pvs.emplace_back( SimplePV{
        "PRDCR_OPEN_TP_COUNT",
        SIMPLE_PV_INT,
        reinterpret_cast< void* >( &open_tp_count_ ),
        256,
        -1,
        256,
        -1,
    } );
    pvs.emplace_back( SimplePV{
        "PRDCR_TP_DATA_RATE_KB_PER_S",
        SIMPLE_PV_INT,
        reinterpret_cast< void* >( &tp_data_kb_per_s_ ),
        std::numeric_limits< int >::max( ),
        -1,
        std::numeric_limits< int >::max( ),
        -1,
    } );
    pvs.emplace_back( SimplePV{
        "PRDCR_MODEL_DATA_RATE_KB_PER_S",
        SIMPLE_PV_INT,
        reinterpret_cast< void* >( &model_data_kb_per_s_ ),
        std::numeric_limits< int >::max( ),
        -1,
        std::numeric_limits< int >::max( ),
        -1,
    } );
    pvs.emplace_back( SimplePV{
        "PRDCR_TOTAL_DATA_RATE_KB_PER_S",
        SIMPLE_PV_INT,
        reinterpret_cast< void* >( &total_data_kb_per_s_ ),
        std::numeric_limits< int >::max( ),
        -1,
        std::numeric_limits< int >::max( ),
        -1,
    } );

    valid_ = true;
}

DCStats::~DCStats( )
{
    if ( !request_stop_ )
    {
        stop( );
    }
}

/*!
 * @brief The channel callback for the ini file parser.  This is called for each
 * channel found.
 * @param channels The current channel list
 * @param ini_file_dcu_id Where to put the dcu id
 * @param channel_name the name of the current channel
 * @param params attributes of the current channel
 * @return 0 on error, else 1
 */
int
DCStats::process_channel( int&              ini_file_dcu_id,
                          const char*       channel_name,
                          const CHAN_PARAM* params )
{
    if ( channels_.size( ) >= MAX_CHANNELS )
    {
        std::cerr << "Too many channels.  The hard limit is " << MAX_CHANNELS
                  << "\n";
        return 0;
    }
    channels_.emplace_back( );
    channel_t& cur = channels_.back( );
    cur.seq_num = static_cast< int >( channels_.size( ) - 1 );
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
    cur.bytes = static_cast< int >( cur.bps * cur.sample_rate );

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
clear_seen_last_cycle( DCUStats& cur )
{
    cur.seen_last_cycle = false;
}

static void
clear_seen_last_cycle_if_skipped( DCUStats& cur )
{
    if ( !cur.processed_this_cycle )
    {
        clear_seen_last_cycle( cur );
    }
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
    cur.processed = false;
}

static void
clear_crc( DCUStats& cur )
{
    cur.crc_per_sec = 0;
    cur.crc_sum = 0;
}

static void
dcu_skipped( DCUStats& cur )
{
    if ( cur.seen_last_cycle )
    {
        add_crc_err( cur );
    }
    cur.status |= 0xbad;
}

// static void
// mark_dcu_if_skipped( DCUStats& cur )
//{
//    if ( !cur.processed )
//    {
//        dcu_skipped( cur );
//    }
//}

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

static void
mark_as_seen( DCUStats& cur )
{
    cur.seen_last_cycle = true;
}

static bool
entry_was_processed( DCUStats& cur )
{
    return cur.processed;
}

dc_queue::value_type
DCStats::get_message( simple_pv_handle epics_server )
{
    boost::optional< dc_queue::value_type > val{ queue_.pop(
        std::chrono::seconds( 2 ) ) };
    if ( val )
    {
        return std::move( val.get( ) );
    }
    not_stalled_ = 0;
    simple_pv_server_update( epics_server );
    return queue_.pop( );
}

void
DCStats::run( simple_pv_handle epics_server )
{
    if ( !valid_ )
    {
        return;
    }
    checksum_crc32 crc;

    std::uint64_t tp_data{ 0 };
    std::uint64_t model_data{ 0 };
    std::uint64_t total_data{ 0 };
    for_each( dcu_status_, clear_seen_last_cycle );
    for_each( dcu_status_, clear_entries );
    for ( std::uint64_t cycles = 1;; ++cycles )
    {

        dc_queue::value_type entry{ get_message( epics_server ) };

        if ( request_stop_ )
        {
            return;
        }

        unsigned int tp_count{ 0 };
        not_stalled_ = 1;

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
        total_data += entry->header.fullDataBlockSize;
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
            tp_count += cur_header.tpCount;
            tp_data += cur_header.tpBlockSize;
            model_data += cur_header.dataBlockSize;

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

            mark_as_seen( cur_status );

            crc.reset( );
            crc.add( data, cur_header.dataBlockSize );
            if ( crc.result( ) != cur_header.dataCrc )
            {
                mark_bad_data_crc( cur_status );
            }
            data += total_data_size;
        }
        for_each( dcu_status_, mark_dcu_if_skipped_this_cycle );
        for_each( dcu_status_, clear_seen_last_cycle_if_skipped );
        // we can either do this off of our counter or off of the
        // data cycle counter.
        if ( cycles % 16 == 0 )
        {
            ++uptime_;
            tp_data_kb_per_s_ = static_cast< unsigned int >( tp_data / 1024 );
            model_data_kb_per_s_ =
                static_cast< unsigned int >( model_data / 1024 );
            total_data_kb_per_s_ =
                static_cast< unsigned int >( total_data / 1024 );

            open_tp_count_ = static_cast< int >( tp_count );
            unique_dcus_per_sec_ =
                boost::count_if( dcu_status_, entry_was_processed );
            simple_pv_server_update( epics_server );
            for_each( dcu_status_, clear_entries );

            tp_data = 0;
            model_data = 0;
            total_data = 0;

            if ( request_clear_crc_ )
            {
                for_each( dcu_status_, clear_crc );
                request_clear_crc_ = false;
            }
        }
    }
}
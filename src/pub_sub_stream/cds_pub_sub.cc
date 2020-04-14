//
// @file sub_recv_buffered.c
// @brief  DAQ data concentrator code. Receives data via subscription and writes
// to local memory.
//

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "../drv/crc.c"
#include <time.h>
#include "../include/daqmap.h"
#include "../include/daq_core.h"

#include <boost/algorithm/string.hpp>

#include <cds-pubsub/pub.hh>
#include <cds-pubsub/sub.hh>
#include <sub_plugin_rmipc.hh>

#include "args.h"

#include "simple_pv.h"
#include "recv_buffer.hh"
#include "make_unique.hh"
#include "arena.hh"

#define __CDECL

#define MIN_DELAY_MS 5
#define MAX_DELAY_MS 40

#define MAX_FE_COMPUTERS 32

int do_verbose = 0;

struct thread_info
{
    int         index{};
    std::string conn_string{};
    std::thread thread{};
};
struct thread_mon_info
{
    int   index;
    void* ctx;
};

// daq_multi_dcu_data_t mxDataBlockSingle[ MAX_FE_COMPUTERS ];
receive_buffer< 4 > circular_buffer;
// int64_t              dataRecvTime[ MAX_FE_COMPUTERS ];
// const int            mc_header_size = sizeof( daq_multi_cycle_header_t );
int                 stop_working_threads = 0;
int                 start_acq = 0;
static volatile int keepRunning = 1;
// int                  thread_cycle[ MAX_FE_COMPUTERS ];
// int                  thread_timestamp[ MAX_FE_COMPUTERS ];
int rcv_errors = 0;
// int                  dataRdy[ MAX_FE_COMPUTERS ];

/*!
 * @brief RAII class to ensure that worker threads get stopped
 */
class thread_stopper
{
public:
    ~thread_stopper( )
    {
        stop_working_threads = 1;
        sleep( 5 );
        fprintf( stderr, "stopping threads\n" );
    }
};

class SubDebug : public pub_sub::SubDebugNotices
{
public:
    SubDebug( )
        : received_messages{ 0 }, dropped_messages{ 0 },
          retransmit_requests{ 0 }, terminated_connections{ 0 },
          new_connections{ 0 }, renewed_connections{ 0 }, retransmit_size{},
          message_spread{}
    {
    }
    ~SubDebug( ) override = default;

    void
    message_received( const pub_sub::SubId sub_id,
                      const pub_sub::KeyType,
                      std::size_t               size,
                      std::chrono::milliseconds spread ) override
    {
        int index = static_cast< int >( spread.count( ) / 2 );
        index = std::min( index, (int)message_spread.size( ) - 1 );
        message_spread[ index ]++;
        ++received_messages;
    }

    void
    retransmit_request_made( const pub_sub::SubId   sub_id,
                             const pub_sub::KeyType key,
                             int                    packet_count ) override
    {
        ++retransmit_requests;
        int index = packet_count / 5;
        index = std::min( index, (int)retransmit_size.size( ) - 1 );
        retransmit_size[ index ]++;
    }

    void
    message_dropped( const pub_sub::SubId   sub_id,
                     const pub_sub::KeyType key ) override
    {
        ++dropped_messages;
    }

    void
    connection_terminated( const pub_sub::SubId   sub_id,
                           const pub_sub::KeyType key ) override
    {
        if ( key != pub_sub::NEXT_PUB_MSG( ) )
        {
            terminated_connections++;
        }
    }

    void
    connection_started( const pub_sub::SubId   sub_id,
                        const pub_sub::KeyType key ) override
    {
        if ( key == pub_sub::NEXT_PUB_MSG( ) )
        {
            new_connections++;
        }
        else
        {
            renewed_connections++;
        }
    }

    void
    clear( )
    {
        // received_messages = 0;
        // dropped_messages = 0;
        // retransmit_requests = 0;
        std::fill( retransmit_size.begin( ), retransmit_size.end( ), 0 );
        std::fill( message_spread.begin( ), message_spread.end( ), 0 );
    }

    int                   received_messages;
    int                   dropped_messages;
    int                   retransmit_requests;
    int                   terminated_connections;
    int                   new_connections;
    int                   renewed_connections;
    std::array< int, 10 > retransmit_size;
    std::array< int, 10 > message_spread;
};

class SimplePVCloser
{
public:
    explicit SimplePVCloser( simple_pv_handle handle ) : handle_{ handle }
    {
    }
    ~SimplePVCloser( )
    {
        simple_pv_server_destroy( &handle_ );
    }

private:
    simple_pv_handle handle_;
};

template < typename It >
void
dump_headers( It cur, It end, std::ostream& os )
{
    for ( ; cur != end; ++cur )
    {
        buffer_headers& cur_buf = *cur;
        int             dcus = cur_buf.dcu_count;
        os << "Buffer latest: " << cur_buf.latest.gps( ) << ":"
           << cur_buf.latest.cycle( ) << "\n";
        os << "dcu_count " << dcus << "\n";
        for ( int i = 0; i < dcus; ++i )
        {
            os << "dcuid " << cur_buf.headers[ i ].dcuId << " ingested at "
               << cur_buf.time_ingested[ i ] << "\n";
        }
        os << std::endl;
    }
}

template < typename It >
std::pair< int, int >
quick_stats( It cur, It end, std::ostream& os, gps_key now_key )
{
    std::pair< int, int > dcu_stats =
        std::make_pair( DAQ_TRANSIT_MAX_DCU + 1, 0 );

    for ( ; cur != end; ++cur )
    {
        buffer_headers& cur_buf = *cur;

        int64_t min_time = 0x0fffffffffffffff;
        int64_t max_time = 0;
        int64_t delta = 0;

        int dcus = cur_buf.dcu_count;
        for ( int i = 0; i < dcus; ++i )
        {
            min_time = std::min( min_time, cur_buf.time_ingested[ i ] );
            max_time = std::max( max_time, cur_buf.time_ingested[ i ] );
            delta = max_time - min_time;
        }

        int key_delta = now_key.key - cur_buf.latest.key;
        if ( key_delta >= 0 )
        {
            dcu_stats.first = std::min( dcu_stats.first, dcus );
            dcu_stats.second = std::max( dcu_stats.second, dcus );
        }
        os << "[" << std::setw( 2 ) << key_delta << " dcus " << std::setw( 3 )
           << dcus << " spread " << std::setw( 3 ) << delta << "ms] ";
    }
    os << std::endl;
    return dcu_stats;
}

/*!
 * @brief The data_recorder is used in conjuction with a recieve buffer
 * to move data from the receive_buffer and publish it.
 */
class data_recorder
{
    data_recorder( const data_recorder& other );
    data_recorder& operator=( const data_recorder& other );

public:
    /*!
     * @Initialize the data_recorder, opening the output shared memory buffer
     * @param shmem_name Name of the shared memory buffer to output to
     * @param shmem_max_size_mb Size of the buffer in MB
     */
    explicit data_recorder(
        const std::vector< std::string >& publisher_strings )
        : publisher_{}, memory_arena_( 5 )
    {
        std::for_each( publisher_strings.begin( ),
                       publisher_strings.end( ),
                       [this]( const std::string& pub_string ) {
                           publisher_.add_destination( pub_string );
                       } );
    }

    /*!
     * @brief copy the given daq_dc_data_t into the output buffer
     * @param input_data Data to copy
     */
    void
    operator( )( const daq_dc_data_t& input_data )
    {
        pub_sub::DataPtr msg_buffer = memory_arena_.get( );
        size_t           data_size =
            input_data.header.fullDataBlockSize + sizeof( input_data.header );
        if ( data_size > sizeof( daq_dc_data_t ) )
        {
            fprintf( stderr,
                     "Overflow of the output buffer, requested %d bytes for a "
                     "%d byte destination",
                     (int)data_size,
                     (int)sizeof( daq_dc_data_t ) );
            throw std::runtime_error( "Overflow of the output buffers" );
        }

        memcpy( msg_buffer.get( ), &input_data, data_size );
        pub_sub::KeyType key =
            ( input_data.header.dcuheader[ 0 ].timeSec << 8 ) +
            input_data.header.dcuheader[ 0 ].cycle;
        publisher_.publish(
            key, pub_sub::Message( std::move( msg_buffer ), data_size ) );
    }

private:
    pub_sub::Publisher publisher_;
    Arena              memory_arena_;
};

// *************************************************************************
// Timing Diagnostic Routine
// *************************************************************************
static int64_t
s_clock( void )
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return ( int64_t )( tv.tv_sec * 1000 + tv.tv_usec / 1000 );
}

std::vector< std::string >
parse_pub_sub_strings( const char* pub_subs_file, const char* pub_subs )
{
    std::vector< std::string > conn_strings{};
    if ( pub_subs_file )
    {
        std::ifstream input( pub_subs_file );
        std::string   line;
        while ( std::getline( input, line, '\n' ) )
        {
            if ( line.empty( ) || line[ 0 ] == '#' )
            {
                continue;
            }
            conn_strings.emplace_back( line );
        }
    }
    else
    {
        boost::algorithm::split(
            conn_strings, pub_subs, boost::algorithm::is_space( ) );
    }
    return conn_strings;
}

// *************************************************************************
// Catch Control C to end cod in controlled manner
// *************************************************************************
void
intHandler( int dummy )
{
    keepRunning = 0;
}

void
sigpipeHandler( int dummy )
{
}

// *************************************************************************
// Main Process
// *************************************************************************
int
main( int argc, char** argv )
{
    pthread_t thread_id[ MAX_FE_COMPUTERS ];
    // int nsys = MAX_FE_COMPUTERS; // The number of mapped shared memories
    // (number
    // of data sources)
    int c;
    int ii; // Loop counter
    int delay_ms = 10;
    int delay_cycles = 0;

    extern char* optarg; // Needed to get arguments to program

    // Declare shared memory data variables
    daq_multi_cycle_header_t*  ifo_header;
    char*                      ifo;
    char*                      ifo_data;
    int                        cycle_data_size;
    daq_multi_dcu_data_t*      ifoDataBlock;
    char*                      nextData;
    int                        buffer_cycles = 0;
    const char*                logfname = nullptr;
    const char*                subs = nullptr;
    const char*                subs_file = nullptr;
    const char*                pubs = nullptr;
    const char*                pubs_file = nullptr;
    const char*                epics_prefix = nullptr;
    std::vector< std::string > subscription_strings{};
    std::vector< std::string > publishing_strings{};
    int                        thread_per_sub = 0;
    int                        dcus_received = 0;
    int                        max_dcus_received = 0;
    int                        min_dcus_received = 256;
    int                        mean_dcus_received = 0;
    int                        reporting_max_dcus_received = 0;
    int                        reporting_min_dcus_received = 0;
    int                        reporting_mean_dcus_recieved = 0;
    int                        spread_ms = 0;
    int                        max_spread_ms = 0;
    int                        min_spread_ms = 100000;
    int                        mean_spread_ms = 0;
    int                        latching_max_spread_ms = 0;
    int                        reporting_max_spread_ms = 0;
    int                        reporting_min_spread_ms = 0;
    int                        reporting_mean_spread_ms = 0;
    int                        total_spread_ms = 0;
    int                        max_total_spread_ms = 0;
    int                        min_total_spread_ms = 100000;
    int                        mean_total_spread_ms = 0;
    int                        latching_total_max_spread_ms = 0;
    int                        reporting_total_max_spread_ms = 0;
    int                        reporting_total_min_spread_ms = 0;
    int                        reporting_total_mean_spread_ms = 0;
    int                        mean_samples = 0;
    int                        late_messages_last_sec = 0;
    int                        very_late_messages_last_sec = 0;
    int                        discarded_messages_last_sec = 0;
    int                        late_messages = 0;
    int                        very_late_messages = 0;
    int                        discarded_messages = 0;

    args_handle arg_parser =
        args_create_parser( "Receive data from a LIGO simple publisher" );
    if ( !arg_parser )
    {
        return -1;
    }
    args_add_string_ptr( arg_parser,
                         'i',
                         "input",
                         "",
                         "Space separated list of input/subscription strings",
                         &subs,
                         "" );
    args_add_string_ptr( arg_parser,
                         'I',
                         "input-list",
                         "file",
                         "File with 1 input/subscription string per line, if "
                         "present, this superceeds -i",
                         &subs_file,
                         nullptr );
    args_add_string_ptr( arg_parser,
                         'o',
                         "output",
                         "",
                         "Space seperated list of output/publishing strings",
                         &pubs,
                         "" );
    args_add_string_ptr( arg_parser,
                         'O',
                         "output-list",
                         "file",
                         "File with 1 output/publishing strings per line, if "
                         "present, present, this superceeds -o",
                         &pubs_file,
                         "" );
    args_add_string_ptr( arg_parser,
                         'l',
                         ARGS_NO_LONG,
                         "filename",
                         "Log file name",
                         &logfname,
                         nullptr );
    args_add_flag(
        arg_parser, 'v', ARGS_NO_LONG, "Verbose output", &do_verbose );
    args_add_int( arg_parser,
                  'd',
                  ARGS_NO_LONG,
                  "5-40",
                  "The number of ms to delay data",
                  &delay_ms,
                  5 );
    args_add_int( arg_parser,
                  'B',
                  ARGS_NO_LONG,
                  "cycles",
                  "The number of cycles to delay data",
                  &buffer_cycles,
                  0 );
    args_add_flag(
        arg_parser,
        ARGS_NO_SHORT,
        "multi-thread",
        "Set if an explicit thread should be created for each subscription",
        &thread_per_sub );
    args_add_string_ptr( arg_parser,
                         'p',
                         ARGS_NO_LONG,
                         "",
                         "The EPICS variable prefix to use",
                         &epics_prefix,
                         nullptr );

    if ( args_parse( arg_parser, argc, argv ) < 0 )
    {
        return -1;
    }

    subscription_strings = parse_pub_sub_strings( subs_file, subs );
    publishing_strings = parse_pub_sub_strings( pubs_file, pubs );

    if ( logfname != nullptr )
    {
        if ( !freopen( optarg, "w", stdout ) )
        {
            perror( "freopen" );
            exit( 1 );
        }
        setvbuf( stdout, NULL, _IOLBF, 0 );
        stderr = stdout;
    }
    if ( delay_ms < MIN_DELAY_MS || delay_ms > MAX_DELAY_MS )
    {
        fprintf( stderr, "The delay factor must be between 5ms and 40ms\n" );
        return -1;
    }
    buffer_cycles = std::max( buffer_cycles, 0 );
    buffer_cycles = std::min( buffer_cycles, (int)circular_buffer.size( ) - 2 );
    if ( buffer_cycles > 0 )
    {
        delay_ms = 0;
    }

    fprintf( stderr, "Delaying output by %dms to wait for data\n", delay_ms );
    fprintf( stderr,
             "Delaying output by %d cycles to wait for data\n",
             buffer_cycles );
    delay_cycles = delay_ms * 1000;

    SubDebug                debug;
    simple_pv_handle        epics_server = nullptr;
    std::vector< SimplePV > pvs;
    if ( epics_prefix )
    {
        pvs.emplace_back( SimplePV{ "DCUS_RECEIVED",
                                    SIMPLE_PV_INT,
                                    &dcus_received,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "MAX_DCUS_RECEIVED",
                                    SIMPLE_PV_INT,
                                    &reporting_max_dcus_received,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "MIN_DCUS_RECEIVED",
                                    SIMPLE_PV_INT,
                                    &reporting_min_dcus_received,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "MEAN_DCUS_RECEIVED",
                                    SIMPLE_PV_INT,
                                    &reporting_mean_dcus_recieved,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{
            "SPREAD_MS", SIMPLE_PV_INT, &spread_ms, 1000, 0, 1000, 0 } );
        pvs.emplace_back( SimplePV{ "MAX_SPREAD_MS_LATCHED",
                                    SIMPLE_PV_INT,
                                    &latching_max_spread_ms,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "MAX_SPREAD_MS",
                                    SIMPLE_PV_INT,
                                    &reporting_max_spread_ms,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "MIN_SPREAD_MS",
                                    SIMPLE_PV_INT,
                                    &reporting_min_spread_ms,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "MEAN_SPREAD_MS",
                                    SIMPLE_PV_INT,
                                    &reporting_mean_spread_ms,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "TOTAL_SPREAD_MS",
                                    SIMPLE_PV_INT,
                                    &total_spread_ms,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "MAX_TOTAL_SPREAD_MS_LATCHED",
                                    SIMPLE_PV_INT,
                                    &latching_total_max_spread_ms,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "MAX_TOTAL_SPREAD_MS",
                                    SIMPLE_PV_INT,
                                    &reporting_total_max_spread_ms,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "MIN_TOTAL_SPREAD_MS",
                                    SIMPLE_PV_INT,
                                    &reporting_total_min_spread_ms,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "MEAN_TOTAL_SPREAD_MS",
                                    SIMPLE_PV_INT,
                                    &reporting_total_mean_spread_ms,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "LATE_MESSAGES_LAST_SEC",
                                    SIMPLE_PV_INT,
                                    &late_messages_last_sec,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "VERY_LATE_MESSAGES_LAST_SEC",
                                    SIMPLE_PV_INT,
                                    &very_late_messages_last_sec,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "DROPPED_MESSAGES_LAST_SEC",
                                    SIMPLE_PV_INT,
                                    &discarded_messages_last_sec,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "LATE_MESSAGES",
                                    SIMPLE_PV_INT,
                                    &late_messages,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "VERY_LATE_MESSAGES",
                                    SIMPLE_PV_INT,
                                    &very_late_messages,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "DROPPED_MESSAGES",
                                    SIMPLE_PV_INT,
                                    &discarded_messages,
                                    1000,
                                    0,
                                    1000,
                                    0 } );
        pvs.emplace_back( SimplePV{ "RECEIVED_MSG_COUNT",
                                    SIMPLE_PV_INT,
                                    &debug.received_messages,
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "DROPPED_UDP_MSG_COUNT",
                                    SIMPLE_PV_INT,
                                    &debug.dropped_messages,
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "RETRANSMIT_REQ",
                                    SIMPLE_PV_INT,
                                    &debug.retransmit_requests,
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "RETRANSMIT_5_PKT",
                                    SIMPLE_PV_INT,
                                    &debug.retransmit_size[ 0 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "RETRANSMIT_10_PKT",
                                    SIMPLE_PV_INT,
                                    &debug.retransmit_size[ 1 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "RETRANSMIT_15_PKT",
                                    SIMPLE_PV_INT,
                                    &debug.retransmit_size[ 2 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "RETRANSMIT_20_PKT",
                                    SIMPLE_PV_INT,
                                    &debug.retransmit_size[ 3 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "RETRANSMIT_25_PKT",
                                    SIMPLE_PV_INT,
                                    &debug.retransmit_size[ 4 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "RETRANSMIT_30_PKT",
                                    SIMPLE_PV_INT,
                                    &debug.retransmit_size[ 5 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "RETRANSMIT_35_PKT",
                                    SIMPLE_PV_INT,
                                    &debug.retransmit_size[ 6 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "RETRANSMIT_40_PKT",
                                    SIMPLE_PV_INT,
                                    &debug.retransmit_size[ 7 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "RETRANSMIT_45_PKT",
                                    SIMPLE_PV_INT,
                                    &debug.retransmit_size[ 8 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "RETRANSMIT_50_PKT",
                                    SIMPLE_PV_INT,
                                    &debug.retransmit_size[ 9 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "MSG_DURATION_0_MS",
                                    SIMPLE_PV_INT,
                                    &debug.message_spread[ 0 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "MSG_DURATION_2_MS",
                                    SIMPLE_PV_INT,
                                    &debug.message_spread[ 1 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "MSG_DURATION_4_MS",
                                    SIMPLE_PV_INT,
                                    &debug.message_spread[ 2 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "MSG_DURATION_6_MS",
                                    SIMPLE_PV_INT,
                                    &debug.message_spread[ 3 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "MSG_DURATION_8_MS",
                                    SIMPLE_PV_INT,
                                    &debug.message_spread[ 4 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "MSG_DURATION_10_MS",
                                    SIMPLE_PV_INT,
                                    &debug.message_spread[ 5 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "MSG_DURATION_12_MS",
                                    SIMPLE_PV_INT,
                                    &debug.message_spread[ 6 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "MSG_DURATION_14_MS",
                                    SIMPLE_PV_INT,
                                    &debug.message_spread[ 7 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "MSG_DURATION_16_MS",
                                    SIMPLE_PV_INT,
                                    &debug.message_spread[ 8 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        pvs.emplace_back( SimplePV{ "MSG_DURATION_18_MS",
                                    SIMPLE_PV_INT,
                                    &debug.message_spread[ 9 ],
                                    1000,
                                    -1,
                                    1000,
                                    -1 } );
        epics_server =
            simple_pv_server_create( epics_prefix, pvs.data( ), pvs.size( ) );
    }
    SimplePVCloser epics_server_teardown_( epics_server );

    // set up to catch Control C
    signal( SIGINT, intHandler );
    // setup to ignore sig pipe
    // signal( SIGPIPE, sigpipeHandler );

    fprintf( stderr, "Num of sys = %d\n", (int)subscription_strings.size( ) );

    data_recorder recorder( publishing_strings );

    std::vector< std::unique_ptr< pub_sub::Subscriber > > subscribers{};
    subscribers.reserve( subscription_strings.size( ) );
    if ( !thread_per_sub )
    {
        subscribers.emplace_back( make_unique_ptr< pub_sub::Subscriber >( ) );
    }
    for ( const auto& conn_str : subscription_strings )
    {
        if ( thread_per_sub )
        {
            subscribers.emplace_back(
                make_unique_ptr< pub_sub::Subscriber >( ) );
        }
        subscribers.front( )->SetupDebugHooks( &debug );
        fprintf( stderr, "Beginning subscription on %s\n", conn_str.c_str( ) );
        subscribers.front( )->subscribe(
            conn_str, []( pub_sub::SubMessage sub_msg ) {
                circular_buffer.ingest(
                    *( (daq_multi_dcu_data_t*)sub_msg.data( ) ) );
            } );
    }

    int nextCycle = 0;
    start_acq = 1;
    static const int header_size = sizeof( daq_multi_dcu_header_t );

    int     min_cycle_time = 1 << 30;
    int     max_cycle_time = 0;
    int     mean_cycle_time = 0;
    int     uptime = 0;
    int     missed_flag = 0;
    int     missed_nsys[ MAX_FE_COMPUTERS ];
    int64_t recv_time[ MAX_FE_COMPUTERS ];
    int64_t min_recv_time = 0;
    int     recv_buckets[ ( MAX_DELAY_MS / 5 ) + 2 ];

    gps_key last_received;

    missed_flag = 1;
    memset( &missed_nsys[ 0 ], 0, sizeof( missed_nsys ) );
    memset( recv_buckets, 0, sizeof( recv_buckets ) );

    std::array< buffer_headers, 4 > headers;

    // ensure the threads get stopped before exit.
    thread_stopper clean_threads_;
    do
    {

        gps_key latest_in_buffer;
        do
        {
            usleep( 2000 );
            latest_in_buffer = circular_buffer.latest( );
        } while ( latest_in_buffer == last_received );

        usleep( delay_cycles );

        gps_key process_at = latest_in_buffer;
        process_at.key -= buffer_cycles;

        circular_buffer.process_slice_at( process_at, recorder );
        circular_buffer.copy_headers( headers.begin( ) );

        auto pub_index = receive_buffer< 4 >::cycle_to_index( process_at.key );
        dcus_received = 0;
        spread_ms = 10000;
        if ( headers[ pub_index ].latest == process_at )
        {
            buffer_headers& cur_header = headers[ pub_index ];
            dcus_received = static_cast< int >( cur_header.dcu_count );
            int64_t smallest = std::numeric_limits< int64_t >::max( );
            int64_t largest = 0;
            for ( int i = 0; i < dcus_received; ++i )
            {
                auto cur_time = cur_header.time_ingested[ i ];
                smallest = std::min( smallest, cur_time );
                largest = std::max( largest, cur_time );
            }
            spread_ms = static_cast< int >( largest - smallest );
        }
        min_dcus_received = std::min( min_dcus_received, dcus_received );
        max_dcus_received = std::max( max_dcus_received, dcus_received );
        min_spread_ms = std::min( min_spread_ms, spread_ms );
        max_spread_ms = std::max( max_spread_ms, spread_ms );
        latching_max_spread_ms =
            std::max( max_spread_ms, latching_max_spread_ms );
        total_spread_ms = circular_buffer.get_and_clear_cycle_message_span( );
        min_total_spread_ms = std::min( min_total_spread_ms, total_spread_ms );
        max_total_spread_ms = std::max( max_total_spread_ms, total_spread_ms );
        latching_total_max_spread_ms =
            std::max( max_total_spread_ms, latching_total_max_spread_ms );
        mean_dcus_received += dcus_received;
        mean_spread_ms += spread_ms;
        mean_total_spread_ms += total_spread_ms;
        ++mean_samples;

        if ( do_verbose )
        {
            auto dcu_stats = quick_stats(
                headers.begin( ), headers.end( ), std::cout, process_at );
            if ( dcu_stats.second < max_dcus_received )
            {
                dump_headers( headers.begin( ), headers.end( ), std::cout );
            }
            max_dcus_received = dcu_stats.second;
        }
        if ( latest_in_buffer.cycle( ) == 0 )
        {
            reporting_max_dcus_received = max_dcus_received;
            reporting_min_dcus_received = min_dcus_received;
            reporting_max_spread_ms = max_spread_ms;
            reporting_min_spread_ms = min_spread_ms;
            reporting_total_max_spread_ms = max_total_spread_ms;
            reporting_total_min_spread_ms = min_total_spread_ms;
            reporting_mean_dcus_recieved = 0;
            reporting_mean_spread_ms = 0;
            reporting_total_mean_spread_ms = 0;
            if ( mean_samples > 0 )
            {
                reporting_mean_dcus_recieved =
                    mean_dcus_received / mean_samples;
                reporting_mean_spread_ms = mean_spread_ms / mean_samples;
                reporting_total_mean_spread_ms =
                    mean_total_spread_ms / mean_samples;
            }
            late_messages_last_sec =
                static_cast< int >( circular_buffer.get_and_clear_late( ) );
            very_late_messages_last_sec =
                static_cast< int >( circular_buffer.get_and_clear_discards( ) );
            discarded_messages_last_sec =
                late_messages_last_sec + very_late_messages_last_sec;

            late_messages += late_messages_last_sec;
            very_late_messages += very_late_messages_last_sec;
            discarded_messages = late_messages + very_late_messages;
        }
        simple_pv_server_update( epics_server );
        if ( latest_in_buffer.cycle( ) == 0 )
        {
            debug.clear( );
            dcus_received = 0;
            max_dcus_received = 0;
            min_dcus_received = 256;
            mean_dcus_received = 0;
            spread_ms = 0;
            max_spread_ms = 0;
            min_spread_ms = 1000000;
            mean_spread_ms = 0;
            mean_samples = 0;
            min_total_spread_ms = 1000000;
            max_total_spread_ms = 0;
        }

    } while ( keepRunning ); // End of infinite loop

    return 0;
}

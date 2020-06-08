//
// @file pub_xmit.cc
// @brief Stream publisher
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

#include "make_unique.hh"

#include "../drv/crc.c"
#include "../include/daq_core.h"
#include "drv/shmem.h"

#include <cds-pubsub/pub.hh>

#include <boost/lockfree/spsc_queue.hpp>

#include "args.h"

static const int          header_size = sizeof( struct daq_multi_dcu_header_t );
static const int          buf_size = DAQ_DCU_BLOCK_SIZE * 2;
int                       modelrates[ DAQ_TRANSIT_MAX_DCU ];
int                       dcuid[ DAQ_TRANSIT_MAX_DCU ];
daq_multi_dcu_data_t*     ixDataBlock;
daq_multi_cycle_header_t* ifo_header;
char*                     zbuffer;

char* ifo = nullptr;
char* ifo_data = nullptr;

int                 do_verbose = 0;
static volatile int keepRunning = 1;
size_t              cycle_data_size = 0;

class Arena
{
public:
    explicit Arena( int prealloc_count ) : arena_{}
    {
        int count = std::min( prealloc_count, 10 );
        for ( int i = 0; i < count; ++i )
        {
            put( new unsigned char[ sizeof( daq_dc_data_t ) ] );
        }
    }
    Arena( const Arena& ) = delete;
    Arena( Arena&& ) = delete;
    ~Arena( )
    {
        while ( arena_.read_available( ) )
        {
            unsigned char* tmp = nullptr;
            arena_.pop( tmp );
            if ( tmp )
            {
                delete[] tmp;
            }
        }
    }
    Arena& operator=( const Arena& ) = delete;
    Arena& operator=( Arena&& ) = delete;

    pub_sub::DataPtr
    get( )
    {
        unsigned char* tmp = nullptr;
        if ( !arena_.pop( tmp ) )
        {
            tmp = new unsigned char[ sizeof( daq_dc_data_t ) ];
        }
        return std::shared_ptr< unsigned char[] >( tmp,
                                                   [this]( unsigned char* p ) {
                                                       if ( p )
                                                       {
                                                           this->put( p );
                                                       }
                                                   } );
    }

    void
    put( unsigned char* p )
    {
        if ( !p )
        {
            return;
        }
        if ( !arena_.push( p ) )
        {
            delete[] p;
        }
    }

private:
    using queue_t =
        boost::lockfree::spsc_queue< unsigned char*,
                                     boost::lockfree::capacity< 10 > >;
    queue_t arena_;
};

/*********************************************************************************/
/*                                U S A G E */
/*                                                                               */
/*********************************************************************************/

/**
 * @brief Set the cycle counter to an invalid value.
 * @param header pointer to the input block header
 * @note Used to force a resync of the counter.
 */
void
reset_cycle_counter( volatile daq_multi_cycle_header_t* header )
{
    header->curCycle = 0x50505050;
}

/**
 * @brief wait until the data in the input shared mem buffer has the
 * @param header pointer to the input block header
 * requested cycle counter.
 *
 * @returns non-zero if it we timeout
 */
int
wait_for_cycle( volatile daq_multi_cycle_header_t* header,
                unsigned int                       requested_cycle )
{
    int timeout = 0;

    do
    {
        usleep( 2000 );
        if ( header->curCycle == requested_cycle )
        {
            return 0;
        }
        ++timeout;
    } while ( timeout < 500 );
    return 1;
}

// **********************************************************************************************
// Capture SIGHALT from ControlC
void
intHandler( int dummy )
{
    keepRunning = 0;
}

// **********************************************************************************************
void
print_diags( int                   nsys,
             int                   lastCycle,
             int                   sendLength,
             daq_multi_dcu_data_t* ixDataBlock )
{
    int           ii = 0;
    unsigned long sym_gps_sec = 0;
    unsigned long sym_gps_nsec = 0;

    // Print diags in verbose mode
    fprintf( stderr,
             "\nTime = %d-%d size = %d\n",
             ixDataBlock->header.dcuheader[ 0 ].timeSec,
             ixDataBlock->header.dcuheader[ 0 ].timeNSec,
             sendLength );
    fprintf( stderr,
             "Sym gps = %d-%d (time received)\n",
             (int)sym_gps_sec,
             (int)sym_gps_nsec );
    fprintf( stderr, "\tCycle = " );
    for ( ii = 0; ii < nsys; ii++ )
        fprintf( stderr, "\t\t%d", ixDataBlock->header.dcuheader[ ii ].cycle );
    fprintf( stderr, "\n\tTimeSec = " );
    for ( ii = 0; ii < nsys; ii++ )
        fprintf( stderr, "\t%d", ixDataBlock->header.dcuheader[ ii ].timeSec );
    fprintf( stderr, "\n\tTimeNSec = " );
    for ( ii = 0; ii < nsys; ii++ )
        fprintf(
            stderr, "\t\t%d", ixDataBlock->header.dcuheader[ ii ].timeNSec );
    fprintf( stderr, "\n\tDataSize = " );
    for ( ii = 0; ii < nsys; ii++ )
        fprintf( stderr,
                 "\t\t%d",
                 ixDataBlock->header.dcuheader[ ii ].dataBlockSize );
    fprintf( stderr, "\n\tTPCount = " );
    for ( ii = 0; ii < nsys; ii++ )
        fprintf(
            stderr, "\t\t%d", ixDataBlock->header.dcuheader[ ii ].tpCount );
    fprintf( stderr, "\n\tTPSize = " );
    for ( ii = 0; ii < nsys; ii++ )
        fprintf(
            stderr, "\t\t%d", ixDataBlock->header.dcuheader[ ii ].tpBlockSize );
    fprintf( stderr, "\n\tXmitSize = " );
    for ( ii = 0; ii < nsys; ii++ )
        fprintf( stderr, "\t\t%d", ixDataBlock->header.fullDataBlockSize );
    fprintf( stderr, "\n\n " );
}

// **********************************************************************************************
int
send_to_local_memory( const std::string& conn_string, int send_delay_ms )
{
    int   do_wait = 1;
    char* nextData = nullptr;

    int          ii = 0;
    int          lastCycle = 0;
    unsigned int nextCycle = 0;

    int status = 0;

    int           cur_req;
    std::uint32_t result;

    Arena              memory_arena( 5 );
    pub_sub::Publisher publisher( conn_string );
    do
    {

        status = wait_for_cycle( ifo_header, nextCycle );
        // status = waitNextCycle(nextCycle,sync2iop,shmIpcPtr[0]);
        if ( status != 0 )
        {
            keepRunning = 0;
            return ( 0 );
        }

        usleep( ( do_wait * 1000 ) );

        nextData = (char*)ifo_data;
        nextData += cycle_data_size * nextCycle;
        ixDataBlock = (daq_multi_dcu_data_t*)nextData;
        int sendLength = ixDataBlock->header.fullDataBlockSize +
            sizeof( daq_multi_dcu_header_t );
        if ( sendLength == -1 || sendLength > sizeof( daq_dc_data_t ) )
        {
            fprintf( stderr, "Message buffer overflow error\n" );
            return ( -1 );
        }
        // Print diags in verbose mode
        //        if ( nextCycle == 8 && do_verbose )
        //            print_diags( nsys, lastCycle, sendLength, ixDataBlock );

        pub_sub::DataPtr msg_buffer = memory_arena.get( );
        // Copy data to message buffer and compact
        memcpy( msg_buffer.get( ), nextData, sendLength );
        auto tmp = reinterpret_cast< daq_dc_data_t* >( nextData );
        // compact_daq_struct(tmp, (void*)msg_buffer.get());

        // Send Data
        usleep( send_delay_ms * 1000 );

        pub_sub::KeyType key =
            ( tmp->header.dcuheader[ 0 ].timeSec << 8 ) + nextCycle;
        publisher.publish(
            key, pub_sub::Message( std::move( msg_buffer ), sendLength ) );
        nextCycle = ( nextCycle + 1 ) % 16;

    } while ( keepRunning ); /* do this until sighalt */

    fprintf(
        stderr,
        "\n***********************************************************\n\n" );

    return 0;
}

/*********************************************************************************/
/*                                M A I N */
/*                                                                               */
/*********************************************************************************/

int
main( int argc, char* argv[] )
{
    int         max_data_size_mb = 64;
    int         max_data_size = 0;
    const char* publish_conn = nullptr;
    const char* buffer_name = "local_dc";
    int         send_delay_ms = 0;
    args_handle arg_parser = NULL;

    const char* logfname = 0;

    fprintf(
        stderr, "\n %s compiled %s : %s\n\n", argv[ 0 ], __DATE__, __TIME__ );

    arg_parser =
        args_create_parser( "Transmit data between a LIGO FE computer and the "
                            "daqd system over simple pub/sub interface" );
    if ( !arg_parser )
    {
        return -1;
    }
    args_add_string_ptr( arg_parser,
                         'b',
                         ARGS_NO_LONG,
                         "buffer",
                         "Name of the mbuf to read local data from",
                         &buffer_name,
                         "local_dc" );
    args_add_string_ptr( arg_parser,
                         'p',
                         ARGS_NO_LONG,
                         "",
                         "Publisher string",
                         &publish_conn,
                         "udp://127.0.0.1/127.255.255.255" );
    args_add_int( arg_parser,
                  'm',
                  ARGS_NO_LONG,
                  "20-100",
                  "Local memory buffer size in megabytes",
                  &max_data_size_mb,
                  100 );
    args_add_string_ptr( arg_parser,
                         'l',
                         ARGS_NO_LONG,
                         "filename",
                         "Log file name",
                         &logfname,
                         0 );
    args_add_flag(
        arg_parser, 'v', ARGS_NO_LONG, "Verbose output", &do_verbose );
    args_add_int(
        arg_parser,
        'D',
        ARGS_NO_LONG,
        "ms",
        "Add a delay in ms to before sending data.  Used to spread the load",
        &send_delay_ms,
        0 );

    if ( args_parse( arg_parser, argc, argv ) < 0 )
    {
        return -1;
    }
    /* Get the parameters */
    if ( max_data_size_mb < 20 )
    {
        fprintf( stderr, "Min data block size is 20 MB\n" );
        args_fprint_usage( arg_parser, argv[ 0 ], stderr );
        return -1;
    }
    if ( max_data_size_mb > 100 )
    {
        fprintf( stderr, "Max data block size is 100 MB\n" );
        args_fprint_usage( arg_parser, argv[ 0 ], stderr );
        return -1;
    }
    max_data_size = max_data_size_mb * 1024 * 1024;

    if ( logfname != nullptr )
    {
        if ( !freopen( logfname, "w", stdout ) )
        {
            perror( "freopen" );
            exit( 1 );
        }
        setvbuf( stdout, NULL, _IOLBF, 0 );
        stderr = stdout;
    }
    fprintf( stderr,
             "Writing DAQ data to local shared memory and sending out on "
             "the publisher\n" );

    // Get pointers to local DAQ mbuf
    ifo = (char*)findSharedMemorySize( (char*)buffer_name, max_data_size_mb );
    ifo_header = (daq_multi_cycle_header_t*)ifo;
    ifo_data = (char*)ifo + sizeof( daq_multi_cycle_header_t );
    cycle_data_size = ( max_data_size - sizeof( daq_multi_cycle_header_t ) ) /
        DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    cycle_data_size -= ( cycle_data_size % 8 );

    fprintf( stderr, "ifo mapped to %p\n", ifo );

    // Setup signal handler to catch Control C
    signal( SIGINT, intHandler );
    sleep( 1 );

    reset_cycle_counter( ifo_header );

    // Enter infinite loop of reading control model data and writing to local
    // shared memory

    send_to_local_memory( publish_conn, send_delay_ms );

    // Cleanup Open-MX stuff

    fprintf( stderr, "Closing out OpenMX and exiting\n" );
    args_destroy( &arg_parser );

    // we never exit except for timeout or being killed
    return 1;
}

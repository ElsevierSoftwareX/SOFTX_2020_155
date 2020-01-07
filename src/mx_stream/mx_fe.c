//
///// @file mx_fe.c
///// @brief  Front End data concentrator
////
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

#include "../drv/crc.c"
#include "../include/daq_core.h"
#include "myriexpress.h"
#include "mx_extensions.h"
#include <pthread.h>

#define MX_MUTEX_T pthread_mutex_t
#define MX_MUTEX_INIT( mutex_ ) pthread_mutex_init( mutex_, 0 )
#define MX_MUTEX_LOCK( mutex_ ) pthread_mutex_lock( mutex_ )
#define MX_MUTEX_UNLOCK( mutex_ ) pthread_mutex_unlock( mutex_ )

#define MX_THREAD_T pthread_t
#define MX_THREAD_CREATE( thread_, start_routine_, arg_ )                      \
    pthread_create( thread_, 0, start_routine_, arg_ )
#define MX_THREAD_JOIN( thread_ ) pthread_join( thread, 0 )

MX_MUTEX_T stream_mutex;
#define FILTER 0x12345
#define MATCH_VAL 0xabcdef
#define DFLT_EID 64
#define DFLT_LEN 8192
#define DFLT_END 128
#define DFLT_ITER 1000
#define NUM_RREQ 16 /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/
#define NUM_SREQ 256 /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/
#define MSG_BUF_SIZE sizeof( daq_dc_data_t )

#define DO_HANDSHAKE 0
#define MATCH_VAL_MAIN ( 1 << 31 )
#define MATCH_VAL_THREAD 1

#define __CDECL

static const int          header_size = sizeof( struct daq_multi_dcu_header_t );
static const int          buf_size = DAQ_DCU_BLOCK_SIZE * 2;
int                       modelrates[ DAQ_TRANSIT_MAX_DCU ];
int                       dcuid[ DAQ_TRANSIT_MAX_DCU ];
daq_multi_dcu_data_t*     ixDataBlock;
daq_multi_cycle_header_t* ifo_header;
char*                     zbuffer;

extern void* findSharedMemory( char* );
extern void* findSharedMemorySize( char*, int );

int                 do_verbose = 0;
static volatile int keepRunning = 1;
char*               ifo = 0;
char*               ifo_data = 0;
size_t              cycle_data_size = 0;

char msg_buffer[ MSG_BUF_SIZE ];

int daqStatBit[ 2 ];

/*********************************************************************************/
/*                                U S A G E */
/*                                                                               */
/*********************************************************************************/

void
Usage( )
{
    fprintf( stderr, "Usage of omx_xmit:\n" );
    fprintf( stderr, "mx_fe  -s <models> <OPTIONS>\n" );
    fprintf( stderr,
             " -b <buffer>    : Name of the mbuf to read local data from "
             "(defaults to local_dc)\n" );
    fprintf( stderr,
             " -m <value>     : Local memory buffer size in megabytes\n" );
    fprintf( stderr, " -l <filename>  : log file name\n" );
    fprintf( stderr, " -v 1           : Enable verbose output\n" );
    fprintf( stderr,
             " -e <0-31>	    : Number of the local mx end point to "
             "transmit on\n" );
    fprintf( stderr,
             " -r <0-31>      : Number of the remote mx end point to transmit "
             "to\n" );
    fprintf( stderr,
             " -t <target name: Name of MX target computer to transmit to\n" );
    fprintf( stderr,
             " -d <directory> : Path to the gds tp dir used to lookup model "
             "rates\n" );
    fprintf( stderr,
             " -D <value>     : Add a delay in ms to sending the data.  Used "
             "to spread the load\n" );
    fprintf( stderr,
             "                : when working with multiple sending systems.  "
             "Defaults to 0.\n" );
    fprintf( stderr, " -h             : This helpscreen\n" );
    fprintf( stderr, "\n" );
}

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
send_to_local_memory( int           nsys,
                      int           send_delay_ms,
                      mx_endpoint_t ep,
                      int64_t       his_nic_id,
                      uint16_t      his_eid,
                      uint32_t      match_val )
{
    int   do_wait = 1;
    char* nextData = 0;

    int          ii = 0;
    int          lastCycle = 0;
    unsigned int nextCycle = 0;

    int status = 0;
    int dataRdy[ 10 ];

    int                cur_req;
    mx_status_t        stat;
    mx_request_t       req[ NUM_SREQ ];
    mx_segment_t       seg;
    uint32_t           result;
    mx_endpoint_addr_t dest;
    uint32_t           filter = FILTER;

    for ( ii = 0; ii < 10; ii++ )
        dataRdy[ ii ] = 0;

    mx_set_error_handler( MX_ERRORS_RETURN );
    int myErrorSignal = 1;

    do
    {
        mx_return_t conStat =
            mx_connect( ep, his_nic_id, his_eid, filter, 1000, &dest );
        if ( conStat != MX_SUCCESS )
        {
            myErrorSignal = 1;
        }
        else
        {
            myErrorSignal = 0;
            fprintf( stderr, "Connection Made\n" );
            mx_return_t ret =
                mx_set_request_timeout( ep, 0, 1 ); // Set one second timeout
            if ( ret != MX_SUCCESS )
            {
                fprintf( stderr,
                         "Failed to set request timeout %s\n",
                         mx_strerror( ret ) );
                exit( 1 );
            }
        }
    } while ( myErrorSignal );

    do
    {
        myErrorSignal = 0;

        for ( ii = 0; ii < nsys; ii++ )
            dataRdy[ ii ] = 0;

        status = wait_for_cycle( ifo_header, nextCycle );
        // status = waitNextCycle(nextCycle,sync2iop,shmIpcPtr[0]);
        if ( status != 0 )
        {
            keepRunning = 0;
            ;
            return ( 0 );
        }

        // IOP will be first model ready
        // Need to wait for 2K models to reach end of their cycled
        usleep( ( do_wait * 1000 ) );

        nextData = (char*)ifo_data;
        nextData += cycle_data_size * nextCycle;
        ixDataBlock = (daq_multi_dcu_data_t*)nextData;
        int sendLength = ixDataBlock->header.fullDataBlockSize +
            sizeof( daq_multi_dcu_header_t );
        if ( sendLength == -1 || sendLength > MSG_BUF_SIZE )
        {
            fprintf( stderr, "Message buffer overflow error\n" );
            return ( -1 );
        }
        // Print diags in verbose mode
        if ( nextCycle == 8 && do_verbose )
            print_diags( nsys, lastCycle, sendLength, ixDataBlock );

        // Copy data to 0mq message buffer
        memcpy( (void*)&msg_buffer, nextData, sendLength );
        // Send Data
        usleep( send_delay_ms * 1000 );
        seg.segment_ptr = &msg_buffer;
        seg.segment_length = sendLength;
        mx_return_t res =
            mx_isend( ep, &seg, 1, dest, match_val, NULL, &req[ cur_req ] );
        if ( res != MX_SUCCESS )
        {
            fprintf( stderr, "mx_isend failed ret=%d\n", res );
            myErrorSignal = 1;
            break;
        }
    again:
        res = mx_wait( ep, &req[ cur_req ], 50, &stat, &result );
        if ( res != MX_SUCCESS )
        {
            fprintf( stderr,
                     "mx_cancel() failed with status %s\n",
                     mx_strerror( res ) );
            exit( 1 );
        }
        if ( result == 0 )
        {
            fprintf( stderr, "trying again \n" );
            goto again;
            // myErrorSignal = 1;
        }
        if ( stat.code != MX_STATUS_SUCCESS )
        {
            fprintf( stderr,
                     "isendxxx failed with status %s\n",
                     mx_strstatus( stat.code ) );
            myErrorSignal = 1;
        }

        nextCycle = ( nextCycle + 1 ) % 16;

    } while ( keepRunning && !myErrorSignal ); /* do this until sighalt */

    fprintf(
        stderr,
        "\n***********************************************************\n\n" );

    return 0;
}

/*********************************************************************************/
/*                                M A I N */
/*                                                                               */
/*********************************************************************************/

int __CDECL
    main( int argc, char* argv[] )
{
    int   counter = 0;
    int   nsys = 1;
    int   dcuId[ 10 ];
    int   ii = 0;
    char* gds_tp_dir = 0;
    int   max_data_size_mb = 64;
    int   max_data_size = 0;
    int   error = 0;
    char* buffer_name = "local_dc";
    int   send_delay_ms = 0;

    mx_endpoint_t ep;
    uint16_t      my_eid;
    uint64_t      his_nic_id;
    uint32_t      board_id;
    uint32_t      filter;
    uint16_t      his_eid;
    char*         rem_host;
    char*         sysname;
    extern char*  optarg;
    mx_return_t   ret;

    rem_host = NULL;
    sysname = NULL;
    filter = FILTER;
    my_eid = DFLT_EID;
    his_eid = DFLT_EID;
    board_id = MX_ANY_NIC;

    fprintf(
        stderr, "\n %s compiled %s : %s\n\n", argv[ 0 ], __DATE__, __TIME__ );

    ii = 0;

    if ( argc < 3 )
    {
        Usage( );
        return ( -1 );
    }

    /* Get the parameters */
    while ( ( counter = getopt( argc, argv, "b:e:m:h:v:r:t:d:l:D:" ) ) != EOF )
    {
        switch ( counter )
        {
        case 't':
            rem_host = optarg;
            break;
        case 'b':
            buffer_name = optarg;
            fprintf( stderr, "Buffer name = '%s'\n", buffer_name );
            break;

        case 'm':
            max_data_size_mb = atoi( optarg );
            if ( max_data_size_mb < 20 )
            {
                fprintf( stderr, "Min data block size is 20 MB\n" );
                return -1;
            }
            if ( max_data_size_mb > 100 )
            {
                fprintf( stderr, "Max data block size is 100 MB\n" );
                return -1;
            }
            break;

        case 'l':
            if ( 0 == freopen( optarg, "w", stdout ) )
            {
                perror( "freopen" );
                exit( 1 );
            }
            setvbuf( stdout, NULL, _IOLBF, 0 );
            stderr = stdout;
            break;
        case 'r':
            his_eid = atoi( optarg );
            fprintf( stderr, "remoteeid = %d\n", his_eid );
            break;
        case 'v':
            do_verbose = atoi( optarg );
            break;
        case 'e':
            my_eid = atoi( optarg );
            fprintf( stderr, "myeid = %d\n", my_eid );
            break;
        case 'd':
            gds_tp_dir = optarg;
            break;
        case 'h':
            Usage( );
            return ( 0 );
        case 'D':
            send_delay_ms = atoi( optarg );
            break;
        default:
            fprintf( stderr, "Not handling argument '%c'\n", counter );
        }
    }

    max_data_size = max_data_size_mb * 1024 * 1024;

    // If sending to DAQ via net enabled, ensure all options have been set
    mx_init( );
    MX_MUTEX_INIT( &stream_mutex );
    if ( my_eid == DFLT_EID || his_eid == DFLT_EID )
    {
        fprintf( stderr,
                 "\n***ERROR\n***Must set both -e and -r options to send data "
                 "to DAQ\n\n" );
        Usage( );
        return ( 0 );
    }
    fprintf( stderr,
             "Writing DAQ data to local shared memory and sending out on "
             "Open-MX\n" );
    if ( my_eid == 0 )
    {
        daqStatBit[ 0 ] = 1;
        daqStatBit[ 1 ] = 2;
    }
    else
    {
        daqStatBit[ 0 ] = 4;
        daqStatBit[ 1 ] = 8;
    }

    // Get pointers to local DAQ mbuf
    ifo = (char*)findSharedMemorySize( buffer_name, max_data_size_mb );
    ifo_header = (daq_multi_cycle_header_t*)ifo;
    ifo_data = (char*)ifo + sizeof( daq_multi_cycle_header_t );
    cycle_data_size = ( max_data_size - sizeof( daq_multi_cycle_header_t ) ) /
        DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    cycle_data_size -= ( cycle_data_size % 8 );

    fprintf( stderr, "ifo mapped to %p\n", ifo );

    // Setup signal handler to catch Control C
    signal( SIGINT, intHandler );
    sleep( 1 );

    // Open the NIC endpoint to send data
    fprintf( stderr, "Open endpoint \n" );
    ret = mx_open_endpoint( board_id, my_eid, filter, NULL, 0, &ep );
    if ( ret != MX_SUCCESS )
    {
        fprintf( stderr, "Failed to open endpoint %s\n", mx_strerror( ret ) );
        exit( 1 );
    }
    sleep( 1 );
    mx_hostname_to_nic_id( rem_host, &his_nic_id );

    reset_cycle_counter( ifo_header );

    // Enter infinite loop of reading control model data and writing to local
    // shared memory
    do
    {
        error = send_to_local_memory(
            nsys, send_delay_ms, ep, his_nic_id, his_eid, MATCH_VAL_MAIN );
    } while ( error == 0 && keepRunning == 1 );

    // Cleanup Open-MX stuff

    fprintf( stderr, "Closing out OpenMX and exiting\n" );
    mx_close_endpoint( ep );
    mx_finalize( );

    return 0;
}

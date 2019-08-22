//
/// @file zmq_xmit.c
/// @brief Transmit data from a buffer containing a
///  daq_multi_cycle_header_t structure out over
///  a zmq publisher.
///
//
#define _GNU_SOURCE
#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../drv/crc.c"
#include "../include/daqmap.h"
#include "../include/drv/fb.h"
#include "../include/daq_core.h"
#include "../drv/gpstime/gpstime.h"
#include "dc_utils.h"
#include "zmq_transport.h"
#include <assert.h>
#include <zmq.h>

#define MAX_NSYS 24

#define __CDECL

static const int          buf_size = DAQ_DCU_BLOCK_SIZE * 2;
daq_multi_cycle_header_t* ifo_header;

extern void* findSharedMemory( char* );
extern void* findSharedMemorySize( char*, int );

int do_verbose = 0;
// int sendLength = 0;
static volatile int keepRunning = 1;
char*               ifo;
char*               ifo_data;
size_t              cycle_data_size;

// ZMQ defines
// char msg_buffer[0x200000];
void* daq_context;
void* daq_publisher;

/*********************************************************************************/
/*                                U S A G E */
/*                                                                               */
/*********************************************************************************/

void
Usage( )
{
    printf( "Usage of zmq_xmit:\n" );
    printf( "zmq_xmit  -s <models> <OPTIONS>\n" );
    printf(
        " -b <buffer>    : Name of the mbuf to read data from [local_dc]\n" );
    printf( " -m <value>     : Local memory buffer size in megabytes\n" );
    printf( " -v 1           : Enable verbose output\n" );
    printf(
        " -e <interface> : Name of the interface to broadcast data through\n" );
    printf( " -D <value>     : Add a delay in ms to sending the data.  Used to "
            "spread the load\n" );
    printf( "                : when working with multiple sending systems.  "
            "Defaults to 0." );
    printf( " -h             : This helpscreen\n" );
    printf( "\n" );
}

void
zmq_make_connection( char* eport )
{
    char loc[ 200 ];
    int  rc;

    // Set up the data publisher socket
    daq_context = zmq_ctx_new( );
    daq_publisher = zmq_socket( daq_context, ZMQ_PUB );
    // sprintf(loc,"%s%d","tcp://*:",DAQ_DATA_PORT);
    // sprintf(loc,"%s%s%s%d","tcp://",eport,":",DAQ_DATA_PORT);
    if ( !dc_generate_connection_string( loc, eport, sizeof( loc ), 0 ) )
    {
        fprintf(
            stderr, "Unable to generate connection string for '%s'\n", eport );
        exit( 1 );
    }
    dc_set_zmq_options( daq_publisher );
    printf( "Binding to '%s'\n", loc );
    rc = zmq_bind( daq_publisher, loc );
    if ( rc < 0 )
    {
        fprintf( stderr, "Errno = %d: %s\n", errno, strerror( errno ) );
    }
    assert( rc == 0 );
    printf( "sending data on %s\n", loc );
}

// **********************************************************************************************

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

/**
 * @brief Check to see if the requested input data is in the input buffer
 * @param header pointer to the input block header
 * @param the cycle you are checking data for
 * @param max_data_size the max size of the input buffer
 * @return 0 if safe, != 0 if overflow
 */
int
data_will_overflow( volatile daq_multi_cycle_header_t* header,
                    int                                cycle,
                    int                                max_data_size )
{
    return ( ( ( cycle + 1 ) * header->cycleDataSize ) > max_data_size );
}

/**
 * @brief Return a pointer to the daq_multi_dcu_data_t* for a given cycle
 * @param header The main input buffer
 * @param cycle The cycle to retrieve data for
 * @returns A pointer to the cycles data
 * @note does not do bounds checking
 */
volatile daq_multi_dcu_data_t*
get_cycle_data( volatile daq_multi_cycle_header_t* header, int cycle )
{
    int            offset = cycle * header->cycleDataSize;
    volatile char* data_block =
        &( ( (volatile daq_multi_cycle_data_t*)header )->dataBlock[ 0 ] );
    return (volatile daq_multi_dcu_data_t*)( data_block + offset );
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
print_diags( volatile daq_multi_dcu_data_t* cycle_block )
{
    printf( "Total models %d\nData size %d\n\n",
            (int)cycle_block->header.dcuTotalModels,
            (int)cycle_block->header.fullDataBlockSize );
}

/*********************************************************************************/
/*                                M A I N */
/*                                                                               */
/*********************************************************************************/

int __CDECL
    main( int argc, char* argv[] )
{
    int   counter = 0;
    int   max_data_size_mb = 64;
    int   max_data_size = 0;
    int   error = 0;
    char* eport = 0;
    char* buffer_name = "local_dc";
    int   send_delay_ms = 0;

    if ( argc < 3 )
    {
        Usage( );
        return ( -1 );
    }

    /* Get the parameters */
    while ( ( counter = getopt( argc, argv, "b:m:v:e:l:hD:" ) ) != EOF )
    {
        switch ( counter )
        {
        case 'b':
            buffer_name = optarg;
            break;

        case 'm':
            max_data_size_mb = atoi( optarg );
            if ( max_data_size_mb < 20 )
            {
                return -1;
            }
            if ( max_data_size_mb > 100 )
            {
                return -1;
            }
            break;
        case 'v':
            do_verbose = atoi( optarg );
            break;
        case 'e':
            eport = optarg;
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
        case 'h':
            Usage( );
            return ( 0 );
        case 'D':
            send_delay_ms = atoi( optarg );
            break;
        }
    }

    max_data_size = max_data_size_mb * 1024 * 1024;

    zmq_make_connection( eport );

    // Get pointers to local DAQ mbuf
    ifo_header = findSharedMemorySize( buffer_name, max_data_size_mb );

    signal( SIGINT, intHandler );

    // Enter infinite loop of reading control model data and writing to local
    // shared memory
    reset_cycle_counter( ifo_header );
    int cycle = 0;
    do
    {
        if ( wait_for_cycle( ifo_header, cycle ) != 0 )
        {
            fprintf( stderr, "Unable to sync with data" );
            break;
        }

        if ( data_will_overflow( ifo_header, cycle, max_data_size ) )
        {
            fprintf( stderr,
                     "Overflow, required data is out of the input buffer" );
            break;
        }

        if ( send_delay_ms > 0 )
        {
            usleep( 1000 * send_delay_ms );
        }

        zmq_send_daq_multi_dcu_t(
            (daq_multi_dcu_data_t*)get_cycle_data( ifo_header, cycle ),
            daq_publisher,
            0 );

        if ( cycle == 0 && do_verbose )
        {
            print_diags( get_cycle_data( ifo_header, cycle ) );
        }

        ++cycle;
        cycle %= 16;
    } while ( error == 0 && keepRunning == 1 );

    printf( "Closing out ZMQ and exiting\n" );
    zmq_close( daq_publisher );
    zmq_ctx_destroy( daq_context );

    return 0;
}

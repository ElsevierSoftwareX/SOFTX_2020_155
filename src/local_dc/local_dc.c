//
///// @file local_dc.c
///// @brief  Front End data concentrator
////
//

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "args.h"
#include "../drv/crc.c"
#include "../include/daqmap.h"
#include "../include/drv/fb.h"
#include "../include/daq_core.h"
#include "../drv/gpstime/gpstime.h"
#include <pthread.h>
#include "modelrate.h"
#include "local_dc_utils.h"

#define MSG_BUF_SIZE sizeof( daq_dc_data_t )

#define __CDECL

static struct rmIpcStr*          shmIpcPtr[ 128 ];
static char*                     shmDataPtr[ 128 ];
static struct cdsDaqNetGdsTpNum* shmTpTable[ 128 ];
static const int          header_size = sizeof( struct daq_multi_dcu_header_t );
static const int          buf_size = DAQ_DCU_BLOCK_SIZE * 2;
int                       modelrates[ DAQ_TRANSIT_MAX_DCU ];
int                       dcuid[ DAQ_TRANSIT_MAX_DCU ];
daq_multi_dcu_data_t*     ixDataBlock;
daq_multi_cycle_header_t* ifo_header;
char*                     zbuffer;

extern void* findSharedMemory( char* );
extern void* findSharedMemorySize( char*, int );

char                modelnames[ DAQ_TRANSIT_MAX_DCU ][ 64 ];
int                 do_verbose = 0;
static volatile int keepRunning = 1;
char*               ifo;
char*               ifo_data;
size_t              cycle_data_size;

int symmetricom_fd = -1;
int daqStatBit[ 2 ];

// **********************************************************************************************
/// Get current GPS time from the symmetricom IRIG-B card
unsigned long
symm_gps_time( unsigned long* frac, int* stt )
{
    unsigned long t[ 3 ];

    ioctl( symmetricom_fd, IOCTL_SYMMETRICOM_TIME, &t );
    t[ 1 ] *= 1000;
    t[ 1 ] += t[ 2 ];
    if ( frac )
        *frac = t[ 1 ];
    if ( stt )
        *stt = 0;
    return t[ 0 ];
}

// *******************************************************************************
/// See if the GPS card is locked.
int
symm_ok( )
{
    unsigned long req = 0;
    ioctl( symmetricom_fd, IOCTL_SYMMETRICOM_STATUS, &req );
    fprintf( stderr, "Symmetricom status: %s\n", req ? "LOCKED" : "UNCLOCKED" );
    return req;
}

// *******************************************************************************
// Wait for data ready from FE models
// *******************************************************************************
int
waitNextCycle2( int          nsys,
                unsigned int cyclereq, // Cycle to wait for
                int          reset, // Request to reset model ipc shared memory
                int          dataRdy[],
                struct rmIpcStr* ipcPtr[] ) // Pointer to IOP IPC shared memory
{
    int iopRunning = 0;
    int ii;
    int threads_rdy = 0;
    int timeout = 0;

    // if reset, want to set all models cycle counters to impossible number
    // this takes care of uninitialized or stopped models
    if ( reset )
    {
        for ( ii = 0; ii < nsys; ++ii )
        {
            ipcPtr[ ii ]->cycle = 50;
        }
    }
    usleep( 1000 );
    // Wait until received data from at least 1 FE or timeout
    do
    {
        usleep( 2000 );
        if ( ipcPtr[ 0 ]->cycle == cyclereq )
        {
            iopRunning = 1;
            dataRdy[ 0 ] = 1;
        }
        timeout += 1;
    } while ( !iopRunning && timeout < 500 );

    // Wait until data received from everyone or timeout
    timeout = 0;
    do
    {
        usleep( 100 );
        for ( ii = 1; ii < nsys; ii++ )
        {
            if ( ipcPtr[ ii ]->cycle == cyclereq && !dataRdy[ ii ] )
                threads_rdy++;
            if ( ipcPtr[ ii ]->cycle == cyclereq )
                dataRdy[ ii ] = 1;
        }
        timeout += 1;
    } while ( threads_rdy < nsys && timeout < 20 );

    return ( iopRunning );
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

    sym_gps_sec = symm_gps_time( &sym_gps_nsec, 0 );
    // Print diags in verbose mode
    fprintf( stderr,
             "\nTime = %d-%d size = %d\n",
             shmIpcPtr[ 0 ]->bp[ lastCycle ].timeSec,
             shmIpcPtr[ 0 ]->bp[ lastCycle ].timeNSec,
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
        fprintf( stderr, "\t\t%d", shmIpcPtr[ ii ]->dataBlockSize );
    fprintf( stderr, "\n\n " );
}

// **********************************************************************************************
int
loadMessageBuffer( int nsys, int lastCycle, int status, int dataRdy[] )
{
    int   sendLength = 0;
    int   ii;
    int   dataXferSize;
    char* dataBuff;
    int   myCrc = 0;
    int   crcLength = 0;

    // Set pointer to 0MQ message data block
    zbuffer = (char*)&ixDataBlock->dataBlock[ 0 ];
    // Initialize data send length to size of message header
    sendLength = header_size;
    // Set number of FE models that have data in this message
    ixDataBlock->header.fullDataBlockSize = 0;
    int db = 0;
    // Loop thru all FE models
    for ( ii = 0; ii < nsys; ii++ )
    {
        if ( dataRdy[ ii ] )
        {
            // Set heartbeat monitor for return to DAQ software
            if ( lastCycle == 0 )
                shmIpcPtr[ ii ]->reqAck ^= daqStatBit[ 0 ];
            // Set DCU ID in header
            ixDataBlock->header.dcuheader[ db ].dcuId = shmIpcPtr[ ii ]->dcuId;
            // Set DAQ .ini file CRC checksum
            ixDataBlock->header.dcuheader[ db ].fileCrc = shmIpcPtr[ ii ]->crc;
            // Set 1/16Hz cycle number
            ixDataBlock->header.dcuheader[ db ].cycle = shmIpcPtr[ ii ]->cycle;
            // Set GPS seconds
            ixDataBlock->header.dcuheader[ db ].timeSec =
                shmIpcPtr[ ii ]->bp[ lastCycle ].timeSec;
            // Set GPS nanoseconds
            ixDataBlock->header.dcuheader[ db ].timeNSec =
                shmIpcPtr[ ii ]->bp[ lastCycle ].timeNSec;
            crcLength = shmIpcPtr[ ii ]->bp[ lastCycle ].crc;
            // Set Status -- as running
            ixDataBlock->header.dcuheader[ db ].status = 2;
            // Indicate size of data block
            // ********ixDataBlock->header.dcuheader[db].dataBlockSize =
            // shmIpcPtr[ii]->dataBlockSize;
            ixDataBlock->header.dcuheader[ db ].dataBlockSize = crcLength;
            // Prevent going beyond MAX allowed data size
            if ( ixDataBlock->header.dcuheader[ db ].dataBlockSize >
                 DAQ_DCU_BLOCK_SIZE )
                ixDataBlock->header.dcuheader[ db ].dataBlockSize =
                    DAQ_DCU_BLOCK_SIZE;
            // Calculate TP data size
            ixDataBlock->header.dcuheader[ db ].tpCount =
                (unsigned int)shmTpTable[ ii ]->count & 0xff;
            ixDataBlock->header.dcuheader[ db ].tpBlockSize = sizeof( float ) *
                modelrates[ ii ] * ixDataBlock->header.dcuheader[ db ].tpCount /
                DAQ_NUM_DATA_BLOCKS_PER_SECOND;

            // Copy GDSTP table to xmission buffer header
            memcpy( &( ixDataBlock->header.dcuheader[ db ].tpNum[ 0 ] ),
                    &( shmTpTable[ ii ]->tpNum[ 0 ] ),
                    sizeof( int ) *
                        ixDataBlock->header.dcuheader[ db ].tpCount );

            // Set pointer to dcu data in shared memory
            dataBuff = (char*)( shmDataPtr[ ii ] + lastCycle * buf_size );
            // Copy data from shared memory into local buffer
            dataXferSize = ixDataBlock->header.dcuheader[ db ].dataBlockSize +
                ixDataBlock->header.dcuheader[ db ].tpBlockSize;
            // if the dataXferSize is too large, something is wrong so return
            // error message.
            if ( dataXferSize > DAQ_DCU_BLOCK_SIZE )
                return ( -1 );
            memcpy( (void*)zbuffer, dataBuff, dataXferSize );

            // Calculate CRC on the data and add to header info
            myCrc = 0;
            myCrc = crc_ptr( (char*)zbuffer, crcLength, 0 );
            myCrc = crc_len( crcLength, myCrc );
            ixDataBlock->header.dcuheader[ db ].dataCrc = myCrc;

            // Increment the 0mq data buffer pointer for next FE
            zbuffer += dataXferSize;
            // Increment the 0mq message size with size of FE data block
            sendLength += dataXferSize;
            // Increment the data block size for the message, this includes
            // regular data + TP data
            ixDataBlock->header.fullDataBlockSize += dataXferSize;

            // Update heartbeat monitor to DAQ code
            if ( lastCycle == 0 )
                shmIpcPtr[ ii ]->reqAck ^= daqStatBit[ 1 ];
            db++;
        }
    }
    ixDataBlock->header.dcuTotalModels = db;
    return sendLength;
}

// **********************************************************************************************
int
send_to_local_memory( int nsys, int len, int do_wait )
{
    char* nextData;

    int          ii;
    int          lastCycle = 0;
    unsigned int nextCycle = 0;

    int sync2iop = 1;
    int status = 0;
    int dataRdy[ 10 ];

    int cur_req;
    int init_mx = 1;

    for ( ii = 0; ii < 10; ii++ )
        dataRdy[ ii ] = 0;

    int           myErrorSignal = 1;
    unsigned long maxDataSize =
        ( MSG_BUF_SIZE > cycle_data_size ? cycle_data_size : MSG_BUF_SIZE );

    do
    {

        myErrorSignal = 0;

        for ( ii = 0; ii < nsys; ii++ )
            dataRdy[ ii ] = 0;
        status =
            waitNextCycle2( nsys, nextCycle, sync2iop, dataRdy, shmIpcPtr );
        // status = waitNextCycle(nextCycle,sync2iop,shmIpcPtr[0]);
        if ( !status )
        {
            keepRunning = 0;
            ;
            return ( 0 );
        }
        else
            sync2iop = 0;

        // IOP will be first model ready
        // Need to wait for 2K models to reach end of their cycled
        usleep( ( do_wait * 1000 ) );

        nextData = (char*)ifo_data;
        nextData += cycle_data_size * nextCycle;
        ixDataBlock = (daq_multi_dcu_data_t*)nextData;
        int sendLength = loadMessageBuffer( nsys, nextCycle, status, dataRdy );
        if ( sendLength == -1 || sendLength > maxDataSize )
        {
            fprintf( stderr, "Message buffer overflow error\n" );
            return ( -1 );
        }
        // Print diags in verbose mode
        if ( nextCycle == 8 && do_verbose )
            print_diags( nsys, lastCycle, sendLength, ixDataBlock );
        // Write header info
        ifo_header->curCycle = nextCycle;
        ifo_header->cycleDataSize = cycle_data_size;
        ifo_header->maxCycle = DAQ_NUM_DATA_BLOCKS_PER_SECOND;

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
    int           counter = 0;
    int           nsys = 1;
    int           ii = 0;
    char*         tmp = 0;
    const char*   gds_tp_dir_ = 0;
    char*         gds_tp_dir = 0;
    int           max_data_size_mb = 64;
    int           max_data_size = 0;
    int           error = 0;
    int           status = -1;
    unsigned long gps_frac = 0;
    int           gps_stt = 0;
    int           gps_ok = 0;
    unsigned long gps_time = 0;
    char*         buffer_name = NULL;
    const char*   buffer_name_ = NULL;

    const char* sysname_;
    char*       sysname;
    const char* logfilename = NULL;
    int         len;
    int         iter;
    int         do_wait = 1;
    int         do_bothways;
    args_handle arg_parser = NULL;

    sysname = NULL;
    bzero( modelrates, sizeof( modelrates[ 0 ] ) * DAQ_TRANSIT_MAX_DCU );
    bzero( dcuid, sizeof( dcuid[ 0 ] ) * DAQ_TRANSIT_MAX_DCU );

    fprintf(
        stderr, "\n %s compiled %s : %s\n\n", argv[ 0 ], __DATE__, __TIME__ );

    ii = 0;

    arg_parser = args_create_parser(
        "The local_dc process concentrates or combines the shared memory "
        "buffers from LIGO FE models into one buffer formated for injestion "
        "into the daqd or transmission over the network.\n"
        "The local_dc needs to get some information from the model .par files. "
        "For systems (such as the standalone edc) that do not have .par files "
        "the dcuid & rate may be specified with the model rate (ex edc:52:16 "
        "or edc:52, if not specified the rate defaults to 16Hz).  Only do this "
        "for special cases." );
    if ( !arg_parser )
    {
        return -1;
    }
    args_add_string_ptr( arg_parser,
                         'b',
                         ARGS_NO_LONG,
                         "buffer",
                         "Name of the mbuf to write data to locally",
                         &buffer_name_,
                         "local_dc" );
    args_add_int( arg_parser,
                  'm',
                  ARGS_NO_LONG,
                  "MB",
                  "Local memory buffer size",
                  &max_data_size_mb,
                  100 );
    args_add_string_ptr( arg_parser,
                         's',
                         ARGS_NO_LONG,
                         "systems",
                         "Space seperated list of systems.",
                         &sysname_,
                         "" );
    args_add_string_ptr( arg_parser,
                         'l',
                         ARGS_NO_LONG,
                         "file",
                         "Log file name",
                         &logfilename,
                         "-" );
    args_add_int( arg_parser,
                  'v',
                  ARGS_NO_LONG,
                  "level",
                  "Enable verbose output",
                  &do_verbose,
                  0 );
    args_add_string_ptr( arg_parser,
                         'd',
                         ARGS_NO_LONG,
                         "directory",
                         "Path to the gds tp dir used to lookup model rates",
                         &gds_tp_dir_,
                         NULL );
    args_add_int( arg_parser,
                  'w',
                  ARGS_NO_LONG,
                  "ms",
                  "Number of ms to wait for models to finish",
                  &do_wait,
                  1 );
    if ( args_parse( arg_parser, argc, argv ) < 0 )
    {
        exit( 1 );
    }

    buffer_name = malloc( strlen( buffer_name_ ) + 1 );
    strncpy( buffer_name, buffer_name_, strlen( buffer_name_ ) + 1 );
    sysname = malloc( strlen( sysname_ ) + 1 );
    strncpy( sysname, sysname_, strlen( sysname_ ) + 1 );
    if ( gds_tp_dir_ )
    {
        gds_tp_dir = malloc( strlen( gds_tp_dir_ ) + 1 );
        strncpy( gds_tp_dir, gds_tp_dir_, strlen( gds_tp_dir_ ) + 1 );
    }

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
    fprintf( stderr, "sysnames = %s\n", sysname );

    if ( strcmp( logfilename, "-" ) != 0 )
    {
        if ( 0 == freopen( logfilename, "w", stdout ) )
        {
            perror( "freopen" );
            exit( 1 );
        }
        setvbuf( stdout, NULL, _IOLBF, 0 );
        stderr = stdout;
    }

    max_data_size = max_data_size_mb * 1024 * 1024;

    daqStatBit[ 0 ] = 1;
    daqStatBit[ 1 ] = 2;
    fprintf( stderr, "Writing DAQ data to local shared memory only \n" );

    // Parse the model names
    if ( sysname != NULL )
    {
        fprintf( stderr, "System names: %s\n", sysname );
        sprintf( modelnames[ 0 ], "%s", strtok( sysname, " " ) );
        for ( ;; )
        {
            char* s = strtok( 0, " " );
            if ( !s )
                break;
            sprintf( modelnames[ nsys ], "%s", s );
            nsys++;
        }
    }
    else
    {
        args_fprint_usage( arg_parser, argv[ 0 ], stderr );
        return ( 0 );
    }

    for ( ii = 0; ii < nsys; ++ii )
    {
        extract_dcu_rate_from_name(
            modelnames[ ii ], &dcuid[ ii ], &modelrates[ ii ] );
        trim_dcuid_and_rate_from_name( modelnames[ ii ] );
    }

    // Open file descriptor for the gpstime driver
    symmetricom_fd = open( "/dev/gpstime", O_RDWR | O_SYNC );
    if ( symmetricom_fd < 0 )
    {
        perror( "/dev/gpstime" );
        exit( 1 );
    }
    gps_ok = symm_ok( );
    gps_time = symm_gps_time( &gps_frac, &gps_stt );
    fprintf( stderr,
             "GPS TIME = %ld\tfrac = %ld\tstt = %d\n",
             gps_time,
             gps_frac,
             gps_stt );

    // Find the shared memory locations for the various model names
    for ( ii = 0; ii < nsys; ii++ )
    {
        char shmem_fname[ 128 ];
        sprintf( shmem_fname, "%s_daq", modelnames[ ii ] );
        void* dcu_addr = findSharedMemory( shmem_fname );
        if ( dcu_addr == NULL )
        {
            fprintf( stderr, "Can't map shmem\n" );
            exit( -1 );
        }
        else
        {
            fprintf( stderr,
                     " %s mapped at 0x%lx\n",
                     modelnames[ ii ],
                     (unsigned long)dcu_addr );
        }
        shmIpcPtr[ ii ] =
            (struct rmIpcStr*)( (char*)dcu_addr + CDS_DAQ_NET_IPC_OFFSET );
        shmDataPtr[ ii ] = ( (char*)dcu_addr + CDS_DAQ_NET_DATA_OFFSET );
        shmTpTable[ ii ] =
            (struct cdsDaqNetGdsTpNum*)( (char*)dcu_addr +
                                         CDS_DAQ_NET_GDS_TP_TABLE_OFFSET );
    }

    // Get model rates to get GDS TP data sizes.
    for ( ii = 0; ii < nsys; ii++ )
    {
        if ( modelrates[ ii ] == 0 )
        {
            status = get_model_rate_dcuid(
                &modelrates[ ii ], &dcuid[ ii ], modelnames[ ii ], gds_tp_dir );

            if ( status != 0 || modelrates[ ii ] == 0 )
            {
                fprintf( stderr,
                         "Unable to determine the rate of %s\n",
                         modelnames[ ii ] );
                exit( 1 );
            }
        }
        fprintf( stderr,
                 "Model %s rate = %d dcuid = %d\n",
                 modelnames[ ii ],
                 modelrates[ ii ],
                 dcuid[ ii ] );
    }

    // Get pointers to local DAQ mbuf
    ifo = (char*)findSharedMemorySize( buffer_name, max_data_size_mb );
    ifo_header = (daq_multi_cycle_header_t*)ifo;
    ifo_data = (char*)ifo + sizeof( daq_multi_cycle_header_t );
    cycle_data_size = ( max_data_size - sizeof( daq_multi_cycle_header_t ) ) /
        DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    cycle_data_size -= ( cycle_data_size % 8 );

    // Setup signal handler to catch Control C
    signal( SIGINT, intHandler );
    sleep( 1 );

    // Enter infinite loop of reading control model data and writing to local
    // shared memory
    do
    {
        error = send_to_local_memory( nsys, len, do_wait );
    } while ( error == 0 && keepRunning == 1 );

    free( (void*)gds_tp_dir_ );
    free( (void*)sysname_ );
    free( (void*)buffer_name );
    // local_dc never returns unless there is a timeout - ie an error
    // or a signal (ctrl-c) has been sent, so it is killed
    // always return an error code.
    return 1;
}

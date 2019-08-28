//
/// @file dix_xmit.c
/// @brief  DAQ data concentrator code. Takes data from a daq_multi_cycle_t
/// shared memory buffer and sends it out over IX dolphin.
//

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
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "../drv/crc.c"
#include <time.h>
#include "../include/daqmap.h"
#include "../include/daq_core.h"
#include "dc_utils.h"

#include "sisci_types.h"
#include "sisci_api.h"
#include "sisci_error.h"
#include "sisci_demolib.h"
#include "testlib.h"

#include "simple_pv.h"

#define __CDECL

#include "./dolphin_common.c"

static int                xmitDataOffset[ IX_BLOCK_COUNT ];
daq_multi_cycle_header_t* xmitHeader[ IX_BLOCK_COUNT ];

extern void* findSharedMemorySize( char*, int );

int do_verbose = 0;

static volatile int keepRunning = 1;

void
usage( )
{
    fprintf( stderr,
             "Usage: dix_ix_xmit [args] -m shared memory size -g IX "
             "channel \n" );
    fprintf( stderr, "-l filename - log file name\n" );
    fprintf( stderr, "-b buffer name - Input buffer [local_dc]\n" );
    fprintf( stderr,
             "-m buffer size - Sizer of the input buffer in MB [20-100]\n" );
    fprintf( stderr, "-v - verbose prints diag test data\n" );
    fprintf( stderr, "-g - Dolphin IX channel to xmit on (0-3)\n" );
    fprintf( stderr, "-p - Debug pv prefix, requires -P as well\n" );
    fprintf( stderr,
             "-P - Path to a named pipe to send PV debug information to\n" );
    fprintf( stderr, "-h - help\n" );
}

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

// **********************************************************************************************
void
print_diags( int                   nsys,
             int                   lastCycle,
             int                   sendLength,
             daq_multi_dcu_data_t* ixDataBlock,
             int                   dbs[] )
{
    // **********************************************************************************************
    int ii = 0;
    // Print diags in verbose mode
    // fprintf( stderr, "Receive errors = %d\n", rcv_errors );
    fprintf( stderr,
             "Time = %d\t size = %d\n",
             ixDataBlock->header.dcuheader[ 0 ].timeSec,
             sendLength );
    fprintf( stderr,
             "DCU ID\tCycle \t "
             "TimeSec\tTimeNSec\tDataSize\tTPCount\tTPSize\tXmitSize\n" );
    for ( ii = 0; ii < nsys; ii++ )
    {
        fprintf( stderr, "%d", ixDataBlock->header.dcuheader[ ii ].dcuId );
        fprintf( stderr, "\t%d", ixDataBlock->header.dcuheader[ ii ].cycle );
        fprintf( stderr, "\t%d", ixDataBlock->header.dcuheader[ ii ].timeSec );
        fprintf( stderr, "\t%d", ixDataBlock->header.dcuheader[ ii ].timeNSec );
        fprintf( stderr,
                 "\t\t%d",
                 ixDataBlock->header.dcuheader[ ii ].dataBlockSize );
        fprintf(
            stderr, "\t\t%d", ixDataBlock->header.dcuheader[ ii ].tpCount );
        fprintf(
            stderr, "\t%d", ixDataBlock->header.dcuheader[ ii ].tpBlockSize );
        fprintf( stderr, "\t%d", dbs[ ii ] );
        fprintf( stderr, "\n " );
    }
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

// *************************************************************************
// Main Process
// *************************************************************************
int
main( int argc, char** argv )
{
    char* buffer_name = "local_dc";
    int   c;
    int   ii; // Loop counter

    extern char* optarg; // Needed to get arguments to program

    // PV/debug information
    char* pv_prefix = 0;
    char* pv_debug_pipe_name = 0;
    int   pv_debug_pipe = -1;

    // Declare shared memory data variables
    daq_multi_cycle_header_t* ifo_header;
    char*                     ifo;
    char*                     ifo_data;
    int                       cycle_data_size;
    daq_multi_dcu_data_t*     ifoDataBlock;
    char*                     nextData;
    int                       max_data_size_mb = 100;
    int                       max_data_size = 0;
    char*                     mywriteaddr;
    int                       xmitBlockNum = 0;

    /* set up defaults */
    int xmitData = 0;

    // Get arguments sent to process
    while ( ( c = getopt( argc, argv, "b:hm:g:vp:P:l:" ) ) != EOF )
        switch ( c )
        {
        case 'v':
            do_verbose = 1;
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
        case 'g':
            segmentId = atoi( optarg );
            xmitData = 1;
            break;
        case 'b':
            buffer_name = optarg;
            break;
        case 'p':
            pv_prefix = optarg;
            break;
        case 'P':
            pv_debug_pipe_name = optarg;
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
        default:
            usage( );
            exit( 1 );
        }
    max_data_size = max_data_size_mb * 1024 * 1024;

    // set up to catch Control C
    signal( SIGINT, intHandler );
    // setup to ignore sig pipe
    signal( SIGPIPE, sigpipeHandler );

    // Get pointers to local DAQ mbuf
    ifo = (char*)findSharedMemorySize( buffer_name, max_data_size_mb );
    ifo_header = (daq_multi_cycle_header_t*)ifo;
    ifo_data = (char*)ifo + sizeof( daq_multi_cycle_header_t );

    reset_cycle_counter( ifo_header );
    if ( wait_for_cycle( ifo_header, 0 ) != 0 )
    {
        if ( wait_for_cycle( ifo_header, 0 ) != 0 )
        {
            fprintf( stderr,
                     "The input mbuf is not updating. Cannot calculate buffer "
                     "sizes, aborting!" );
            exit( 1 );
        }
    }

    cycle_data_size = ifo_header->cycleDataSize;
    if ( cycle_data_size % 8 != 0 )
    {
        fprintf( stderr, "Insufficient alignment for cycle_data_size\n" );
        fprintf( stderr,
                 "cycle_data_size = %ld, %%8 = %d\n",
                 (long int)cycle_data_size,
                 cycle_data_size % 8 );
        exit( 1 );
    }
    if ( ( cycle_data_size * ( ifo_header->maxCycle ) ) +
             sizeof( *ifo_header ) >
         max_data_size )
    {
        fprintf( stderr,
                 "Overflow condition, the input buffer window is"
                 "too small for all possible cycles\n" );
        fprintf( stderr, "Input buffer size = %ld\n", (long int)max_data_size );
        fprintf( stderr,
                 "# of cycles = %d, cycle data size = %ld\n",
                 ifo_header->maxCycle,
                 (long int)cycle_data_size );
        fprintf( stderr,
                 "With headers this requires %ld bytes.\n",
                 (long int)( ( cycle_data_size * ( ifo_header->maxCycle ) ) +
                             sizeof( *ifo_header ) ) );
        exit( 1 );
    }

    // Connect to Dolphin
    error = dolphin_init( );
    fprintf( stderr,
             "Read = 0x%lx \n Write = 0x%lx \n",
             (long)readAddr,
             (long)writeAddr );

    // Set pointer to xmit header in Dolphin xmit data area.
    mywriteaddr = (char*)writeAddr;
    for ( ii = 0; ii < IX_BLOCK_COUNT; ii++ )
    {
        xmitHeader[ ii ] = (daq_multi_cycle_header_t*)mywriteaddr;
        mywriteaddr += IX_BLOCK_SIZE;
        xmitDataOffset[ ii ] =
            IX_BLOCK_SIZE * ii + sizeof( struct daq_multi_cycle_header_t );
        fprintf( stderr,
                 "Dolphin at 0x%lx and 0x%lx\n",
                 (long)xmitHeader[ ii ],
                 (long)xmitDataOffset[ ii ] );
    }

    int              nextCycle = 0;
    int64_t          mytime = 0;
    int64_t          mylasttime = 0;
    int64_t          myptime = 0;
    int64_t          n_cycle_time = 0;
    int              mytotaldcu = 0;
    char*            zbuffer;
    size_t           zbuffer_remaining = 0;
    int              dc_datablock_size = 0;
    int              datablock_size_running = 0;
    int              datablock_size_mb_s = 0;
    static const int header_size = sizeof( daq_multi_dcu_header_t );
    char             dcstatus[ 4096 ];
    char             dcs[ 48 ];
    int              edcuid[ 10 ];
    int              estatus[ 10 ];
    int              edbs[ 10 ];
    unsigned long    ets = 0;
    int              timeout = 0;
    int              threads_rdy;
    int              any_rdy = 0;
    int              jj, kk;
    int              sendLength = 0;

    int     min_cycle_time = 1 << 30;
    int     pv_min_cycle_time = 0;
    int     max_cycle_time = 0;
    int     pv_max_cycle_time = 0;
    int     mean_cycle_time = 0;
    int     pv_mean_cycle_time = 0;
    int     pv_dcu_count = 0;
    int     pv_total_datablock_size = 0;
    int     pv_datablock_size_mb_s = 0;
    int     uptime = 0;
    int     pv_uptime = 0;
    int     gps_time = 0;
    int     pv_gps_time = 0;
    int     missed_flag = 0;
    int64_t min_recv_time = 0;
    int64_t cur_ref_time = 0;
    int     festatus = 0;
    int     pv_festatus = 0;

    SimplePV pvs[] = {
        {
            "RECV_MIN_MS",
            SIMPLE_PV_INT,
            &pv_min_cycle_time,

            80,
            45,
            70,
            54,
        },
        {
            "RECV_MAX_MS",
            SIMPLE_PV_INT,
            &pv_max_cycle_time,

            80,
            45,
            70,
            54,
        },
        {
            "RECV_MEAN_MS",
            SIMPLE_PV_INT,
            &pv_mean_cycle_time,

            80,
            45,
            70,
            54,
        },
        {
            "DCU_COUNT",
            SIMPLE_PV_INT,
            &pv_dcu_count,

            120,
            0,
            115,
            0,
        },
        {
            "DATA_SIZE",
            SIMPLE_PV_INT,
            &pv_total_datablock_size,

            100 * 1024 * 1024,
            0,
            90 * 1024 * 1024,
            1 * 1024 * 1024,
        },
        {
            "DATA_RATE",
            SIMPLE_PV_INT,
            &pv_datablock_size_mb_s,

            100 * 1024 * 1024,
            0,
            90 * 1024 * 1024,
            1000000,
        },
        {
            "UPTIME_SECONDS",
            SIMPLE_PV_INT,
            &pv_uptime,

            100 * 1024 * 1024,
            0,
            90 * 1024 * 1024,

            1,
        },
        {
            "RCV_STATUS",
            SIMPLE_PV_INT,
            &pv_festatus,

            0xffffffff,
            0,
            0xfffffffe,

            1,
        },
        {
            "GPS",
            SIMPLE_PV_INT,
            &pv_gps_time,

            0xfffffff,
            0,
            0xfffffffe,

            1,
        },

    };
    if ( pv_debug_pipe_name )
    {
        pv_debug_pipe = open( pv_debug_pipe_name, O_NONBLOCK | O_RDWR, 0 );
        if ( pv_debug_pipe < 0 )
        {
            fprintf( stderr,
                     "Unable to open %s for writting (pv status)\n",
                     pv_debug_pipe_name );
            exit( 1 );
        }
    }

    missed_flag = 1;
    do
    {
        if ( wait_for_cycle( ifo_header, nextCycle ) == 0 )
        {
            int tbsize = 0;
            // Timing diagnostics
            mytime = s_clock( );
            myptime = mytime - mylasttime;
            mylasttime = mytime;
            if ( myptime < min_cycle_time )
            {
                min_cycle_time = myptime;
            }
            if ( myptime > max_cycle_time )
            {
                max_cycle_time = myptime;
            }
            mean_cycle_time += myptime;
            ++n_cycle_time;

            // Reset total DCU counter
            mytotaldcu = 0;
            // Reset total DC data size counter
            dc_datablock_size = 0;
            // Get pointer to next data block in shared memory

            nextData = (char*)ifo_data;
            nextData += cycle_data_size * nextCycle;
            ifoDataBlock = (daq_multi_dcu_data_t*)nextData;

            min_recv_time = 0x7fffffffffffffff;
            festatus = 0;

            // Write total data block size to shared memory header
            // ifoDataBlock->header.fullDataBlockSize = dc_datablock_size;
            // Write total dcu count to shared memory header
            // ifoDataBlock->header.dcuTotalModels = mytotaldcu;
            // Set multi_cycle head cycle to indicate data ready for this cycle
            // ifo_header->curCycle = nextCycle;
            xmitBlockNum = nextCycle % IX_BLOCK_COUNT;

            mytotaldcu = ifoDataBlock->header.dcuTotalModels;

            // Calc IX message size
            sendLength = header_size + ifoDataBlock->header.fullDataBlockSize;

            if ( nextCycle == 0 )
            {
                datablock_size_mb_s = datablock_size_running / 1024;
                pv_datablock_size_mb_s = datablock_size_mb_s;
                uptime++;
                pv_uptime = uptime;
                gps_time = ifoDataBlock->header.dcuheader[ 0 ].timeSec;
                pv_gps_time = gps_time;
                pv_dcu_count = mytotaldcu;
                pv_festatus = festatus;
                mean_cycle_time =
                    ( n_cycle_time > 0 ? mean_cycle_time / n_cycle_time
                                       : 1 << 31 );

                pv_mean_cycle_time = mean_cycle_time;
                pv_max_cycle_time = max_cycle_time;
                pv_min_cycle_time = min_cycle_time;
                send_pv_update( pv_debug_pipe,
                                pv_prefix,
                                pvs,
                                sizeof( pvs ) / sizeof( pvs[ 0 ] ) );

                if ( do_verbose )
                {
                    fprintf( stderr,
                             "\nData rdy for cycle = %d\t\tTime Interval = %ld "
                             "msec\n",
                             nextCycle,
                             myptime );
                    fprintf( stderr,
                             "Min/Max/Mean cylce time %d/%d/%d msec over %ld "
                             "cycles\n",
                             min_cycle_time,
                             max_cycle_time,
                             mean_cycle_time,
                             n_cycle_time );
                    fprintf( stderr,
                             "Total DCU = %d\t\t\tBlockSize = %d\n",
                             mytotaldcu,
                             dc_datablock_size );
                    print_diags(
                        mytotaldcu, nextCycle, sendLength, ifoDataBlock, edbs );
                }
                n_cycle_time = 0;
                min_cycle_time = 1 << 30;
                max_cycle_time = 0;
                mean_cycle_time = 0;

                missed_flag = 1;
                datablock_size_running = 0;
            }
            else
            {
                missed_flag <<= 1;
            }

            if ( xmitData )
            {
                if ( sendLength > IX_BLOCK_SIZE )
                {
                    fprintf( stderr,
                             "Buffer overflow.  Sending %d bytes into a "
                             "dolphin block that holds %d\n",
                             (int)sendLength,
                             (int)IX_BLOCK_SIZE );
                    abort( );
                }
                // WRITEDATA to Dolphin Network
                SCIMemCpy( sequence,
                           nextData,
                           remoteMap,
                           xmitDataOffset[ xmitBlockNum ],
                           sendLength,
                           memcpyFlag,
                           &error );
                error = SCI_ERR_OK;
                if ( error != SCI_ERR_OK )
                {
                    fprintf(
                        stderr, "SCIMemCpy failed - Error code 0x%x\n", error );
                    fprintf( stderr,
                             "For reference the expected error codes are:\n" );
                    fprintf( stderr,
                             "SCI_ERR_OUT_OF_RANGE = 0x%x\n",
                             SCI_ERR_OUT_OF_RANGE );
                    fprintf( stderr,
                             "SCI_ERR_SIZE_ALIGNMENT = 0x%x\n",
                             SCI_ERR_SIZE_ALIGNMENT );
                    fprintf( stderr,
                             "SCI_ERR_OFFSET_ALIGNMENT = 0x%x\n",
                             SCI_ERR_OFFSET_ALIGNMENT );
                    fprintf( stderr,
                             "SCI_ERR_TRANSFER_FAILED = 0x%x\n",
                             SCI_ERR_TRANSFER_FAILED );
                    return error;
                }
                // Set data header information
                unsigned int maxCycle = ifo_header->maxCycle;
                unsigned int curCycle = ifo_header->curCycle;
                xmitHeader[ xmitBlockNum ]->maxCycle = maxCycle;
                xmitHeader[ xmitBlockNum ]->cycleDataSize = sendLength;
                ;
                // Send cycle last as indication of data ready for receivers
                xmitHeader[ xmitBlockNum ]->curCycle = curCycle;
                // Have to flush the buffers to make data go onto Dolphin
                // network
                SCIFlush( sequence, SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY );
            }
        }
        sprintf( dcstatus, "%ld ", ets );
        for ( ii = 0; ii < mytotaldcu; ii++ )
        {
            sprintf(
                dcs, "%d %d %d ", edcuid[ ii ], estatus[ ii ], edbs[ ii ] );
            strcat( dcstatus, dcs );
        }

        // Increment cycle count
        nextCycle++;
        nextCycle %= 16;
    } while ( keepRunning ); // End of infinite loop

    if ( xmitData )
    {
        fprintf( stderr, "closing out ix\n" );
        // Cleanup the Dolphin connections
        error = dolphin_closeout( );
    }

    exit( 0 );
}

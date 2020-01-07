//
///	@file mx_crc_only.c
///	@brief  Dummy code to run RCG V3.1 and later in 'standiop' mode ie
///standalone system 	< This is stripped down version of mx_stream. Purpose is to
///provide crc checksum for 	< DAQ on a standalone system.
//

#include "myriexpress.h"
#include "mx_extensions.h"
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
#include <iostream>
#include "../drv/crc.c"
#include "../daqd/stats/stats.hh"

#define FILTER 0x12345
#define MATCH_VAL 0xabcdef
#define DFLT_EID 1
#define DFLT_LEN 8192
#define DFLT_END 128
#define MAX_LEN ( 1024 * 1024 * 1024 )
#define DFLT_ITER 1000
#define NUM_RREQ 16 /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/
#define NUM_SREQ 256 /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/

#define DO_HANDSHAKE 0
#define MATCH_VAL_MAIN ( 1 << 31 )
#define MATCH_VAL_THREAD 1

#define DAQ_NUM_DATA_BLOCKS 16
#define DAQ_NUM_DATA_BLOCKS_PER_SECOND 16
#define CDS_DAQ_NET_IPC_OFFSET 0x0
#define CDS_DAQ_NET_GDS_TP_TABLE_OFFSET 0x1000
#define CDS_DAQ_NET_DATA_OFFSET 0x2000
#define DAQ_DCU_SIZE 0x400000
#define DAQ_DCU_BLOCK_SIZE ( DAQ_DCU_SIZE / DAQ_NUM_DATA_BLOCKS )
#define DAQ_GDS_MAX_TP_NUM 0x100
#define MMAP_SIZE 1024 * 1024 * 64 - 5000

int          do_verbose;
int          num_threads;
volatile int threads_running;
unsigned int do_wait =
    1; // Wait for this number of milliseconds before starting a cycle
unsigned int wait_delay = 4; // Wait before acknowledging sends with mx_wait()
                             // for this number of cycles times nsys

extern void* findSharedMemory( char* );

void
usage( )
{
    fprintf( stderr, "Usage: mx_dummy [args] -s sys_names \n" );
    fprintf( stderr, "-s - system names: \"x1x12 x1lsc x1asc\"\n" );
    fprintf( stderr, "-v - verbose\n" );
    fprintf( stderr, "-h - help\n" );
}

typedef struct blockProp
{
    unsigned int status;
    unsigned int timeSec;
    unsigned int timeNSec;
    unsigned int run;
    unsigned int cycle;
    unsigned int crc; /* block data CRC checksum */
} blockPropT;

struct rmIpcStr
{ /* IPC area structure                   */
    unsigned int cycle; /* Copy of latest cycle num from blocks */
    unsigned int dcuId; /* id of unit, unique within each site  */
    unsigned int crc; /* Configuration file's checksum        */
    unsigned int command; /* Allows DAQSC to command unit.        */
    unsigned int cmdAck; /* Allows unit to acknowledge DAQS cmd. */
    unsigned int request; /* DCU request of controller            */
    unsigned int reqAck; /* controller acknowledge of DCU req.   */
    unsigned int status; /* Status is set by the controller.     */
    unsigned int channelCount; /* Number of data channels in a DCU     */
    unsigned int dataBlockSize; /* Num bytes actually written by DCU within a
                                   1/16 data block */
    blockPropT
        bp[ DAQ_NUM_DATA_BLOCKS ]; /* An array of block property structures */
};

/* GDS test point table structure for FE to frame builder communication */
typedef struct cdsDaqNetGdsTpNum
{
    int count; /* test points count */
    int tpNum[ DAQ_GDS_MAX_TP_NUM ];
} cdsDaqNetGdsTpNum;

struct daqMXdata
{
    struct rmIpcStr   mxIpcData;
    cdsDaqNetGdsTpNum mxTpTable;
    char              mxDataBlock[ DAQ_DCU_BLOCK_SIZE ];
};
unsigned int
                                 nsys; // The number of mapped shared memories (number of data sources)
static struct rmIpcStr*          shmIpcPtr[ 128 ];
static char*                     shmDataPtr[ 128 ];
static struct cdsDaqNetGdsTpNum* shmTpTable[ 128 ];
static const int                 buf_size = DAQ_DCU_BLOCK_SIZE * 2;
static const int                 header_size =
    sizeof( struct rmIpcStr ) + sizeof( struct cdsDaqNetGdsTpNum );

class stats loop_stats;
class stats mx_wait_stats;
class stats mx_isend_stats;
class stats mx_total_stats;

void
shandler( int a )
{
    loop_stats.printDate( std::cerr );
    std::cerr << "loop:";
    loop_stats.println( std::cerr );
    std::cerr << "mx total:";
    mx_total_stats.println( std::cerr );
    std::cerr << "mx_wait():";
    mx_wait_stats.println( std::cerr );
    std::cerr << "mx_isend():";
    mx_total_stats.println( std::cerr );
}

void
reset_stats( int a )
{
    loop_stats.clearStats( );
    mx_wait_stats.clearStats( );
    mx_isend_stats.clearStats( );
    mx_total_stats.clearStats( );
}

static inline void
sender( int len, uint32_t match_val, uint16_t my_dcu )
{

    int              cur_req;
    struct daqMXdata mxDataBlock;
    int              myErrorSignal;
    // struct timeval start_time;
    char*        dataBuff;
    unsigned int myCrc = 0;

    mx_set_error_handler( MX_ERRORS_RETURN );

    int mxStatBit[ 2 ];
    mxStatBit[ 0 ] = 1;
    mxStatBit[ 1 ] = 2;
    // Clear CRC on startup to avoid sigfault if models not running
    for ( unsigned int i = 0; i < nsys; i++ )
        for ( unsigned int j = 0; j < 16; j++ )
            shmIpcPtr[ i ]->bp[ j ].crc = 0;

    do
    {
        int lastCycle = 0;

        myErrorSignal = 0;
        cur_req = 0;
        usleep( 1000000 );

        // Waity for cycle 0
        for ( ; shmIpcPtr[ 0 ]->cycle; )
            usleep( 1000 );
        lastCycle = 0;

        if ( !myErrorSignal )
        {
            do
            {
                int new_cycle;
                // Wait for cycle count update from FE
                do
                {
                    usleep( 1000 );
                    // printf("%d\n", shmIpcPtr[0]->cycle);
                    new_cycle = shmIpcPtr[ 0 ]->cycle;
                } while ( new_cycle == lastCycle );

                lastCycle++;
                lastCycle %= 16;

                loop_stats.tick( );
                usleep( do_wait * 1000 );

                // Send data for each system
                for ( unsigned int i = 0; i < nsys; i++ )
                {

                    if ( lastCycle == 0 )
                        shmIpcPtr[ i ]->reqAck ^= mxStatBit[ 0 ];
                    // Copy values from shmmem to MX buffer
                    mxDataBlock.mxIpcData.dataBlockSize =
                        shmIpcPtr[ i ]->dataBlockSize;
                    if ( mxDataBlock.mxIpcData.dataBlockSize >
                         DAQ_DCU_BLOCK_SIZE )
                        mxDataBlock.mxIpcData.dataBlockSize =
                            DAQ_DCU_BLOCK_SIZE;

                    /// Copy data from shmem to xmit buffer
                    dataBuff =
                        (char*)( shmDataPtr[ i ] + lastCycle * buf_size );
                    memcpy( (void*)&mxDataBlock.mxDataBlock[ 0 ],
                            dataBuff,
                            mxDataBlock.mxIpcData.dataBlockSize );
                    /// Calculate CRC checksum on data : NOTE
                    /// shmIpcPtr[i]->bp[lastCycle].crc contains crc length info
                    myCrc = crc_ptr( (char*)&mxDataBlock.mxDataBlock[ 0 ],
                                     shmIpcPtr[ i ]->bp[ lastCycle ].crc,
                                     0 );
                    myCrc =
                        crc_len( shmIpcPtr[ i ]->bp[ lastCycle ].crc, myCrc );
                    /// Send CRC back to DAQ shared memory
                    shmIpcPtr[ i ]->bp[ lastCycle ].crc = myCrc;
                    if ( lastCycle == 0 )
                        shmIpcPtr[ i ]->reqAck ^= mxStatBit[ 1 ];
                }

            } while ( !myErrorSignal );
        }
    } while ( 1 );
}

int
main( int argc, char** argv )
{
    char*        rem_host;
    char*        sysname;
    int          len;
    int          c;
    extern char* optarg;

#if DEBUG
    extern int mx_debug_mask;
    mx_debug_mask = 0xFFF;
#endif
    rem_host = NULL;
    sysname = NULL;
    len = DFLT_LEN;

    while ( ( c = getopt( argc, argv, "hd:e:f:n:b:r:s:l:W:Vvw:x" ) ) != EOF )
        switch ( c )
        {
        case 's':
            sysname = optarg;
            printf( "sysnames = %s\n", sysname );
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
        case 'v':
            do_verbose = 1;
            break;
        case 'h':
        default:
            usage( );
            exit( 1 );
        }

    if ( sysname == NULL )
    {
        usage( );
        exit( 1 );
    }

    printf( "System names: %s\n", sysname );

    if ( do_verbose )
        printf( "Starting streaming send to host %s\n", rem_host );
    nsys = 1;
    char* sname[ 128 ];
    sname[ 0 ] = strtok( sysname, " " );
    for ( ;; )
    {
        printf( "%s\n", sname[ nsys - 1 ] );
        char* s = strtok( 0, " " );
        if ( !s )
            break;
        sname[ nsys ] = s;
        nsys++;
    }

    // Map shared memory for all systems
    for ( unsigned int i = 0; i < nsys; i++ )
    {
        char shmem_fname[ 128 ];
        sprintf( shmem_fname, "%s_daq", sname[ i ] );
        void* dcu_addr = findSharedMemory( shmem_fname );
        if ( dcu_addr <= 0 )
        {
            fprintf( stderr, "Can't map shmem\n" );
            exit( 1 );
        }
        else
        {
            // printf("mapped at 0x%lx\n",(unsigned long)dcu_addr);
        }
        shmIpcPtr[ i ] =
            (struct rmIpcStr*)( (char*)dcu_addr + CDS_DAQ_NET_IPC_OFFSET );
        shmDataPtr[ i ] = (char*)( (char*)dcu_addr + CDS_DAQ_NET_DATA_OFFSET );
        shmTpTable[ i ] =
            (struct cdsDaqNetGdsTpNum*)( (char*)dcu_addr +
                                         CDS_DAQ_NET_GDS_TP_TABLE_OFFSET );
    }

    (void)signal( SIGHUP, shandler );
    (void)signal( SIGUSR1, reset_stats );

    len = sizeof( struct daqMXdata );
    fprintf( stderr, "send len = %d\n", len );
    fflush( stderr );

    sender( len, MATCH_VAL_MAIN, 0 );

    exit( 0 );
}

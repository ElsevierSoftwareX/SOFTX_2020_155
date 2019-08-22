
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sisci_types.h"
#include "sisci_api.h"
#include "sisci_error.h"
#include "sisci_demolib.h"
#include "testlib.h"
#include <malloc.h>
#include <signal.h>
#include <unistd.h>

#include "../drv/crc.c"
#include "../include/daq_core.h"

#define __CDECL

#define MY_DCU_OFFSET 0x00000
#define MY_IPC_OFFSET ( MY_DCU_OFFSET + 0x8000 )
#define MY_GDS_OFFSET ( MY_DCU_OFFSET + 0x9000 )
#define MY_DAT_OFFSET ( MY_DCU_OFFSET + 0xa000 )

#include "./dolphin_common.c"
extern void* findSharedMemorySize( char*, int );

static volatile int keepRunning = 1;

/*********************************************************************************/
/*                                U S A G E */
/*                                                                               */
/*********************************************************************************/

void
Usage( )
{
    printf( "Usage of dix_recv:\n" );
    printf( "dix_recv -g <group>\n" );
    printf( " -b <name>      : Destination buffer name (default local_dc)\n" );
    printf( " -m <value>     : Size in MB of the destination buffer [20-100] "
            "(default=100)\n" );
    printf( " -a <value>     : Local adapter number (default %d)\n",
            localAdapterNo );
    printf( " -g <value>     : Reflective group identifier (0..5))\n" );
    printf( " -v <value>     : Diagnostics level (0..1) \n" );
    printf( " -h             : This help screen\n" );
    printf( "\n" );
}

void
intHandler( int dummy )
{
    keepRunning = 0;
}

// **********************************************************************************************
void
print_diags2( int                   nsys,
              int                   lastCycle,
              int                   sendLength,
              daq_multi_dcu_data_t* ixDataBlock )
{
    // **********************************************************************************************
    int ii = 0;
    // Print diags in verbose mode
    printf( "\nTime = %d\t size = %d\n",
            ixDataBlock->header.dcuheader[ 0 ].timeSec,
            sendLength );
    printf( "Num DCU = %d\tMB per second = %d\n",
            nsys,
            ( sendLength * 16 / 1000000 ) );
    for ( ii = 0; ii < nsys; ii++ )
    {
        printf( "\t%d", ixDataBlock->header.dcuheader[ ii ].dcuId );
        printf( "\t%d", ixDataBlock->header.dcuheader[ ii ].timeSec );
        printf( "\t%d", ixDataBlock->header.dcuheader[ ii ].cycle );
        printf( "\t%d", ixDataBlock->header.dcuheader[ ii ].dataBlockSize );
        printf( "\t%d", ixDataBlock->header.dcuheader[ ii ].tpCount );
        printf( "\t%d", ixDataBlock->header.dcuheader[ ii ].tpBlockSize );
        printf( "\n" );
    }
    printf( "\n\n " );
}

/*********************************************************************************/
/*                                M A I N */
/*                                                                               */
/*********************************************************************************/

int __CDECL
    main( int argc, char* argv[] )
{
    int                       counter;
    int                       ii;
    char*                     myreadaddr;
    char*                     rcvDataPtr[ IX_BLOCK_COUNT ];
    daq_multi_dcu_data_t*     ixDataBlock;
    int                       sendLength = 0;
    daq_multi_cycle_header_t* rcvHeader[ IX_BLOCK_COUNT ];
    int                       myCrc;
    int                       do_verbose = 0;
    int                       max_data_size_mb = 100;
    int                       max_data_size = 0;
    char*                     dest_mbuf_name = "local_dc";

    printf( "\n %s compiled %s : %s\n\n", argv[ 0 ], __DATE__, __TIME__ );

    if ( argc < 2 )
    {
        printf( "Exiting here \n" );
        Usage( );
        return ( -1 );
    }

    /* Get the parameters */
    while ( ( counter = getopt( argc, argv, "m:g:a:v:b:" ) ) != EOF )
        switch ( counter )
        {
        case 'b':
            dest_mbuf_name = optarg;
            break;

        case 'm':
            max_data_size_mb = atoi( optarg );
            break;

        case 'g':
            segmentId = atoi( optarg );
            break;

        case 'a':
            localAdapterNo = atoi( optarg );
            break;

        case 'v':
            do_verbose = atoi( optarg );
            break;

        case 'h':
            printf( "Exiting here 2 \n" );
            Usage( );
            return ( 0 );
        }
    if ( max_data_size_mb < 20 || max_data_size_mb > 100 )
    {
        fprintf( stderr, "The data size parameter must be between 20 & 100\n" );
        exit( 1 );
    }
    max_data_size = max_data_size_mb * 1024 * 1024;

    // Attach to local shared memory
    char* ifo = (char*)findSharedMemorySize( dest_mbuf_name, max_data_size_mb );
    daq_multi_cycle_data_t* ifo_shm = (daq_multi_cycle_data_t*)ifo;
    // char *ifo_data = (char *)ifo + sizeof(daq_multi_cycle_header_t);
    char* ifo_data = (char*)&( ifo_shm->dataBlock[ 0 ] );
    int   cycle_data_size =
        ( max_data_size - sizeof( daq_multi_cycle_header_t ) ) /
        DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    cycle_data_size -= ( cycle_data_size % 8 );
    ifo_shm->header.cycleDataSize = cycle_data_size;
    ifo_shm->header.maxCycle = DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    printf( "cycle data size = %d\n", cycle_data_size );
    sleep( 3 );

    int   lastCycle = 15;
    int   nextCycle = 0;
    int   rcvBlockNum = 0;
    int   new_cycle = 0;
    int   nsys = 0;
    char* nextData;
    int   cyclesize = 0;

    // Connect to Dolphin
    error = dolphin_init( );
    printf(
        "Read = 0x%lx \n Write = 0x%lx \n", (long)readAddr, (long)writeAddr );

    // Set pointers to receive header and data in Dolphin network memory area
    myreadaddr = (char*)readAddr;
    for ( ii = 0; ii < IX_BLOCK_COUNT; ii++ )
    {
        rcvHeader[ ii ] = (daq_multi_cycle_header_t*)myreadaddr;
        rcvDataPtr[ ii ] =
            (char*)myreadaddr + sizeof( struct daq_multi_cycle_header_t );
        myreadaddr += IX_BLOCK_SIZE;
    }

    // Catch control C exit
    signal( SIGINT, intHandler );

    // Go into infinite receive loop
    do
    {
        // Wait for cycle couont update from FE
        // Check every 2 milliseconds
        do
        {
            usleep( 2000 );
            new_cycle = rcvHeader[ rcvBlockNum ]->curCycle;
        } while ( new_cycle != nextCycle && keepRunning );
        // Save cycle number of last received message
        lastCycle = new_cycle;
        cyclesize = rcvHeader[ rcvBlockNum ]->cycleDataSize;
        // Calculate the correct segment on the source buffer
        // rcvDataPtr = ((char *)myreadaddr + sizeof(daq_multi_cycle_header_t))
        // + new_cycle*cyclesize; Set up pointers to copy data to receive shmem
        nextData = (char*)ifo_data;
        nextData += cycle_data_size * new_cycle;
        ixDataBlock = (daq_multi_dcu_data_t*)nextData;
        sendLength = rcvHeader[ rcvBlockNum ]->cycleDataSize;
        // Copy data from Dolphin to local memory
        memcpy( nextData, rcvDataPtr[ rcvBlockNum ], sendLength );

        // Calculate CRC checksum of received data
        myCrc = crc_ptr( (char*)nextData, sendLength, 0 );
        myCrc = crc_len( sendLength, myCrc );

        // Write data header info to shared memory
        ifo_shm->header.curCycle = rcvHeader[ rcvBlockNum ]->curCycle;

        // Verify send CRC matches received CRC
        // if(ifo_header->msgcrc != myCrc)
        //   printf("Sent = %d and RCV = %d\n",ifo_header->msgcrc,myCrc);

        // Print some diagnostics
        // if(new_cycle == 0 && do_verbose)
        nsys = ixDataBlock->header.dcuTotalModels;
        nextCycle = ( new_cycle + 1 ) % 16;
        rcvBlockNum = ( rcvBlockNum + 1 ) % IX_BLOCK_COUNT;

        if ( new_cycle == 0 && do_verbose )
            print_diags2( nsys, new_cycle, sendLength, ixDataBlock );
    } while ( keepRunning );

    // Exit
    return SCI_ERR_OK;
}

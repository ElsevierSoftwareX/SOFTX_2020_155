/*********************************************************************************
 *                                                                               *
 * Copyright (C) 1993 - 2015 * Dolphin Interconnect Solutions AS *
 *                                                                               *
 * This program is free software; you can redistribute it and/or modify * it
 *under the terms of the GNU General Public License as published by          *
 * the Free Software Foundation; either version 2 of the License, * or (at your
 *option) any later version.                                        *
 *                                                                               *
 * This program is distributed in the hope that it will be useful, * but WITHOUT
 *ANY WARRANTY; without even the implied warranty of                *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the * GNU General
 *Public License for more details.                                  *
 *                                                                               *
 * You should have received a copy of the GNU General Public License * along
 *with this program; if not, write to the Free Software                   *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. *
 *                                                                               *
 *                                                                               *
 *********************************************************************************/

/*********************************************************************************/
/*                                                                               */
/* This program demonstrates the use of the following SISCI API function calls:
 */
/*                                                                               */
/*                                                                               */
/*      SCIInitialize() */
/*      SCITerminate() */
/*      SCIOpen() */
/*      SCIQuery() */
/*      SCICreateSegment() */
/*      SCIMapLocalSegment() */
/*      SCIMapRemoteSegment() */
/*      SCIConnectSegment() */
/*      SCISetSegmentAvailable() */
/*      SCISetSegmentUnavailable() */
/*      SCIMemCpy() */
/*      SCIUnmapSegment() */
/*      SCIDisconnectSegment() */
/*      SCIRemoveSegment() */
/*      SCIClose() */
/*                                                                               */
/*********************************************************************************/

// TODO:
// memcpy Server recvd data to mbuf
// See if DAQ can see it.
// Try test code first to see mbuf data.

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef OS_IS_VXWORKS
#include <time.h> /* Needed by VXWORKS */
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sisci_error.h"
#include "sisci_api.h"

#ifdef _WIN32
#define __CDECL __cdecl
#else
#define __CDECL
#endif

#define NO_CALLBACK NULL
#define NO_FLAGS 0
#define BLOCK_TRANSFER_READY 6

sci_error_t          error;
sci_desc_t           sd;
sci_local_segment_t  localSegment;
sci_remote_segment_t remoteSegment;
sci_map_t            localMap;
sci_map_t            remoteMap;
unsigned int         localSegmentId;
unsigned int         remoteSegmentId;
unsigned int         localAdapterNo = 0;
unsigned int         localNodeId = 0;
unsigned int         remoteNodeId = 0;
unsigned int         segmentSize = 8192;
unsigned int         offset = 0;
unsigned int         remoteOffset = 0;
unsigned int         client = 0;
unsigned int         server = 0;
unsigned int         errorCheck = 0;
unsigned int         memcpyFlag = NO_FLAGS;
void*                localMapAddr;
unsigned int         keyOffset = 0;

#include "../drv/crc.c"
#include "../include/daqmap.h"
#include "../drv/rfm.c"

#define CDS_DAQ_NET_IPC_OFFSET 0x0
#define CDS_DAQ_NET_GDS_TP_TABLE_OFFSET 0x1000
#define CDS_DAQ_NET_DATA_OFFSET 0x2000

#define MY_DCU_OFFSET 0x100000
#define MY_IPC_OFFSET ( MY_DCU_OFFSET + 0x8000 )
#define MY_GDS_OFFSET ( MY_DCU_OFFSET + 0x9000 )
#define MY_DAT_OFFSET ( MY_DCU_OFFSET + 0xa000 )

// extern void *findSharedMemory(char *);

static struct rmIpcStr*          shmIpcPtr[ 128 ];
static char*                     shmDataPtr[ 128 ];
static struct cdsDaqNetGdsTpNum* shmTpTable[ 128 ];
static const int                 header_size =
    sizeof( struct rmIpcStr ) + sizeof( struct cdsDaqNetGdsTpNum );
volatile char*   dxIpcPtr;
volatile char*   dxDataPtr;
volatile char*   dxGdsPtr;
volatile char*   dxDataBlk;
volatile char*   drIpcPtr;
volatile char*   drDataPtr;
volatile char*   drGdsPtr;
volatile char*   drDataBlk;
int*             drIntData;
static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;

/*********************************************************************************/
/*                               U S A G E */
/*                                                                               */
/*********************************************************************************/

void
Usage( )
{
    printf( "Usage of memcopy\n" );
    printf( "memcopy -rn <remote node-id> -client/server [ -a <adapter no> "
            "-size <segment size> ] \n\n" );
    printf( " -rn               : Remote node-id\n" );
    printf( " -client           : The local node is client\n" );
    printf( " -server           : The local node is server\n" );
    printf( " -a                : Local adapter number (default: %d)\n",
            localAdapterNo );
    printf( " -size             : Segment size         (default: %d)\n",
            segmentSize );
    printf( " -errcheck         : Check for errors     (default: no check)\n" );
    printf( " -ko <value>       : Needed when several tests in parallel "
            "(keyoffset)\n" );
    printf( " -help             : This helpscreen\n" );

    printf( "\n" );
}

/*********************************************************************************/
/*                   P R I N T   P A R A M E T E R S */
/*                                                                               */
/*********************************************************************************/

void
PrintParameters( unsigned int client,
                 unsigned int localNodeId,
                 unsigned int remoteNodeId,
                 unsigned int localAdapterNo,
                 unsigned int segmentSize,
                 unsigned int errcheck )
{

    printf( "Test parameters for %s \n", ( client ) ? "client" : "server" );
    printf( "-----------------------------\n" );
    printf( "Local node-id      : %d\n", localNodeId );
    printf( "Remote node-id     : %d\n", remoteNodeId );
    printf( "Local adapter no.  : %d\n", localAdapterNo );
    printf( "Segment size       : %d\n", segmentSize );
    printf( "Error check        : %s\n",
            ( errcheck ) ? "Enabled" : "Disabled" );
    printf( "-----------------------------\n\n" );
}

/*********************************************************************************/
/*                              S L E E P */
/*                                                                               */
/*********************************************************************************/

void
SleepMilliseconds( int milliseconds )
{
#if defined( _WIN32 )
    Sleep( milliseconds );
#elif defined( OS_IS_VXWORKS )
    /* CLOCKS_PER_SEC Numer of ticks pr second) */
    taskDelay( 1000 / CLOCKS_PER_SEC * milliseconds );
#else

    if ( milliseconds < 1000 )
    {
        usleep( 1000 * milliseconds );
    }
    else if ( milliseconds % 1000 == 0 )
    {
        sleep( milliseconds / 1000 );
    }
    else
    {
        usleep( 1000 * ( milliseconds % 1000 ) );
        sleep( milliseconds / 1000 );
    }
#endif
}

/*********************************************************************************/
/*                    S E N D   O N E   I N T E R R U P T */
/*                                                                               */
/*********************************************************************************/

sci_error_t
SendOneInterrupt( sci_desc_t   sd,
                  unsigned int localAdapterNo,
                  unsigned int localNodeId,
                  unsigned int remoteNodeId,
                  unsigned int interruptNo )
{

    sci_error_t            error;
    sci_remote_interrupt_t remoteInterrupt;
    unsigned int           connTimeout;

    /* Now connect to the other sides interrupt flag */
    connTimeout = 500;
    do
    {
        SCIConnectInterrupt( sd,
                             &remoteInterrupt,
                             remoteNodeId,
                             localAdapterNo,
                             interruptNo,
                             SCI_INFINITE_TIMEOUT,
                             NO_FLAGS,
                             &error );
        connTimeout--;
        SleepMilliseconds( 10 );
    } while ( error != SCI_ERR_OK && connTimeout > 0 );

    if ( connTimeout == 0 )
    {
        return SCI_ERR_CONNECTION_REFUSED;
    }

    /* Trigg interrupt */
    printf( "Node %u triggering interrupt\n", localNodeId );
    SCITriggerInterrupt( remoteInterrupt, NO_FLAGS, &error );
    if ( error != SCI_ERR_OK )
    {
        fprintf(
            stderr, "SCITriggerInterrupt failed - Error code 0x%x\n", error );
        return error;
    }

    /* Disconnect and remove interrupts */
    SCIDisconnectInterrupt( remoteInterrupt, NO_FLAGS, &error );
    if ( error != SCI_ERR_OK )
    {
        fprintf( stderr,
                 "SCIDisconnectInterrupt failed - Error code 0x%x\n",
                 error );
        return error;
    }

    return SCI_ERR_OK;
}

/*********************************************************************************/
/*                  R E C E I V E   O N E   I N T E R R U P T */
/*                                                                               */
/*********************************************************************************/

sci_error_t
ReceiveOneInterrupt( sci_desc_t   sd,
                     unsigned int localAdapterNo,
                     unsigned int localNodeId,
                     unsigned int interruptNo )
{

    sci_error_t           error;
    sci_local_interrupt_t localInterrupt;
    int                   paranoia = 2000;

    /* Create an interrupt */
    SCICreateInterrupt( sd,
                        &localInterrupt,
                        localAdapterNo,
                        &interruptNo,
                        NULL,
                        NULL,
                        SCI_FLAG_FIXED_INTNO,
                        &error );
    if ( error != SCI_ERR_OK )
    {
        fprintf(
            stderr, "SCICreateInterrupt failed - Error code 0x%x\n", error );
        return error;
    }

    /* Wait for a interrupt */
    do
    {
        SCIWaitForInterrupt(
            localInterrupt, SCI_INFINITE_TIMEOUT, NO_FLAGS, &error );
        if ( error == SCI_ERR_OK )
            break;
        printf( "\nNode %u wait for interrupt cancelled\n", localNodeId );
    } while ( 1 );

    printf( "\nNode %u received interrupt (0x%x)\n", localNodeId, interruptNo );

    /* Remove interrupt */
    do
    {
        SCIRemoveInterrupt( localInterrupt, NO_FLAGS, &error );
        SleepMilliseconds( 1 );
    } while ( ( error == SCI_ERR_BUSY ) && ( paranoia-- > 0 ) );

    if ( error != SCI_ERR_OK )
    {
        fprintf(
            stderr, "SCIRemoveInterrupt failed - Error code 0x%x\n", error );
        return error;
    }

    return SCI_ERR_OK;
}

/*********************************************************************************/
/*                          C L I E N T   N O D E */
/*                                                                               */
/*********************************************************************************/

sci_error_t
ClientNode( void )
{

    sci_sequence_t sequence = NULL;
    sci_error_t    error;

#if 0
    /* Create local segment */
    SCICreateSegment(sd,&localSegment,localSegmentId, segmentSize, NO_CALLBACK, NULL, NO_FLAGS,&error);
    if (error == SCI_ERR_OK) {
       printf("Local segment (id=0x%x, size=%d) is created. \n", localSegmentId, segmentSize);  
    } else {
       fprintf(stderr,"SCICreateSegment failed - Error code 0x%x\n",error);
       return error;
    }


    /* Prepare the segment */
    SCIPrepareSegment(localSegment,localAdapterNo,NO_FLAGS,&error);
    if (error == SCI_ERR_OK) {
        printf("Local segment (id=0x%x, size=%d) is created. \n", localSegmentId, segmentSize);  
    } else {
        fprintf(stderr,"SCIPrepareSegment failed - Error code 0x%x\n",error);
        return error;
    }

    /* Map local segment to user space */
    localMapAddr = SCIMapLocalSegment(localSegment,&localMap, offset,segmentSize, NULL,NO_FLAGS,&error);
    // localMapAddr = SCIMapLocalSegment(localSegment,&localMap, 2048,segmentSize, NULL,NO_FLAGS,&error);
    if (error == SCI_ERR_OK) {
        printf("Local segment (id=0x%x) is mapped to user space.\n", localSegmentId); 
    } else {
        fprintf(stderr,"SCIMapLocalSegment failed - Error code 0x%x\n",error);
        return error;
    } 


    /* Set the segment available */
    SCISetSegmentAvailable(localSegment, localAdapterNo, NO_FLAGS, &error);
    if (error == SCI_ERR_OK) {
        printf("Local segment (id=0x%x) is available for remote connections. \n", localSegmentId); 
    } else {
        fprintf(stderr,"SCISetSegmentAvailable failed - Error code 0x%x\n",error);
        return error;
    }

#endif

    /* Connect to remote segment */
    printf( "Connect to remote segment (id=0x%x) .... ", remoteSegmentId );
    do
    {
        SCIConnectSegment( sd,
                           &remoteSegment,
                           remoteNodeId,
                           remoteSegmentId,
                           localAdapterNo,
                           NO_CALLBACK,
                           NULL,
                           SCI_INFINITE_TIMEOUT,
                           NO_FLAGS,
                           &error );

    } while ( error != SCI_ERR_OK );

    printf( "connected\n" );

    int remoteSize = SCIGetRemoteSegmentSize( remoteSegment );
    printf( "Remote segment (id=0x%x) is connected with size of %d.\n",
            remoteSegmentId,
            remoteSize );

    volatile unsigned char* writeAddr;

    /* Map remote segment to user space */
    writeAddr = SCIMapRemoteSegment(
        remoteSegment, &remoteMap, offset, remoteSize, NULL, NO_FLAGS, &error );
    if ( error == SCI_ERR_OK )
    {
        printf( "Remote segment (id=0x%x) is mapped to user space. \n",
                localSegmentId );
    }
    else
    {
        fprintf(
            stderr, "SCIMapRemoteSegment failed - Error code 0x%x\n", error );
        return error;
    }

    if ( errorCheck )
    {
        memcpyFlag = SCI_FLAG_ERROR_CHECK;
        /* Create a sequence for data error checking */
        SCICreateMapSequence( remoteMap, &sequence, 0, &error );
        if ( error != SCI_ERR_OK )
        {
            fprintf( stderr,
                     "SCICreateMapSequence failed - Error code 0x%x\n",
                     error );
            return error;
        }
    }

    printf( "\n-- Start the data transfer -- \n\n\n" );
    /* Transfer the data from the local segment to the remote segment */
    int  mydata[ 256 ];
    int  ii;
    int* mydataPtr;

    for ( ii = 0; ii < 256; ii++ )
        mydata[ ii ] = ii;
    mydataPtr = (int*)&mydata[ 0 ];

    unsigned char* dataBuff;
    dxIpcPtr = (unsigned char*)( writeAddr + MY_IPC_OFFSET );
    dxGdsPtr = (unsigned char*)( writeAddr + MY_GDS_OFFSET );
    dxDataPtr = (unsigned char*)( writeAddr + MY_DAT_OFFSET );

    char sname[ 128 ];
    sprintf( sname, "%s_daq", "x1lscaux" );
    void* dcu_addr = findSharedMemory( sname );
    if ( dcu_addr <= 0 )
    {
        fprintf( stderr, "Can't map shmem\n" );
        exit( 1 );
    }
    else
    {
        printf( "mapped at 0x%lx\n", (unsigned long)dcu_addr );
    }
    shmIpcPtr[ 0 ] =
        (struct rmIpcStr*)( (char*)dcu_addr + CDS_DAQ_NET_IPC_OFFSET );
    shmDataPtr[ 0 ] = (char*)( (char*)dcu_addr + CDS_DAQ_NET_DATA_OFFSET );
    shmTpTable[ 0 ] =
        (struct cdsDaqNetGdsTpNum*)( (char*)dcu_addr +
                                     CDS_DAQ_NET_GDS_TP_TABLE_OFFSET );

    for ( ; shmIpcPtr[ 0 ]->cycle; )
        usleep( 1000 );
    int lastCycle = 0;
    printf( "Found cycle zero \n" );
    int new_cycle;

    for ( ii = 0; ii < 44; ii++ )
    {

        // Wait for cycle count update from FE
        do
        {
            usleep( 1000 );
            new_cycle = shmIpcPtr[ 0 ]->cycle;
        } while ( new_cycle == lastCycle );

        lastCycle = new_cycle;

        drIntData = (int*)shmDataPtr[ 0 ];
        drIntData += 2;

        SCIMemCpy( sequence,
                   shmTpTable[ 0 ],
                   remoteMap,
                   MY_GDS_OFFSET,
                   sizeof( struct cdsDaqNetGdsTpNum ),
                   memcpyFlag,
                   &error );
        if ( error != SCI_ERR_OK )
        {
            fprintf( stderr, "SCIMemCpy failed - Error code 0x%x\n", error );
            return error;
        }
        // printf("Copy DAQ Data Block size =
        // %d\n",shmIpcPtr[0]->dataBlockSize);

        dataBuff = (unsigned char*)shmDataPtr[ 0 ] + ( lastCycle * buf_size );
        // SCIMemCpy(sequence,shmDataPtr[0],
        // remoteMap,MY_DAT_OFFSET,shmIpcPtr[0]->dataBlockSize,memcpyFlag,&error);
        SCIMemCpy( sequence,
                   dataBuff,
                   remoteMap,
                   MY_DAT_OFFSET,
                   shmIpcPtr[ 0 ]->dataBlockSize,
                   memcpyFlag,
                   &error );
        if ( error != SCI_ERR_OK )
        {
            fprintf( stderr, "SCIMemCpy failed - Error code 0x%x\n", error );
            return error;
        }

#if 0
    SCIMemCpy(sequence,mydataPtr, remoteMap,remoteOffset,sizeof(mydata),memcpyFlag,&error);
    if (error == SCI_ERR_OK) {
        printf("The data segment is transferred from the local node %d to the remote node %d\n\n\n",
                localNodeId,
                remoteNodeId); 
    } else {
        fprintf(stderr,"SCIMemCpy failed - Error code 0x%x\n",error);
        return error; 
    }
#endif

        SCIMemCpy( sequence,
                   shmIpcPtr[ 0 ],
                   remoteMap,
                   MY_IPC_OFFSET,
                   sizeof( struct rmIpcStr ),
                   memcpyFlag,
                   &error );
        if ( error == SCI_ERR_OK )
        {
            printf( "Sending CPU value of %d on cycle %d\n",
                    *drIntData,
                    lastCycle );
        }
        else
        {
            fprintf( stderr, "SCIMemCpy failed - Error code 0x%x\n", error );
            return error;
        }
    }

    /* Send an interrupt to remote node telling that the memcopy is ready */
    error = SendOneInterrupt(
        sd, localAdapterNo, localNodeId, remoteNodeId, BLOCK_TRANSFER_READY );
    if ( error == SCI_ERR_OK )
    {
        printf( "\nInterrupt message sent to remote node\n" );
    }
    else
    {
        printf( "\nInterrupt synchronization failed\n" );
        return error;
    }

    if ( errorCheck )
    {
        /* Remove the Sequence */
        SCIRemoveSequence( sequence, NO_FLAGS, &error );
        if ( error != SCI_ERR_OK )
        {
            fprintf(
                stderr, "SCIRemoveSequence failed - Error code 0x%x\n", error );
            return error;
        }
    }

    /* Unmap remote segment */
    SCIUnmapSegment( remoteMap, NO_FLAGS, &error );
    if ( error == SCI_ERR_OK )
    {
        printf( "The remote segment is unmapped\n" );
    }
    else
    {
        fprintf( stderr, "SCIUnmapSegment failed - Error code 0x%x\n", error );
        return error;
    }

    /* Disconnect segment */
    SCIDisconnectSegment( remoteSegment, NO_FLAGS, &error );
    if ( error == SCI_ERR_OK )
    {
        printf( "The segment is disconnected\n" );
    }
    else
    {
        fprintf(
            stderr, "SCIDisconnectSegment failed - Error code 0x%x\n", error );
        return error;
    }

#if 0
    /* Set local segment unavilable */
    SCISetSegmentUnavailable(localSegment, localAdapterNo, NO_FLAGS, &error);
    if (error == SCI_ERR_OK) {
        printf("The local segment is set to unavailable\n"); 
    } else {
        fprintf(stderr,"SCISetSegmentUnavailable failed - Error code 0x%x\n",error);
        return error;
    }


    /* Unmap local segment */
    SCIUnmapSegment(localMap,NO_FLAGS,&error);
    if (error == SCI_ERR_OK) {
        printf("The local segment is unmapped\n"); 
    } else {
        fprintf(stderr,"SCIUnmapSegment failed - Error code 0x%x\n",error);
        return error;
    }


    /* Remove local segment */
    SCIRemoveSegment(localSegment,NO_FLAGS,&error);
    if (error == SCI_ERR_OK) {
        printf("The local segment is removed\n"); 
    } else {
        fprintf(stderr,"SCIRemoveSegment failed - Error code 0x%x\n",error);
        return error; 
    }
#endif

    return SCI_ERR_OK;
}

/*********************************************************************************/
/*                          S E R V E R   N O D E */
/*                                                                               */
/*********************************************************************************/

unsigned int
ServerNode( void )
{

    sci_error_t error;

    /* Create local segment */
    SCICreateSegment( sd,
                      &localSegment,
                      localSegmentId,
                      segmentSize,
                      NO_CALLBACK,
                      NULL,
                      NO_FLAGS,
                      &error );
    // SCICreateSegment(sd,&localSegment,localSegmentId, segmentSize,
    // NO_CALLBACK, NULL, SCI_FLAG_BROADCAST,&error);
    if ( error == SCI_ERR_OK )
    {
        printf( "Local segment (id=0x%x, size=%d) is created. \n",
                localSegmentId,
                segmentSize );
    }
    else
    {
        fprintf( stderr, "SCICreateSegment failed - Error code 0x%x\n", error );
        return error;
    }

    /* Prepare the segment */
    SCIPrepareSegment( localSegment, localAdapterNo, NO_FLAGS, &error );
    // SCIPrepareSegment(localSegment,localAdapterNo,SCI_FLAG_BROADCAST,&error);
    if ( error == SCI_ERR_OK )
    {
        printf( "Local segment (id=0x%x, size=%d) is created. \n",
                localSegmentId,
                segmentSize );
    }
    else
    {
        fprintf(
            stderr, "SCIPrepareSegment failed - Error code 0x%x\n", error );
        return error;
    }

    volatile int* readAddr;

    /* Map local segment to user space */
    readAddr = SCIMapLocalSegment(
        localSegment, &localMap, offset, segmentSize, NULL, NO_FLAGS, &error );
    if ( error == SCI_ERR_OK )
    {
        printf( "Local segment (id=0x%x) is mapped to user space.\n",
                localSegmentId );
    }
    else
    {
        fprintf(
            stderr, "SCIMapLocalSegment failed - Error code 0x%x\n", error );
        return error;
    }

    /* Set the segment available */
    SCISetSegmentAvailable( localSegment, localAdapterNo, NO_FLAGS, &error );
    if ( error == SCI_ERR_OK )
    {
        printf(
            "Local segment (id=0x%x) is available for remote connections. \n",
            localSegmentId );
    }
    else
    {
        fprintf( stderr,
                 "SCISetSegmentAvailable failed - Error code 0x%x\n",
                 error );
        return error;
    }

    printf( "Wait for the data transfer ....." );

    int            ii;
    int            lastCycle = 0;
    unsigned char *dataBuffDx, *dataBuffLc;
    char           sname[ 128 ];
    sprintf( sname, "%s_daq", "x1lscaux" );
    void* dcu_addr = findSharedMemory( sname );
    if ( dcu_addr <= 0 )
    {
        fprintf( stderr, "Can't map shmem\n" );
        exit( 1 );
    }
    else
    {
        printf( "mapped at 0x%lx\n", (unsigned long)dcu_addr );
    }
    shmIpcPtr[ 2 ] =
        (struct rmIpcStr*)( (char*)dcu_addr + CDS_DAQ_NET_IPC_OFFSET );
    shmDataPtr[ 2 ] = (char*)( (char*)dcu_addr + CDS_DAQ_NET_DATA_OFFSET );
    shmTpTable[ 2 ] =
        (struct cdsDaqNetGdsTpNum*)( (char*)dcu_addr +
                                     CDS_DAQ_NET_GDS_TP_TABLE_OFFSET );

    shmIpcPtr[ 1 ] = (struct rmIpcStr*)( (char*)readAddr + MY_IPC_OFFSET );
    shmTpTable[ 1 ] =
        (struct cdsDaqNetGdsTpNum*)( (char*)readAddr + MY_GDS_OFFSET );
    shmDataPtr[ 1 ] = (char*)( (char*)readAddr + MY_DAT_OFFSET );
    drGdsPtr = (unsigned char*)( readAddr + MY_GDS_OFFSET );
    drDataPtr = (unsigned char*)readAddr + MY_DAT_OFFSET;
    drIntData = (int*)drDataPtr;
    drIntData += 6;

    for ( ii = 0; ii < 33; ii++ )
    {
        do
        {
            usleep( 1000 );
        } while ( shmIpcPtr[ 1 ]->cycle == lastCycle );
        lastCycle = shmIpcPtr[ 1 ]->cycle;
        dataBuffDx = (unsigned char*)shmDataPtr[ 1 ] + ( lastCycle * buf_size );
        dataBuffLc = (unsigned char*)shmDataPtr[ 2 ] + ( lastCycle * buf_size );
        memcpy( dataBuffLc, dataBuffDx, shmIpcPtr[ 1 ]->dataBlockSize );
        memcpy( shmTpTable[ 2 ],
                shmTpTable[ 1 ],
                sizeof( struct cdsDaqNetGdsTpNum ) );
        memcpy( shmIpcPtr[ 2 ], shmIpcPtr[ 1 ], sizeof( struct rmIpcStr ) );
        printf( "Server received CPU METER = %d on cycle %d \n\n",
                *drIntData,
                shmIpcPtr[ 1 ]->cycle );
        printf( "Data size is %d\n", shmIpcPtr[ 1 ]->dataBlockSize );
    }

    /* Wait for interrupt signal telling that memcopy is ready */
    error = ReceiveOneInterrupt(
        sd, localAdapterNo, localNodeId, BLOCK_TRANSFER_READY );
    if ( error == SCI_ERR_OK )
    {
        printf( "\nMemcopy is ready\n" );
    }
    else
    {
        printf( "\nInterrupt synchronization failed\n" );
        return error;
    }

    /* Set local segment unavilable */
    SCISetSegmentUnavailable( localSegment, localAdapterNo, NO_FLAGS, &error );
    if ( error == SCI_ERR_OK )
    {
        printf( "The local segment is set to unavailable\n" );
    }
    else
    {
        fprintf( stderr,
                 "SCISetSegmentUnavailable failed - Error code 0x%x\n",
                 error );
        return error;
    }

    /* Unmap local segment */
    SCIUnmapSegment( localMap, NO_FLAGS, &error );
    if ( error == SCI_ERR_OK )
    {
        printf( "The local segment is unmapped\n" );
    }
    else
    {
        fprintf( stderr, "SCIUnmapSegment failed - Error code 0x%x\n", error );
        return error;
    }

    /* Remove local segment */
    SCIRemoveSegment( localSegment, NO_FLAGS, &error );
    if ( error == SCI_ERR_OK )
    {
        printf( "The local segment is removed\n" );
    }
    else
    {
        fprintf( stderr, "SCIRemoveSegment failed - Error code 0x%x\n", error );
        return error;
    }

    return SCI_ERR_OK;
}

/*********************************************************************************/
/*                              M A I N */
/*                                                                               */
/*********************************************************************************/

int __CDECL
    main( int argc, char* argv[] )
{

    int counter;

    printf( "%s compiled %s : %s\n\n", argv[ 0 ], __DATE__, __TIME__ );

    if ( argc < 3 )
    {
        Usage( );
        return ( -1 );
    }

    /* Get the parameters */
    for ( counter = 1; counter < argc; counter++ )
    {

        if ( !strcmp( "-rn", argv[ counter ] ) )
        {
            remoteNodeId = strtol( argv[ counter + 1 ], (char**)NULL, 10 );
            continue;
        }

        if ( !strcmp( "-ko", argv[ counter ] ) )
        {
            /*LINTED*/
            keyOffset = strtol( argv[ counter + 1 ], (char**)NULL, 10 );
            continue;
        }

        if ( !strcmp( "-size", argv[ counter ] ) )
        {
            segmentSize = strtol( argv[ counter + 1 ], (char**)NULL, 10 );
            continue;
        }

        if ( !strcmp( "-a", argv[ counter ] ) )
        {
            localAdapterNo = strtol( argv[ counter + 1 ], (char**)NULL, 10 );
            continue;
        }

        if ( !strcmp( "-client", argv[ counter ] ) )
        {
            client = 1;
            continue;
        }

        if ( !strcmp( "-server", argv[ counter ] ) )
        {
            server = 1;
            continue;
        }

        if ( !strcmp( "-errcheck", argv[ counter ] ) )
        {
            errorCheck = 1;
            continue;
        }

        if ( !strcmp( "-help", argv[ counter ] ) )
        {
            Usage( );
            return ( 0 );
        }
    }

    if ( remoteNodeId == 0 )
    {
        fprintf(
            stderr,
            "Remote node-id is not specified. Use -rn <remote node-id>\n" );
        return ( -1 );
    }

    if ( server == 0 && client == 0 )
    {
        fprintf( stderr, "You must specify a client node or a server node\n" );
        return ( -1 );
    }

    if ( server == 1 && client == 1 )
    {
        fprintf( stderr, "Both server node and client node is selected.\n" );
        fprintf( stderr,
                 "You must specify either a client or a server node\n" );
        return ( -1 );
    }

    /* Initialize the SISCI library */
    SCIInitialize( NO_FLAGS, &error );
    if ( error != SCI_ERR_OK )
    {
        fprintf( stderr, "SCIInitialize failed - Error code: 0x%x\n", error );
        return ( -1 );
    }

    /* Open a file descriptor */
    SCIOpen( &sd, NO_FLAGS, &error );
    if ( error != SCI_ERR_OK )
    {
        if ( error == SCI_ERR_INCONSISTENT_VERSIONS )
        {
            fprintf( stderr,
                     "Version mismatch between SISCI user library and SISCI "
                     "driver\n" );
        }
        fprintf( stderr, "SCIOpen failed - Error code 0x%x\n", error );
        SCITerminate( );
        return ( -1 );
    }

    /* Get local nodeId */
    SCIGetLocalNodeId( localAdapterNo, &localNodeId, NO_FLAGS, &error );

    if ( error != SCI_ERR_OK )
    {
        fprintf(
            stderr, "Could not find the local adapter %d\n", localAdapterNo );
        SCIClose( sd, NO_FLAGS, &error );
        SCITerminate( );
        return ( -1 );
    }

    /* Print out parameters */
    PrintParameters( client,
                     localNodeId,
                     remoteNodeId,
                     localAdapterNo,
                     segmentSize,
                     errorCheck );

    /* Create a segmentId */
    // localSegmentId  = (localNodeId << 16)  | remoteNodeId | keyOffset;
    // remoteSegmentId = (remoteNodeId << 16) | localNodeId | keyOffset;

    localSegmentId = ( localNodeId << 16 ) | 10;
    remoteSegmentId = ( 12 << 16 ) | 10;

    if ( client )
    {
        error = ClientNode( );
        if ( error != SCI_ERR_OK )
        {
            fprintf( stderr, "Client node failed - Error = 0x%x\n", error );
            SCIClose( sd, NO_FLAGS, &error );
            SCITerminate( );
            return ( -1 );
        }
    }
    else
    {
        error = ServerNode( );
        if ( error != SCI_ERR_OK )
        {
            fprintf( stderr, "Server node failed - Error = 0x%x\n", error );
            SCIClose( sd, NO_FLAGS, &error );
            SCITerminate( );
            return ( -1 );
        }
    }

    /* Close sci descriptor */
    SCIClose( sd, NO_FLAGS, &error );
    if ( error != SCI_ERR_OK )
    {
        fprintf( stderr, "SCIClose failed - Error code: 0x%x\n", error );
        SCITerminate( );
        return ( -1 );
    }

    /* Free allocated resources */
    SCITerminate( );

    return 0;
}

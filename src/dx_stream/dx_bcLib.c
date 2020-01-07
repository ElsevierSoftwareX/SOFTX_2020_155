#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sisci_types.h"
#include "sisci_api.h"
#include "sisci_error.h"
#include "sisci_demolib.h"
#include "testlib.h"
#include <malloc.h>
#include "./dx.h"

#define __CDECL

#define NO_CALLBACK NULL
#define NO_FLAGS 0
#define DATA_TRANSFER_READY 8
#define CMD_READY 1234

/* Use upper 4 KB of segment for synchronization. */
#define SYNC_OFFSET ( ( segmentSize ) / 4 - 1024 )

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

#if 0
sci_desc_t              sd;
sci_map_t               remoteMap;
unsigned int            localAdapterNo = 0;
unsigned int            remoteNodeId   = 0;
unsigned int            localNodeId    = 0;
unsigned int		segmentSize 	= 4000000;
unsigned int segmentId = 2;
#endif

sci_error_t
dx_init( DX_INFO* dxinfo )
{
    sci_error_t  error;
    sci_desc_t   sd;
    unsigned int localAdapterNo;
    unsigned int localNodeId = 0;

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
        return ( -1 );
    }

    dxinfo->sd = sd;
    localAdapterNo = dxinfo->localAdapterNo;

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

    dxinfo->localNodeId = localNodeId;
    return SCI_ERR_OK;
}

volatile void*
dx_attach_local_segment( DX_INFO* dxinfo )
{
    volatile void*      readAddr;
    sci_error_t         error;
    sci_local_segment_t localSegment;
    sci_map_t           localMap;
    unsigned int        offset = 0;

    /* Create local reflective memory segment */
    SCICreateSegment( dxinfo->sd,
                      &localSegment,
                      dxinfo->segmentId,
                      dxinfo->segmentSize,
                      NO_CALLBACK,
                      NULL,
                      SCI_FLAG_BROADCAST,
                      &error );
    if ( error == SCI_ERR_OK )
    {
        printf( "Local segment (id=0x%x, size=%d) is created. \n",
                dxinfo->segmentId,
                dxinfo->segmentSize );
    }
    else
    {
        fprintf( stderr, "SCICreateSegment failed - Error code 0x%x\n", error );
        return NULL;
    }

    dxinfo->localSegment = localSegment;

    /* Prepare the segment */
    SCIPrepareSegment(
        localSegment, dxinfo->localAdapterNo, SCI_FLAG_BROADCAST, &error );

    if ( error == SCI_ERR_OK )
    {
        printf( "Local segment (id=0x%x, size=%d) is prepared. \n",
                dxinfo->segmentId,
                dxinfo->segmentSize );
    }
    else
    {
        fprintf(
            stderr, "SCIPrepareSegment failed - Error code 0x%x\n", error );
        return NULL;
    }

    /* Map local segment to user space - this is the address to read back data
     * from the reflective memory region */
    readAddr = (unsigned char*)SCIMapLocalSegment( localSegment,
                                                   &localMap,
                                                   offset,
                                                   dxinfo->segmentSize,
                                                   NULL,
                                                   NO_FLAGS,
                                                   &error );
    if ( error == SCI_ERR_OK )
    {
        printf( "Local segment (id=0x%x) is mapped to user space at 0x%lx\n",
                dxinfo->segmentId,
                (unsigned long)readAddr );
    }
    else
    {
        fprintf(
            stderr, "SCIMapLocalSegment failed - Error code 0x%x\n", error );
        return NULL;
    }

    dxinfo->localMap = localMap;

    /* Set the segment available */
    SCISetSegmentAvailable(
        localSegment, dxinfo->localAdapterNo, NO_FLAGS, &error );
    if ( error == SCI_ERR_OK )
    {
        printf(
            "Local segment (id=0x%x) is available for remote connections. \n",
            dxinfo->segmentId );
    }
    else
    {
        fprintf( stderr,
                 "SCISetSegmentAvailable failed - Error code 0x%x\n",
                 error );
        return NULL;
    }

    return readAddr;
}

volatile void*
dx_attach_remote_segment( DX_INFO* dxinfo )
{
    sci_remote_segment_t remoteSegment;
    unsigned int         remoteNodeId;
    volatile void*       writeAddr;
    sci_error_t          error;
    sci_sequence_t       sequence = NULL;
    unsigned int         offset = 0;
    sci_map_t            remoteMap;

    remoteNodeId = DIS_BROADCAST_NODEID_GROUP_ALL;

    /* Connect to remote segment */
    printf( "Connect to remote segment .... " );

    do
    {
        SCIConnectSegment( dxinfo->sd,
                           &remoteSegment,
                           remoteNodeId,
                           dxinfo->segmentId,
                           dxinfo->localAdapterNo,
                           NO_CALLBACK,
                           NULL,
                           SCI_INFINITE_TIMEOUT,
                           SCI_FLAG_BROADCAST,
                           &error );

        sleep( 1 );

    } while ( error != SCI_ERR_OK );

    int remoteSize = SCIGetRemoteSegmentSize( remoteSegment );
    printf( "Remote segment (id=0x%x) is connected with size %d.\n",
            dxinfo->segmentId,
            remoteSize );
    dxinfo->remoteSegment = remoteSegment;

    /* Map remote segment to user space */
    writeAddr = (unsigned char*)SCIMapRemoteSegment( remoteSegment,
                                                     &remoteMap,
                                                     offset,
                                                     dxinfo->segmentSize,
                                                     NULL,
                                                     SCI_FLAG_BROADCAST,
                                                     &error );
    if ( error == SCI_ERR_OK )
    {
        printf( "Remote segment (id=0x%x) is mapped to user space. \n",
                dxinfo->segmentId );
        dxinfo->remoteMap = remoteMap;
    }
    else
    {
        fprintf(
            stderr, "SCIMapRemoteSegment failed - Error code 0x%x\n", error );
        return NULL;
    }
    /* Create a sequence for data error checking*/
    SCICreateMapSequence( remoteMap, &sequence, NO_FLAGS, &error );
    if ( error != SCI_ERR_OK )
    {
        fprintf(
            stderr, "SCICreateMapSequence failed - Error code 0x%x\n", error );
        return NULL;
    }
    dxinfo->sequence = sequence;
    return writeAddr;
}

sci_error_t
dx_cleanup( DX_INFO* dxinfo )
{
    sci_error_t error;

    if ( dxinfo->mode != DX_RCVR_ONLY )
    {
        /* Remove the Sequence */
        SCIRemoveSequence( dxinfo->sequence, NO_FLAGS, &error );
        if ( error != SCI_ERR_OK )
        {
            fprintf(
                stderr, "SCIRemoveSequence failed - Error code 0x%x\n", error );
            return error;
        }
    }

    if ( dxinfo->mode != DX_SEND_ONLY )
    {
        /* Unmap local segment */
        SCIUnmapSegment( dxinfo->localMap, NO_FLAGS, &error );

        if ( error == SCI_ERR_OK )
        {
            printf( "The local segment is unmapped\n" );
        }
        else
        {
            fprintf(
                stderr, "SCIUnmapSegment failed - Error code 0x%x\n", error );
            return error;
        }

        /* Remove local segment */
        SCIRemoveSegment( dxinfo->localSegment, NO_FLAGS, &error );
        if ( error == SCI_ERR_OK )
        {
            printf( "The local segment is removed\n" );
        }
        else
        {
            fprintf(
                stderr, "SCIRemoveSegment failed - Error code 0x%x\n", error );
            return error;
        }
    }

    if ( dxinfo->mode != DX_RCVR_ONLY )
    {
        /* Unmap remote segment */
        SCIUnmapSegment( dxinfo->remoteMap, NO_FLAGS, &error );
        if ( error == SCI_ERR_OK )
        {
            printf( "The remote segment is unmapped\n" );
        }
        else
        {
            fprintf(
                stderr, "SCIUnmapSegment failed - Error code 0x%x\n", error );
            return error;
        }

        /* Disconnect segment */
        SCIDisconnectSegment( dxinfo->remoteSegment, NO_FLAGS, &error );
        if ( error == SCI_ERR_OK )
        {
            printf( "The segment is disconnected\n" );
        }
        else
        {
            fprintf( stderr,
                     "SCIDisconnectSegment failed - Error code 0x%x\n",
                     error );
            return error;
        }
    }

    return SCI_ERR_OK;
}

 
/*********************************************************************************
 *                                                                               *
 * Copyright (C) 1993 - 2015                                                     * 
 *         Dolphin Interconnect Solutions AS                                     *
 *                                                                               *
 * This program is free software; you can redistribute it and/or modify          * 
 * it under the terms of the GNU General Public License as published by          *
 * the Free Software Foundation; either version 2 of the License,                *
 * or (at your option) any later version.                                        *
 *                                                                               *
 * This program is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 * 
 * GNU General Public License for more details.                                  *
 *                                                                               *
 * You should have received a copy of the GNU General Public License             *
 * along with this program; if not, write to the Free Software                   *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.   *
 *                                                                               *
 *                                                                               *
 *********************************************************************************/ 


/*********************************************************************************/
/*                                                                               */
/* This program demonstrates the use of the SISCI Reflective Memory              */
/* functionality available with the Dolphin Express DX technology.               */
/*                                                                               */
/* This functionality is not available for the SCI technology.                   */
/*                                                                               */
/*********************************************************************************/


#ifdef _WIN32
#ifndef OS_IS_WINDOWS
#define OS_IS_WINDOWS       1
#endif /* !OS_IS_WINDOWS */
#endif /* _WIN32 */


#ifdef OS_IS_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sisci_types.h"
#include "sisci_api.h"
#include "sisci_error.h"
#include "sisci_demolib.h"
#include "testlib.h"
#include <malloc.h>

#include "../drv/crc.c"
#include "../include/daqmap.h"

#define CDS_DAQ_NET_IPC_OFFSET 0x0
#define CDS_DAQ_NET_GDS_TP_TABLE_OFFSET 0x1000
#define CDS_DAQ_NET_DATA_OFFSET 0x2000


#define __CDECL

#define NO_CALLBACK         NULL
#define NO_FLAGS            0
#define DATA_TRANSFER_READY 8
#define CMD_READY           1234

/* Use upper 4 KB of segment for synchronization. */
#define SYNC_OFFSET ((segmentSize) / 4 - 1024)

#define FILTER     0x12345
#define MATCH_VAL  0xabcdef
#define DFLT_EID   1
#define DFLT_LEN   8192
#define DFLT_END   128
#define MAX_LEN    (1024*1024*1024) 
#define DFLT_ITER  1000
#define NUM_RREQ   16  /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/
#define NUM_SREQ   256  /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/

#define DO_HANDSHAKE 0
#define MATCH_VAL_MAIN (1 << 31)
#define MATCH_VAL_THREAD 1

#define MY_DCU_OFFSET	0x1a00000
#define MY_IPC_OFFSET	(MY_DCU_OFFSET + 0x8000)
#define MY_GDS_OFFSET	(MY_DCU_OFFSET + 0x9000)
#define MY_DAT_OFFSET	(MY_DCU_OFFSET + 0xa000)

extern void *findSharedMemory(char *);

static struct rmIpcStr *shmIpcPtr[128];
static char *shmDataPtr[128];
static struct cdsDaqNetGdsTpNum *shmTpTable[128];
static const int header_size = sizeof(struct rmIpcStr) + sizeof(struct cdsDaqNetGdsTpNum);
volatile char *dxIpcPtr;
volatile char *dxDataPtr;
volatile char *dxGdsPtr;
volatile char *dxDataBlk;
volatile char *drIpcPtr;
volatile char *drDataPtr;
volatile char *drGdsPtr;
volatile char *drDataBlk;
int *drIntData;
static const int buf_size = DAQ_DCU_BLOCK_SIZE;
/*
 * Remote nodeId:
 *
 * DIS_BROADCAST_NODEID_GROUP_ALL is a general broadcast remote nodeId and 
 * must be used as the remote nodeId in SCIConnectSegment() function
 */


sci_error_t             error;
sci_desc_t              sd;
sci_local_segment_t     localSegment;
sci_remote_segment_t    remoteSegment;
sci_map_t               localMap;
sci_map_t               remoteMap;
unsigned int            localAdapterNo = 0;
unsigned int            remoteNodeId   = 0;
unsigned int            localNodeId    = 0;
unsigned int            segmentId;
unsigned int            segmentSize    = 8192;
unsigned int            offset         = 0;
unsigned int            client         = 0;
unsigned int            server         = 0;
unsigned int            *localbufferPtr;
unsigned int            loops          = 5000;
int                     rank           = 0;
int                     nodes          = 0;
unsigned int 		memcpyFlag     = NO_FLAGS;

/*********************************************************************************/
/*                                U S A G E                                      */
/*                                                                               */
/*********************************************************************************/

void Usage()
{
    printf("Usage of reflective:\n");
    printf("reflective  -client -nodes <nodes>[ -a <adapter no> -size <segment size> ] \n");
    printf("reflective  -server -rank <rank> [ -a <adapter no> -size <segment size> ] \n\n");
    printf(" -client            : %s\n", (client) ? "The local node is client" : "The local node is server");
    printf(" -a <value>         : Local adapter number (default %d)\n", localAdapterNo);
    printf(" -size <value>      : Segment size   (default %d)\n", segmentSize);
    printf(" -group <value>     : Reflective group identifier (0..5))\n");
    printf(" -rank <value>      : Rank of server nodes (1,2,3,4,5,6,7, 8,9)\n");
    printf(" -nodes <value>     : Number of servers.\n");
    printf(" -loops <value>     : Loops to execute  (default %d)\n", loops);
    printf(" -help              : This helpscreen\n");
    printf("\n");
}


/*********************************************************************************/
/*                   P R I N T   P A R A M E T E R S                             */
/*                                                                               */
/*********************************************************************************/
void PrintParameters(void)
{
    printf("Test parameters for %s \n",(client) ?  "client" : "server" );
    printf("----------------------------\n\n");
    printf("Local adapter no.     : %d\n",localAdapterNo);
    printf("Local nodeId.         : %d\n",localNodeId);
    printf("Segment size          : %d\n",segmentSize);
    printf("My Rank               : %d\n",rank);
    printf("RM SegmentId          : %d\n",segmentId);
    printf("Number of nodes in RM : %d\n",nodes);
    printf("----------------------------\n\n");
}

int waitServers(int nodes,sci_sequence_t sequence,volatile unsigned int *readAddr,volatile unsigned int *writeAddr)
{
int node_offset;
int value;

        /* Lets wait for the servers to write CMD_READY  */
        printf("Wait for %d servers ...\n", nodes);
        
        for (node_offset=1; node_offset <= nodes;node_offset++){
            int wait_loops = 0;
            
            do {
                value = (*(readAddr+SYNC_OFFSET+node_offset));
                wait_loops++;
            } while (value != CMD_READY); 
        }
        printf("Client received CMD_READY from all nodes \n\n",value); 
            
        /* Lets write CMD_READY to offset 0 to signal all servers to go on. */
        *(writeAddr+SYNC_OFFSET) = CMD_READY;  
        SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
}

int waitClients(int rank,sci_sequence_t sequence,volatile unsigned int *readAddr,volatile unsigned int *writeAddr)
{

        int wait_loops = 0;
	int value;
        
        /* Lets write CMD_READY the to client, offset "myrank" */
        *(writeAddr+SYNC_OFFSET+rank) = CMD_READY;
        SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
        
        printf("Wait for CMD_READY from master ...\n");

        /* Lets wait for the client to send me CMD_READY in offset 0 */
        do {
            
            value = (*(readAddr+SYNC_OFFSET));  
            wait_loops++;

            if ((wait_loops % 100000000)==0) {
                /* Lets again write CMD_READY to the client, offset "myrank" */
                *(writeAddr+SYNC_OFFSET+rank) = CMD_READY;
                SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
            }

        } while (value != CMD_READY);
        printf("Server received CMD_READY\n");
}


sci_error_t create_and_test_reflective_memory(int client)
{
    volatile unsigned int *readAddr;  /* Read from reflective memory */
    volatile unsigned int *writeAddr; /* Write to reflective memory  */
    unsigned int          value;
    unsigned int          written_value = 0;
    sci_sequence_t        sequence   = NULL;
    double                average;
    timer_start_t         timer_start;
    int                   verbose = 0;
    int                   node_offset;
    volatile unsigned char *myreadAddr;
    volatile unsigned char *mywriteAddr;

#if 0
    volatile void *base;
    DAQ_INFO_BLOCK *dinfo;
    struct rmIpcStr *dipc;
    char *dataPtr;
    int *dataIntType;

	base = (volatile void *)findSharedMemory(mydaq);
        if(base == NULL) {
                printf("Memory ptr not found for %s\n",mydaq);
        } else {
                printf("My mem points are:\n");
        }
#endif
    /* 
     * The segmentId paramter is used to set the reflective memory group id 
     * when the flag SCI_FLAG_BROADCAST is specified. 
     *
     * For Dolphin Express DX, the reflective memory group id is limited to 0-5
     * For Dolphin Express IX, the reflective memory group id is limited to 0-3
     *
     * All nodes within the broadcast group must have the same segmentId to communicate.
     */

    /* Create local reflective memory segment */    
    SCICreateSegment(sd,&localSegment,segmentId, segmentSize, NO_CALLBACK, NULL, SCI_FLAG_BROADCAST, &error);

    if (error == SCI_ERR_OK) {
        printf("Local segment (id=0x%x, size=%d) is created. \n", segmentId, segmentSize);  
    } else {
        fprintf(stderr,"SCICreateSegment failed - Error code 0x%x\n",error);
        return error;
    }

    /* Prepare the segment */
    SCIPrepareSegment(localSegment,localAdapterNo, SCI_FLAG_BROADCAST, &error); 
    
    if (error == SCI_ERR_OK) {
        printf("Local segment (id=0x%x, size=%d) is prepared. \n", segmentId, segmentSize);  
    } else {
        fprintf(stderr,"SCIPrepareSegment failed - Error code 0x%x\n",error);
        return error;
    }

    /* Map local segment to user space - this is the address to read back data from the reflective memory region */
    readAddr = SCIMapLocalSegment(localSegment,&localMap, offset,segmentSize, NULL,NO_FLAGS,&error);
    myreadAddr = (unsigned char *)readAddr;
    if (error == SCI_ERR_OK) {
        printf("Local segment (id=0x%x) is mapped to user space at 0x%lx\n", segmentId,(unsigned long)myreadAddr); 
    } else {
        fprintf(stderr,"SCIMapLocalSegment failed - Error code 0x%x\n",error);
        return error;
    } 

    /* Set the segment available */
    SCISetSegmentAvailable(localSegment, localAdapterNo, NO_FLAGS, &error);
    if (error == SCI_ERR_OK) {
        printf("Local segment (id=0x%x) is available for remote connections. \n", segmentId); 
    } else {
        fprintf(stderr,"SCISetSegmentAvailable failed - Error code 0x%x\n",error);
        return error;
    } 

    /* Connect to remote segment */
    printf("Connect to remote segment .... ");

    do { 
        SCIConnectSegment(sd,
                          &remoteSegment,
                          remoteNodeId,
                          segmentId,
                          localAdapterNo,
                          NO_CALLBACK,
                          NULL,
                          SCI_INFINITE_TIMEOUT,
                          SCI_FLAG_BROADCAST,
                          &error);

        sleep(1);

    } while (error != SCI_ERR_OK);

    int remoteSize = SCIGetRemoteSegmentSize(remoteSegment);
    printf("Remote segment (id=0x%x) is connected with size %d.\n", segmentId,remoteSize);

    /* Map remote segment to user space */
    writeAddr = SCIMapRemoteSegment(remoteSegment,&remoteMap,offset,segmentSize,NULL,SCI_FLAG_BROADCAST,&error);
    mywriteAddr = (unsigned char *)writeAddr;
    if (error == SCI_ERR_OK) {
        printf("Remote segment (id=0x%x) is mapped to user space. \n", segmentId);         
    } else {
        fprintf(stderr,"SCIMapRemoteSegment failed - Error code 0x%x\n",error);
        return error;
    } 

    /* Create a sequence for data error checking*/ 
    SCICreateMapSequence(remoteMap,&sequence,NO_FLAGS,&error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCICreateMapSequence failed - Error code 0x%x\n",error);
        return error;
    }

    /* The reflective memory functionality is operational at this point. */

    /* Demonstrate how to use reflective memory */ 
    printf("\n");

    /* Perform a barrier operation. The client acts as master. */
    if (client){
	waitServers(nodes,sequence,readAddr,writeAddr);
    } else {
	waitClients(rank,sequence,readAddr,writeAddr);
    }
    
    printf("\n***********************************************************\n\n");
    
    printf("Starting latency measurements...\n");
    printf("Loops: %d\n", loops);
    
    writeAddr += 256;
    readAddr += 256;
    
    written_value=1;
    int ii;
	int new_cycle = 0;
        int lastCycle = 0;
unsigned char *dataBuff;
	dxIpcPtr = (unsigned char *) (mywriteAddr + MY_IPC_OFFSET);
	dxGdsPtr = (unsigned char *) (mywriteAddr + MY_GDS_OFFSET);
	dxDataPtr = (unsigned char *) (mywriteAddr + MY_DAT_OFFSET);
	drIpcPtr = (unsigned char *) (myreadAddr + MY_IPC_OFFSET);
	drGdsPtr = (unsigned char *) (myreadAddr + MY_GDS_OFFSET);
	drDataPtr = (unsigned char *) (myreadAddr + MY_DAT_OFFSET);
	drIntData = (int *)drDataPtr;
	drIntData += 2;
printf("My drintdata is at 0x%lx \n",(unsigned long)drIpcPtr);

        if (client){
		for (;shmIpcPtr[0]->cycle;) usleep(1000);
		printf("Found cycle zero in client\n");
	}
    do {
        
        if (client){
            
            if (verbose) {
                printf("Send broadcast message (value %d) to %d available nodes ...\n",written_value,nodes);
            }
            
            /* Lets write the value to offset 0 */
// WRITEDATA
                // Wait for cycle count update from FE
                do{
                        usleep(1000);
                        new_cycle = shmIpcPtr[0]->cycle;
                } while (new_cycle == lastCycle);

		lastCycle = new_cycle;

SCIMemCpy(sequence,shmTpTable[0], remoteMap,MY_GDS_OFFSET,sizeof(struct cdsDaqNetGdsTpNum),memcpyFlag,&error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCIMemCpy failed - Error code 0x%x\n",error);
        return error;
    }
dataBuff = (unsigned char *)shmDataPtr[0] + (lastCycle * buf_size);
// dataBuff = (unsigned char *)shmDataPtr[0]; // + (lastCycle * buf_size);
	drIntData = (int *)drDataPtr;
	drIntData += 2;
    // SCIMemCpy(sequence,shmDataPtr[0], remoteMap,MY_DAT_OFFSET,shmIpcPtr[0]->dataBlockSize,memcpyFlag,&error);
SCIMemCpy(sequence,dataBuff, remoteMap,MY_DAT_OFFSET,shmIpcPtr[0]->dataBlockSize,memcpyFlag,&error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCIMemCpy failed - Error code 0x%x\n",error);
        return error;
    }
SCIMemCpy(sequence,shmIpcPtr[0], remoteMap,MY_IPC_OFFSET,sizeof(struct rmIpcStr),memcpyFlag,&error);
    if (error == SCI_ERR_OK) {
        printf("Sending CPU value of %d on cycle %d\n",*drIntData,lastCycle);
    } else {
        fprintf(stderr,"SCIMemCpy failed - Error code 0x%x\n",error);
        return error;
    }
#if 0
	    memcpy((unsigned char *)dxIpcPtr,(unsigned char *)shmIpcPtr[0],sizeof(struct rmIpcStr)); 
	    memcpy((unsigned char *)dxGdsPtr,(unsigned char *)shmTpTable[0],sizeof(struct cdsDaqNetGdsTpNum)); 
	    dataBuff = (unsigned char *)shmDataPtr[0]; // + (lastCycle * buf_size);
	    memcpy((unsigned char *)dxDataPtr,(unsigned char *)dataBuff,shmIpcPtr[0]->dataBlockSize); 
#endif
            *writeAddr = written_value; 
            SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
            
	    printf("Writing cycle %d with size 0x%d \n",new_cycle,shmIpcPtr[0]->dataBlockSize);
            /* Lets wait for the servers to write the written value +1  */
            
            for (node_offset=1; node_offset <= nodes;node_offset++){
                int wait_loops = 0;
                
                do {
                    value = (*(readAddr+node_offset));
                    wait_loops++;
                    if ((wait_loops % 10000000)==0){
                        printf("Value = %d (expected = %d) delayed from rank %d - after %u reads\n", value, written_value+1, node_offset, wait_loops);
                    }
                } while (value != written_value+1); 
            }
            
            if (verbose) {
                printf("Client received broadcast value %d from all nodes \n\n",value); 
            }
            written_value++;
            
        } else {
            int wait_loops = 0;
            
            /* Lets wait for the client to send me a value in offset 0 */
            
            if (verbose) {
                printf("Wait for broadcast message...\n");
            }
            
            do {
                
                value = (*(readAddr));  
                wait_loops++;
                // if ((wait_loops % 10000000)==0) {
                   //  printf("Value = %d delayed from Client - written_value=%d\n", value,written_value);
                // }
            } while (value != written_value); 
            
            if (verbose) {
                printf("Server received broadcast value %d \n",value); 
            }
            
            written_value++;
            /* Lets write a value back received value +1 to the client, offset "myrank" */
            *(writeAddr+rank) = written_value;
            SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
        }
    } while (written_value < loops); /* do this number of loops */
    
    printf("\n***********************************************************\n\n");
    
    /* Lets clean up after demonstrating the use of reflective memory  */
    
    /* Remove the Sequence */
    SCIRemoveSequence(sequence,NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCIRemoveSequence failed - Error code 0x%x\n",error);
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
    
    /* Unmap remote segment */
    SCIUnmapSegment(remoteMap,NO_FLAGS,&error);
    if (error == SCI_ERR_OK) {
        printf("The remote segment is unmapped\n"); 
    } else {
        fprintf(stderr,"SCIUnmapSegment failed - Error code 0x%x\n",error);
        return error;
    }
    
    /* Disconnect segment */
    SCIDisconnectSegment(remoteSegment,NO_FLAGS,&error);
    if (error == SCI_ERR_OK) {
        printf("The segment is disconnected\n"); 
    } else {
        fprintf(stderr,"SCIDisconnectSegment failed - Error code 0x%x\n",error);
        return error;
    } 
    
    return SCI_ERR_OK;
}


/*********************************************************************************/
/*                                M A I N                                        */
/*                                                                               */
/*********************************************************************************/

int __CDECL
main(int argc,char *argv[])
{
    int counter; 

    printf("\n %s compiled %s : %s\n\n",argv[0],__DATE__,__TIME__);
    
    if (argc<3) {
        Usage();
        return(-1);
    }

    /* Get the parameters */
    for (counter=1; counter<argc; counter++) {
      
        if (!strcmp("-rank",argv[counter])) {
            /*LINTED*/
            rank = strtol(argv[counter+1],(char **) NULL,10);
            continue;
        }  

         if (!strcmp("-nodes",argv[counter])) {
            /*LINTED*/
            nodes = strtol(argv[counter+1],(char **) NULL,10);
            continue;
        }  

        if (!strcmp("-group",argv[counter])) {
            /*LINTED*/
            segmentId = strtol(argv[counter+1],(char **) NULL,10);
            continue;
        }

        if (!strcmp("-loops",argv[counter])) {
            loops = strtol(argv[counter+1],(char **) NULL,10);
            continue;
        }

        if (!strcmp("-size",argv[counter])) {
            segmentSize = strtol(argv[counter+1],(char **) NULL,10);
            if (segmentSize < 4096){
                printf("Min segment size is 4 KB\n");
                return -1;
            }
            continue;
        } 

        if (!strcmp("-a",argv[counter])) {
            localAdapterNo = strtol(argv[counter+1],(char **) NULL,10);
            continue;
        }

        if (!strcmp("-rn",argv[counter])) {
	    continue;
        }

        if (!strcmp("-client",argv[counter])) {
            client = 1;
            continue;
        }

        if (!strcmp("-server",argv[counter])) {
            server = 1;
            continue;
        }

        if (!strcmp("-help",argv[counter])) {
            Usage();
            return(0);
        }
    }

    if (client == 0 && rank == 0 ){
        printf("Rank must be set to nonzero value for server\n\n");
        Usage();
        return(0);
    }

    if (client == 1 ){
        printf("Nodes must be set to nonzero value for client\n\n");

    }

    if (server == 0 && client == 0) {
        fprintf(stderr,"You must specify a client node or a server node\n");
        return(-1);
    }

    if (server == 1 && client == 1) {
        fprintf(stderr,"Both server node and client node is selected.\n"); 
        fprintf(stderr,"You must specify either a client or a server node\n");
        return(-1);
    }

	if(client) {
		char sname[128];
		sprintf(sname,"%s_daq","x1ioplsc0");
		volatile void *dcu_addr = findSharedMemory(sname);
		if (dcu_addr <= 0) {
			fprintf(stderr, "Can't map shmem\n");
			exit(1);
		} else {
			printf("mapped at 0x%lx\n",(unsigned long)dcu_addr);
		}
		// shmIpcPtr[0] = (struct rmIpcStr *)((char *)dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
		shmIpcPtr[0] = (struct rmIpcStr *)dcu_addr;
		shmDataPtr[0] = (char *)(dcu_addr + DAQ_NIB_DATA_OFFSET);
		shmTpTable[0] = (struct cdsDaqNetGdsTpNum *)((char *)dcu_addr + DAQ_NIB_GDS_TABLE_OFFSET);

		for (;shmIpcPtr[0]->cycle;) usleep(1000);
        	int lastCycle = 0;
		printf("Found cycle zero \n");
		int new_cycle;
                // Wait for cycle count update from FE
                do{
                        usleep(1000);
                        new_cycle = shmIpcPtr[0]->cycle;
                } while (new_cycle == lastCycle);
                printf("New cycle = %d\n", shmIpcPtr[0]->cycle);
                printf("Size of rmIpcStr = %ld\n", sizeof(struct rmIpcStr));
                printf("Size of GDStp = %ld\n", sizeof(struct cdsDaqNetGdsTpNum));
		drIntData = (int *)shmDataPtr[0];
		drIntData += 2;
		printf("CPU = %d\n",*drIntData);
	}


    /* Initialize the SISCI library */
    SCIInitialize(NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCIInitialize failed - Error code: 0x%x\n",error);
        return(-1);
    }

    /* Open a file descriptor */
    SCIOpen(&sd,NO_FLAGS,&error);
    if (error != SCI_ERR_OK) {
        if (error == SCI_ERR_INCONSISTENT_VERSIONS) {
            fprintf(stderr,"Version mismatch between SISCI user library and SISCI driver\n");
        }
        fprintf(stderr,"SCIOpen failed - Error code 0x%x\n",error);
        return(-1); 
    }

    /* Get local nodeId */
    SCIGetLocalNodeId(localAdapterNo,
                      &localNodeId,
                      NO_FLAGS,
                      &error);

    if (error != SCI_ERR_OK) {
        fprintf(stderr,"Could not find the local adapter %d\n", localAdapterNo);
        SCIClose(sd,NO_FLAGS,&error);
        SCITerminate();
        return(-1);
    }

    /*
     * Set remote nodeId to BROADCAST NODEID
     */
    remoteNodeId = DIS_BROADCAST_NODEID_GROUP_ALL;

    /* Print parameters */
    PrintParameters();

    error = create_and_test_reflective_memory(client);

    if (error!= SCI_ERR_OK) {
        fprintf(stderr,"SCIClose failed - Error code: 0x%x\n",error);
        SCITerminate();
        return(-1);
    }

    /* Close the file descriptor */
    SCIClose(sd,NO_FLAGS,&error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCIClose failed - Error code: 0x%x\n",error);
        SCITerminate();
        return(-1);
    }

    /* Free allocated resources */
    SCITerminate();

    return SCI_ERR_OK;
}

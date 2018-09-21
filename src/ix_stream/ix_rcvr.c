 
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
#include "../include/drv/fb.h"
#include "../include/daq_core.h"



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

// #define MY_DCU_OFFSET	0x1a00000
#define MY_DCU_OFFSET	0x00000
#define MY_IPC_OFFSET	(MY_DCU_OFFSET + 0x8000)
#define MY_GDS_OFFSET	(MY_DCU_OFFSET + 0x9000)
#define MY_DAT_OFFSET	(MY_DCU_OFFSET + 0xa000)

#include "./dolphin_common.c"
extern void *findSharedMemory(char *);

static struct rmIpcStr *shmIpcPtr[128];
static char *shmDataPtr[128];
static struct cdsDaqNetGdsTpNum *shmTpTable[128];
static const int header_size = sizeof(struct daq_fe_header_t);
static const int buf_size = DAQ_DCU_BLOCK_SIZE;
char modelnames[DAQ_TRANSIT_MAX_DCU][64];
char *sysname;
int modelrates[DAQ_TRANSIT_MAX_DCU];
daq_multi_dcu_data_t ixDataBlock;
char *daqbuffer = (char *) &ixDataBlock;
daq_fe_data_t *zbuffer;
unsigned int            loops          = 170;
static const int ipcSize = sizeof(struct daq_msg_header_t);


int signOff(int rank,sci_sequence_t sequence,volatile unsigned int *readAddr,volatile unsigned int *writeAddr)
{
        /* Lets write CMD_READY the to client, offset "myrank" */
        *(writeAddr+SYNC_OFFSET+rank) = 0;
        SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
}

int waitSender(int rank,sci_sequence_t sequence,volatile unsigned int *readAddr,volatile unsigned int *writeAddr)
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
        *(writeAddr+SYNC_OFFSET+rank) = 0;
        SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
}


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



sci_error_t ix_rcv_reflective_memory()
{
    
    unsigned int          value;
    unsigned int          written_value = 0;
    double                average;
    timer_start_t         timer_start;
    int                   verbose = 1;
    int                   node_offset;



    printf("Read = 0x%lx \n Write = 0x%lx \n",(long)readAddr,(long)writeAddr);

    /* Perform a barrier operation. The client acts as master. */
	waitSender(rank,sequence,readAddr,writeAddr);
    
    printf("\n***********************************************************\n\n");
    
    printf("Loops: %d\n", loops);
    
    writeAddr += 256;
    readAddr += 256;
    
    written_value=1;
    int ii;
	int new_cycle = 0;
        int lastCycle = 0;
	int msgSize = 0;

    do {
        
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

	    printf("zbuff count = %d\n",zbuffer->header.dcuTotalModels);
	    printf("zbuff size  = %d\n",zbuffer->header.dataBlockSize);
	    printf("zbuff dcuid  = %d\n",zbuffer->header.dcuheader[0].dcuId);
	    printf("zbuff cycle  = %d\n",zbuffer->header.dcuheader[0].cycle);
	    msgSize = header_size + zbuffer->header.dataBlockSize;
	    printf("buff size  = %d\t%d\t%d\n",msgSize,header_size,ipcSize);

            
            if (verbose) {
                printf("Received broadcast value %d \n",value); 
            }
            
            written_value++;
            /* Lets write a value back received value +1 to the client, offset "myrank" */
            *(writeAddr+rank) = written_value;
            SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
    } while (written_value < loops); /* do this number of loops */
    
    printf("\n***********************************************************\n\n");
    
    /* Lets clean up after demonstrating the use of reflective memory  */
    
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
    volatile unsigned char *daq_read_addr;

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
        if (!strcmp("-models",argv[counter])) {
	    sysname = argv[counter+1];
	    printf ("sysnames = %s\n",sysname);
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

    error = dolphin_init();
    printf("Read = 0x%lx \n Write = 0x%lx \n",(long)readAddr,(long)writeAddr);

    daq_read_addr = (unsigned char *)readAddr + MY_DAT_OFFSET;
    zbuffer = (daq_fe_data_t *)daq_read_addr;

    printf("Calling recvr \n");

    error = ix_rcv_reflective_memory();

    signOff(rank,sequence,readAddr,writeAddr);
    
    error = dolphin_closeout();

    return SCI_ERR_OK;
}

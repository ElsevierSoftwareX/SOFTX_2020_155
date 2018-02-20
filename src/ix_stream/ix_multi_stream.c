 
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
#define DAQ_RDY_MAX_WAIT        80

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
static const int header_size = sizeof(struct rmIpcStr) + sizeof(struct cdsDaqNetGdsTpNum);
int *drIntData;
static const int buf_size = DAQ_DCU_BLOCK_SIZE;
char modelnames[DAQ_TRANSIT_MAX_DCU][64];
char *sysname;
int modelrates[DAQ_TRANSIT_MAX_DCU];
daq_multi_dcu_data_t ixDataBlock;
char *daqbuffer = (char *) &ixDataBlock;
char buffer[1024000];
char *zbuffer;
int do_verbose = 1;
int sendLength = 0;

unsigned int            loops          = 170;

/*********************************************************************************/
/*                                U S A G E                                      */
/*                                                                               */
/*********************************************************************************/

void Usage()
{
    printf("Usage of ix_multi_stream:\n");
    printf("ix_multi_stream  -n <nodes> -g <group> -m <models> \n");
    printf(" -a <value>     : Local adapter number (default %d)\n", localAdapterNo);
    printf(" -s <value>     : Segment size   (default %d)\n", segmentSize);
    printf(" -g <value>     : Reflective group identifier (0..5))\n");
    printf(" -n <value>     : Number of receivers\n");
    printf(" -l <value>     : Loops to execute  (default %d)\n", loops);
    printf(" -h             : This helpscreen\n");
    printf("\n");
}

int sync2zero(struct rmIpcStr *ipcPtr) {
        int lastCycle = 0;

        // Find cycle zero
         for (;ipcPtr->cycle;) usleep(1000);
         return(lastCycle);
}

int getmodelrate( char *modelname, char *gds_tp_dir) {
    int rate = 0;
    char gdsfile[128];
    int ii = 0;
    FILE *f = 0;
    char *token = 0;
    char *search = "=";
    char line[80];
	char *s = 0;
	char *s1 = 0;

	if (gds_tp_dir) {
		sprintf(gdsfile, "%s/tpchn_%s.par", gds_tp_dir, modelname);
	} else {
		/// Need to get IFO and SITE info from environment variables.
		s = getenv("IFO");
		for (ii = 0; s[ii] != '\0'; ii++) {
			if (isupper(s[ii])) s[ii] = (char) tolower(s[ii]);
		}
		s1 = getenv("SITE");
		for (ii = 0; s1[ii] != '\0'; ii++) {
			if (isupper(s1[ii])) s1[ii] = (char) tolower(s1[ii]);
		}
		sprintf(gdsfile, "/opt/rtcds/%s/%s/target/gds/param/tpchn_%s.par", s1, s, modelname);
	}
    f = fopen(gdsfile, "rt");
    if (!f) return 0;
    while(fgets(line,80,f) != NULL) {
        line[strcspn(line, "\n")] = 0;
        if (strstr(line, "datarate") != NULL) {
            token = strtok(line, search);
            token = strtok(NULL, search);
            if (!token) continue;
            while (*token && *token == ' ') {
                ++token;
            }
            rate = atoi(token);
            break;
        }
    }
    fclose(f);

    return rate;
}

int waitServers(int nodes,sci_sequence_t sequence,volatile unsigned int *readAddr,volatile unsigned int *writeAddr)
{
int node_offset;
int value;

        /* Lets wait for the servers to write CMD_READY  */
        printf("Wait for %d receivers ...\n", nodes);
        
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

sci_error_t send_via_reflective_memory(int nsys)
{
    unsigned int          value;
    unsigned int          written_value = 0;
    sci_sequence_t        sequence   = NULL;
    int                   verbose = 1;
    int                   node_offset;
    volatile unsigned char *myreadAddr;
    volatile unsigned char *mywriteAddr;


    int timeout;
    int myCrc;
    int do_wait = 1;
    int daqStatBit[2];
    daqStatBit[0] = 1;
    daqStatBit[1] = 2;
    int reftimeerror = 0;
    int refcycle;
    int reftimeSec;
    int reftimeNSec;
    int dataTPLength;

    myreadAddr = (unsigned char *)readAddr;
    mywriteAddr = (unsigned char *)writeAddr;


    
	waitServers(nodes,sequence,readAddr,writeAddr);
    writeAddr += 256;
    readAddr += 256;
    
    written_value=1;
    int ii;
	int new_cycle = 0;
        int lastCycle = 0;
unsigned char *dataBuff;

  int sync2iop = 1;


//	for (;shmIpcPtr[0]->cycle;) usleep(1000);
//	printf("Found cycle zero in client\n");
    do {
        
		if(sync2iop) {
			printf("Syncing to IOP\n");
			lastCycle = sync2zero(shmIpcPtr[0]);
			sync2iop = 0;
			printf("Found cycle zero\n");
		}
		timeout = 0;
		// Wait for a new 1/16Hz DAQ data cycle
		do{
			usleep(1000);
			new_cycle = shmIpcPtr[0]->cycle;
			timeout += 1;
		}while (new_cycle == lastCycle && timeout < DAQ_RDY_MAX_WAIT);
		if(timeout >= DAQ_RDY_MAX_WAIT) {
			sync2iop = 1;
			lastCycle = sync2zero(shmIpcPtr[0]);
			printf("Iop model not running\n");
		}
		if(sync2iop) continue;
		// IOP will be first model ready
		// Need to wait for 2K models to reach end of their cycled
		usleep((do_wait * 1000));

		// Print diags in verbose mode
		if(new_cycle == 0 && do_verbose) {
			printf("\nTime = %d-%d size = %d\n",shmIpcPtr[0]->bp[lastCycle].timeSec,shmIpcPtr[0]->bp[lastCycle].timeNSec,sendLength);
			printf("\tCycle = ");
			for(ii=0;ii<nsys;ii++) printf("\t\t%d",ixDataBlock.header.dcuheader[ii].cycle);
			printf("\n\tTimeSec = ");
			for(ii=0;ii<nsys;ii++) printf("\t%d",ixDataBlock.header.dcuheader[ii].timeSec);
			printf("\n\tTimeNSec = ");
			for(ii=0;ii<nsys;ii++) printf("\t\t%d",ixDataBlock.header.dcuheader[ii].timeNSec);
			printf("\n\tDataSize = ");
			for(ii=0;ii<nsys;ii++) printf("\t\t%d",ixDataBlock.header.dcuheader[ii].dataBlockSize);
		    	printf("\n\tTPCount = ");
		    	for(ii=0;ii<nsys;ii++) printf("\t\t%d",ixDataBlock.header.dcuheader[ii].tpCount);
		    	printf("\n\tTPSize = ");
		    	for(ii=0;ii<nsys;ii++) printf("\t\t%d",ixDataBlock.header.dcuheader[ii].tpBlockSize);
		    	printf("\n\n ");
		}

		// Increment the local DAQ cycle counter
		lastCycle ++;
		lastCycle %= 16;

		
		// Set pointer to 0MQ message data block
		zbuffer = (char *)&ixDataBlock.dataBlock[0];
		// Initialize data send length to size of message header
		sendLength = header_size;
		// Set number of FE models that have data in this message
		ixDataBlock.header.dcuTotalModels = nsys;
		ixDataBlock.header.dataBlockSize = 0;
		// Loop thru all FE models
		for (ii=0;ii<nsys;ii++) {
			reftimeerror = 0;
			// Set heartbeat monitor for return to DAQ software
			if (lastCycle == 0) shmIpcPtr[ii]->reqAck ^= daqStatBit[0];
			// Set DCU ID in header
			ixDataBlock.header.dcuheader[ii].dcuId = shmIpcPtr[ii]->dcuId;
			// Set DAQ .ini file CRC checksum
			ixDataBlock.header.dcuheader[ii].fileCrc = shmIpcPtr[ii]->crc;
			// Set 1/16Hz cycle number
			ixDataBlock.header.dcuheader[ii].cycle = shmIpcPtr[ii]->cycle;
			if(ii == 0) refcycle = shmIpcPtr[ii]->cycle;
			// Set GPS seconds
			ixDataBlock.header.dcuheader[ii].timeSec = shmIpcPtr[ii]->bp[lastCycle].timeSec;
			if (ii == 0) reftimeSec = shmIpcPtr[ii]->bp[lastCycle].timeSec;
			// Set GPS nanoseconds
			ixDataBlock.header.dcuheader[ii].timeNSec = shmIpcPtr[ii]->bp[lastCycle].timeNSec;
			if (ii == 0) reftimeNSec = shmIpcPtr[ii]->bp[lastCycle].timeNSec;
			if (ii != 0 && reftimeSec != shmIpcPtr[ii]->bp[lastCycle].timeSec) 
				reftimeerror = 1;;
			if (ii != 0 && reftimeNSec != shmIpcPtr[ii]->bp[lastCycle].timeNSec) 
				reftimeerror |= 2;;
			if(reftimeerror) {
				ixDataBlock.header.dcuheader[ii].cycle = refcycle;
				// printf("Timing error model %d\n",ii);
				// Set Status -- Need to update for models not running
				ixDataBlock.header.dcuheader[ii].status = 0xbad;
				// Indicate size of data block
				ixDataBlock.header.dcuheader[ii].dataBlockSize = 0;
				ixDataBlock.header.dcuheader[ii].tpBlockSize = 0;
				ixDataBlock.header.dcuheader[ii].tpCount = 0;
			} else {
				// Set Status -- Need to update for models not running
				ixDataBlock.header.dcuheader[ii].status = 2;
				// Indicate size of data block
				ixDataBlock.header.dcuheader[ii].dataBlockSize = shmIpcPtr[ii]->dataBlockSize;
				// Prevent going beyond MAX allowed data size
				if (ixDataBlock.header.dcuheader[ii].dataBlockSize > DAQ_DCU_BLOCK_SIZE)
					ixDataBlock.header.dcuheader[ii].dataBlockSize = DAQ_DCU_BLOCK_SIZE;

                		ixDataBlock.header.dcuheader[ii].tpCount = (unsigned int)shmTpTable[ii]->count;
				ixDataBlock.header.dcuheader[ii].tpBlockSize = sizeof(float) * modelrates[ii] * ixDataBlock.header.dcuheader[ii].tpCount;
				// Prevent going beyond MAX allowed data size
				if (ixDataBlock.header.dcuheader[ii].tpBlockSize > DAQ_DCU_BLOCK_SIZE)
					ixDataBlock.header.dcuheader[ii].tpBlockSize = DAQ_DCU_BLOCK_SIZE;

				memcpy(&(ixDataBlock.header.dcuheader[ii].tpNum[0]),
				       &(shmTpTable[ii]->tpNum[0]),
					   sizeof(int)*ixDataBlock.header.dcuheader[ii].tpCount);

			// Set pointer to dcu data in shared memory
			dataBuff = (char *)(shmDataPtr[ii] + lastCycle * buf_size);
			// Copy data from shared memory into local buffer
			dataTPLength = ixDataBlock.header.dcuheader[ii].dataBlockSize + ixDataBlock.header.dcuheader[ii].tpBlockSize;
			memcpy((void *)zbuffer, dataBuff, dataTPLength);


			// Calculate CRC on the data and add to header info
			myCrc = crc_ptr((char *)zbuffer, ixDataBlock.header.dcuheader[ii].dataBlockSize, 0); // .crc is crcLength
			myCrc = crc_len(ixDataBlock.header.dcuheader[ii].dataBlockSize, myCrc);
			ixDataBlock.header.dcuheader[ii].dataCrc = myCrc;

			// Increment the 0mq data buffer pointer for next FE
			zbuffer += dataTPLength;
			// Increment the 0mq message size with size of FE data block
			sendLength += dataTPLength;
			// Increment the data block size for the message
			ixDataBlock.header.dataBlockSize += (unsigned int)dataTPLength;

			// Update heartbeat monitor to DAQ code
			if (lastCycle == 0) shmIpcPtr[ii]->reqAck ^= daqStatBit[1];
			}
		}

		// Copy data to combined message buffer
		memcpy(buffer,daqbuffer,sendLength);
		// Send Data
            
            if (verbose) {
                printf("Send broadcast message (value %d) to %d available nodes ...\n",written_value,nodes);
            }
            
            /* Lets write the value to offset 0 */
// WRITEDATA
SCIMemCpy(sequence,buffer, remoteMap,MY_DAT_OFFSET,sendLength,memcpyFlag,&error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCIMemCpy failed - Error code 0x%x\n",error);
        return error;
    }

            *writeAddr = written_value; 
            SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
            
	    printf("Writing cycle %d with size %d \n",lastCycle,ixDataBlock.header.dataBlockSize);
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
                printf("Received broadcast ack %d from all nodes \n\n",value); 
            }
            written_value++;
            
    } while (written_value < loops); /* do this number of loops */
    
    printf("\n***********************************************************\n\n");
    
    
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
    int nsys = 1;
    int dcuId[10];
    int ii;
    char *gds_tp_dir = 0;

    printf("\n %s compiled %s : %s\n\n",argv[0],__DATE__,__TIME__);
    
    if (argc<3) {
        Usage();
        return(-1);
    }

    /* Get the parameters */
     while ((counter = getopt(argc, argv, "r:n:g:l:s:m:h:a:")) != EOF) 
      switch(counter) {
      
        case 'r':
            rank = atoi(optarg);
            break;

        case 'n':
            nodes = atoi(optarg);
            break;

        case 'g':
            segmentId = atoi(optarg);
            break;

        case 'l':
            loops = atoi(optarg);
            break;

        case 's':
            segmentSize = atoi(optarg);
            if (segmentSize < 4096){
                printf("Min segment size is 4 KB\n");
                return -1;
            }
            break;

        case 'm':
	    sysname = optarg;
	    printf ("sysnames = %s\n",sysname);
            continue;

        case 'a':
            localAdapterNo = atoi(optarg);
            continue;

        case 'h':
            Usage();
            return(0);
    }
    if(sysname != NULL) {
	printf("System names: %s\n", sysname);
        sprintf(modelnames[0],"%s",strtok(sysname, " "));
        for(;;) {
        	char *s = strtok(0, " ");
	        if (!s) break;
	        sprintf(modelnames[nsys],"%s",s);
	        dcuId[nsys] = 0;
	        nsys++;
	}
    } else {
	Usage();
        return(0);
    }

    for(ii=0;ii<nsys;ii++) {
    	 char shmem_fname[128];
	 sprintf(shmem_fname, "%s_daq", modelnames[ii]);
	 void *dcu_addr = findSharedMemory(shmem_fname);
	 if (dcu_addr <= 0) {
	 	fprintf(stderr, "Can't map shmem\n");
		exit(-1);
	} else {
		printf(" %s mapped at 0x%lx\n",modelnames[ii],(unsigned long)dcu_addr);
	}
	 shmIpcPtr[ii] = (struct rmIpcStr *)((char *)dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
	 shmDataPtr[ii] = ((char *)dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
	 shmTpTable[ii] = (struct cdsDaqNetGdsTpNum *)((char *)dcu_addr + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
    }
    for (ii = 0; ii < nsys; ii++) {
            modelrates[ii] = getmodelrate(modelnames[ii], gds_tp_dir);
	    if (modelrates[ii] == 0) {
	            fprintf(stderr, "Unable to determine the rate of %s\n", modelnames[ii]);
	            exit(1);
            }
    }

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


    error = dolphin_init();
    printf("Read = 0x%lx \n Write = 0x%lx \n",(long)readAddr,(long)writeAddr);



    error = send_via_reflective_memory(nsys);

    error = dolphin_closeout();

    return SCI_ERR_OK;
}

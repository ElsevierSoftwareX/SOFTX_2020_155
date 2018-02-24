
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
#include "../include/daqmap.h"
#include "../include/drv/fb.h"
#include "../include/daq_core.h"


#define __CDECL

#if 0
#define NO_CALLBACK         NULL
#define NO_FLAGS            0
#define DATA_TRANSFER_READY 8
#define CMD_READY           1234
#define DAQ_RDY_MAX_WAIT        80

/* Use upper 4 KB of segment for synchronization. */
#define SYNC_OFFSET ((segmentSize) / 4 - 1024)
#endif

// #define MY_DCU_OFFSET	0x1a00000
#define MY_DCU_OFFSET	0x00000
#define MY_IPC_OFFSET	(MY_DCU_OFFSET + 0x8000)
#define MY_GDS_OFFSET	(MY_DCU_OFFSET + 0x9000)
#define MY_DAT_OFFSET	(MY_DCU_OFFSET + 0xa000)

#include "./dolphin_common.c"
extern void *findSharedMemorySize(char *,int);

char *sysname;
static volatile int keepRunning = 1;


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
    printf(" -h             : This helpscreen\n");
    printf("\n");
}

void intHandler(int dummy) {
        keepRunning = 0;
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
    char *myreadaddr;
    char *rcvDataPtr;
    daq_multi_dcu_data_t *ixDataBlock;
    int sendLength = 0;
    daq_multi_cycle_header_t *rcvHeader;

    printf("\n %s compiled %s : %s\n\n",argv[0],__DATE__,__TIME__);
    
    if (argc<2) {
    	printf("Exiting here \n");
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
    	printf("Exiting here 2 \n");
            Usage();
            return(0);
    }

    // Attach to local shared memory
    char *ifo = (char *)findSharedMemorySize("ifo",100);
    daq_multi_cycle_header_t *ifo_header = (daq_multi_cycle_header_t *)ifo;
    char *ifo_data = (char *)ifo + sizeof(daq_multi_cycle_header_t);
    int lastCycle = 15;
    int new_cycle = 0;
    char *nextData;

    // Connect to Dolphin
    error = dolphin_init();
    printf("Read = 0x%lx \n Write = 0x%lx \n",(long)readAddr,(long)writeAddr);
    myreadaddr = (char *)readAddr;
    myreadaddr += MY_DAT_OFFSET;
    rcvHeader = (daq_multi_cycle_header_t *)myreadaddr;
    rcvDataPtr = (char *)myreadaddr + sizeof(daq_multi_cycle_header_t);

    signal(SIGINT,intHandler);

    // Sync to cycle zero
    for (;rcvHeader->curCycle;) usleep(1000);
    printf("Found cycle zero = %d\n",rcvHeader->curCycle);

    do{
	    // Wait for cycle count update from FE
	    do{
		usleep(2000);
		new_cycle = rcvHeader->curCycle;
	    } while (new_cycle == lastCycle && keepRunning);
	    lastCycle = new_cycle;
	    // Set up pointers to copy data to receive shmem
    	    nextData = (char *)ifo_data;
	    nextData += DAQ_TRANSIT_DC_DATA_BLOCK_SIZE * new_cycle;
	    ixDataBlock = (daq_multi_dcu_data_t *)nextData;
	    sendLength = rcvHeader->cycleDataSize;
	    // Copy data from Dolphin to local memory
	    memcpy(nextData,rcvDataPtr,sendLength);

	    if(new_cycle == 0)
	    {
		    printf("New cycle = %d\n", new_cycle);
		    printf("\t\tNum DCU = %d\n", ixDataBlock->header.dcuTotalModels);
		    printf("\t\tNew Size = %d\n", ixDataBlock->header.dataBlockSize);
		    printf("\t\tTime = %d\n", ixDataBlock->header.dcuheader[0].timeSec);
		    printf("\t\tSend Size = %d\n", sendLength);
	    }
	
	    ifo_header->curCycle = rcvHeader->curCycle;
	    ifo_header->maxCycle = rcvHeader->maxCycle;
	    ifo_header->cycleDataSize = sendLength;
    } while(keepRunning);



    error = dolphin_closeout();
    return SCI_ERR_OK;
}

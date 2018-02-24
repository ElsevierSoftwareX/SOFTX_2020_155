
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
    printf(" -g <value>     : Reflective group identifier (0..5))\n");
    printf(" -v <value>     : Diagnostics level (0..1) \n");
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
    int ii;
    char *myreadaddr;
    char *rcvDataPtr;
    daq_multi_dcu_data_t *ixDataBlock;
    int sendLength = 0;
    daq_multi_cycle_header_t *rcvHeader;
    int myCrc;
    int do_verbose = 0;

    printf("\n %s compiled %s : %s\n\n",argv[0],__DATE__,__TIME__);
    
    if (argc<2) {
    	printf("Exiting here \n");
        Usage();
        return(-1);
    }

    /* Get the parameters */
     while ((counter = getopt(argc, argv, "g:a:v:")) != EOF) 
      switch(counter) {

        case 'g':
            segmentId = atoi(optarg);
            break;

        case 'a':
            localAdapterNo = atoi(optarg);
            break;

        case 'v':
            do_verbose = atoi(optarg);
            break;

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

    // Set pointers to receive header and data in Dolphin network memory area
    myreadaddr = (char *)readAddr;
    myreadaddr += MY_DAT_OFFSET;
    rcvHeader = (daq_multi_cycle_header_t *)myreadaddr;
    rcvDataPtr = (char *)myreadaddr + sizeof(daq_multi_cycle_header_t);

    // Catch control C exit
    signal(SIGINT,intHandler);

    // Sync to cycle zero
    for (;rcvHeader->curCycle;) usleep(1000);
    printf("Found cycle zero = %d\n",rcvHeader->curCycle);

    // Go into infinite receive loop
    do{
	    // Wait for cycle couont update from FE
	    // Check every 2 milliseconds
	    do{
		usleep(2000);
		new_cycle = rcvHeader->curCycle;
	    } while (new_cycle == lastCycle && keepRunning);
	    // Save cycle number of last received message
	    lastCycle = new_cycle;
	    // Set up pointers to copy data to receive shmem
    	    nextData = (char *)ifo_data;
	    nextData += DAQ_TRANSIT_DC_DATA_BLOCK_SIZE * new_cycle;
	    ixDataBlock = (daq_multi_dcu_data_t *)nextData;
	    sendLength = rcvHeader->cycleDataSize;
	    // Copy data from Dolphin to local memory
	    memcpy(nextData,rcvDataPtr,sendLength);

	    // Calculate CRC checksum of received data
	    myCrc = crc_ptr((char *)nextData, sendLength, 0);
	    myCrc = crc_len(sendLength, myCrc);
	
	    // Write data header info to shared memory
	    ifo_header->maxCycle = rcvHeader->maxCycle;
	    ifo_header->cycleDataSize = sendLength;
	    ifo_header->msgcrc = rcvHeader->msgcrc;
	    ifo_header->curCycle = rcvHeader->curCycle;

	    // Verify send CRC matches received CRC
	    if(ifo_header->msgcrc != myCrc)
		    printf("Sent = %d and RCV = %d\n",ifo_header->msgcrc,myCrc);

	    // Print some diagnostics
	    if(new_cycle == 0 && do_verbose)
	    {
		    printf("New cycle = %d\n", new_cycle);
		    printf("\t\tNum DCU = %d\n", ixDataBlock->header.dcuTotalModels);
		    printf("\t\tNew Size = %d\n", ixDataBlock->header.dataBlockSize);
		    printf("\t\tTime = %d\n", ixDataBlock->header.dcuheader[0].timeSec);
		    printf("\t\tSend Size = %d\n", sendLength);
	    }
    } while(keepRunning);

    // Cleanup the Dolphin connections
    error = dolphin_closeout();
    // Exit
    return SCI_ERR_OK;
}


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

#define DO_HANDSHAKE 0

// #define MY_DCU_OFFSET	0x1a00000
#define MY_DCU_OFFSET	0x00000
#define MY_IPC_OFFSET	(MY_DCU_OFFSET + 0x8000)
#define MY_GDS_OFFSET	(MY_DCU_OFFSET + 0x9000)
#define MY_DAT_OFFSET	(MY_DCU_OFFSET + 0xa000)

#include "./dolphin_common.c"
extern void *findSharedMemorySize(char *,int);
static volatile int keepRunning = 1;


/*********************************************************************************/
/*                                U S A G E                                      */
/*                                                                               */
/*********************************************************************************/

void Usage()
{
    printf("Usage of ix_dc_xmit:\n");
    printf("ix_multi_stream  -n <nodes> -g <group> -m <models> \n");
    printf(" -a <value>     : Local adapter number (default %d)\n", localAdapterNo);
    printf(" -g <value>     : Reflective group identifier (0..3))\n");
    printf(" -v <value>     : Diagnostics level (0..1))\n");
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
    char *mywriteaddr;
    int myCrc;
    static const int xmitDataOffset = MY_DAT_OFFSET + sizeof(struct daq_multi_cycle_header_t);
    static const int header_size = sizeof(daq_multi_dcu_header_t);
    char *sysname;
    daq_multi_dcu_data_t *ixDataBlock;
    int do_verbose = 0;
    int sendLength = 0;
    daq_multi_cycle_header_t *xmitHeader;
    // daq_multi_dcu_data_t *xmitData;
    int lastCycle = 0;
    int new_cycle;
    char *nextData;

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
            continue;

        case 'v':
            do_verbose = atoi(optarg);
            continue;

        case 'h':
            Usage();
            return(0);
    }

    // Attach to local shared memory
    char *ifo = (char *)findSharedMemorySize("ifo",100);
    // Set pointer to ifo data header
    daq_multi_cycle_header_t *ifo_header = (daq_multi_cycle_header_t *)ifo;
    // Set pointer to ifo data block
    char *ifo_data = (char *)ifo + sizeof(daq_multi_cycle_header_t);

    // Connect to Dolphin
    error = dolphin_init();
    printf("Read = 0x%lx \n Write = 0x%lx \n",(long)readAddr,(long)writeAddr);

    // Set pointer to xmit header in Dolphin xmit data area.
    mywriteaddr = (char *)writeAddr;
    mywriteaddr += MY_DAT_OFFSET;
    xmitHeader = (daq_multi_cycle_header_t *)mywriteaddr;

    // Trap control C exit
    signal(SIGINT,intHandler);

    // Sync to cycle zero in ifo shared memory (FE receive data area)
    for (;ifo_header->curCycle;) usleep(1000);
    printf("Found cycle zero = %d\n",ifo_header->curCycle);
    printf("Found cycle size = %d\n",ifo_header->cycleDataSize);
	int cyclesize = ifo_header->cycleDataSize;


    do{
	    // Wait for cycle count update in ifo shared memory
	    // Check every 2 milliseconds
	    do{
		usleep(2000);
		new_cycle = ifo_header->curCycle;
	    } while (new_cycle == lastCycle && keepRunning);
	    // Save cycle number of last received
	    lastCycle = new_cycle;
	    // Move pointer to proper data cycle in ifo memory
    	    nextData = (char *)ifo_data;
	    nextData += cyclesize * new_cycle;
	    ixDataBlock = (daq_multi_dcu_data_t *)nextData;
	    // Set the xmit data length to be header size plus data size
	    sendLength = header_size + ixDataBlock->header.dataBlockSize;

#if 0
	    // Print some diagnostics
	    // if(new_cycle == 0 && do_verbose == 1)
	    // {
		    printf("New cycle = %d\n", new_cycle);
		    printf("\t\tNum DCU = %d\n", ixDataBlock->header.dcuTotalModels);
		    printf("\t\tNew Size = %d\n", ixDataBlock->header.dataBlockSize);
		    printf("\t\tTime = %d\n", ixDataBlock->header.dcuheader[0].timeSec);
		    printf("\t\tSend Size = %d\n", sendLength);
		    printf("\t\tCycle Size = %d\n", DAQ_TRANSIT_DC_DATA_BLOCK_SIZE);
	    // }
		usleep(10000);
#endif
	
	    // WRITEDATA to Dolphin Network
	    SCIMemCpy(sequence,ixDataBlock, remoteMap,xmitDataOffset,sendLength,memcpyFlag,&error);
	    if (error != SCI_ERR_OK) {
		fprintf(stderr,"SCIMemCpy failed - Error code 0x%x\n",error);
		return error;
	    }
	    // Calculate data CRC checksum
	    myCrc = crc_ptr((char *)ixDataBlock, sendLength, 0);
	    myCrc = crc_len(sendLength, myCrc);
	    // Set data header information
	    xmitHeader->maxCycle = ifo_header->maxCycle;
	    xmitHeader->cycleDataSize = ifo_header->cycleDataSize;
	    xmitHeader->msgcrc = myCrc;
	    // Send cycle last as indication of data ready for receivers
	    xmitHeader->curCycle = ifo_header->curCycle;
	    // Have to flush the buffers to make data go onto Dolphin network
            SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
    } while(keepRunning);

    // Cleanup the Dolphin connections
    error = dolphin_closeout();
    // Exit
    return SCI_ERR_OK;
}

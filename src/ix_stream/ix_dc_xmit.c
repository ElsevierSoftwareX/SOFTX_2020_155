
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

#define DO_HANDSHAKE 0

// #define MY_DCU_OFFSET	0x1a00000
#define MY_DCU_OFFSET	0x00000
#define MY_IPC_OFFSET	(MY_DCU_OFFSET + 0x8000)
#define MY_GDS_OFFSET	(MY_DCU_OFFSET + 0x9000)
#define MY_DAT_OFFSET	(MY_DCU_OFFSET + 0xa000)

#include "./dolphin_common.c"
extern void *findSharedMemorySize(char *,int);

static char *shmDataPtr[128];
static struct cdsDaqNetGdsTpNum *shmTpTable[128];
static const int xmitDataOffset = MY_DAT_OFFSET + sizeof(struct daq_multi_cycle_header_t);
static const int header_size = sizeof(daq_multi_dcu_header_t);
char *sysname;
daq_multi_dcu_data_t *ixDataBlock;
int do_verbose = 1;
int sendLength = 0;
static volatile int keepRunning = 1;
daq_multi_cycle_header_t *xmitHeader;
daq_multi_dcu_data_t *xmitData;

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
    char *mywriteaddr;
    int myCrc;

    printf("\n %s compiled %s : %s\n\n",argv[0],__DATE__,__TIME__);
    printf("my data offset = %d\n",xmitDataOffset);
    
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
    int lastCycle = 0;
    int new_cycle;
    char *nextData;

    // Connect to Dolphin
    error = dolphin_init();
    printf("Read = 0x%lx \n Write = 0x%lx \n",(long)readAddr,(long)writeAddr);
    mywriteaddr = (char *)writeAddr;
    mywriteaddr += MY_DAT_OFFSET;
    xmitHeader = (daq_multi_cycle_header_t *)mywriteaddr;

    signal(SIGINT,intHandler);

    // Sync to cycle zero
    for (;ifo_header->curCycle;) usleep(1000);
    printf("Found cycle zero = %d\n",ifo_header->curCycle);

    // SEND CYCLE ZERO DATA HERE ****************

    do{
	    // Wait for cycle count update from FE
	    do{
		usleep(2000);
		new_cycle = ifo_header->curCycle;
	    } while (new_cycle == lastCycle && keepRunning);
	    lastCycle ++;
	    lastCycle %= 16;
    	    nextData = (char *)ifo_data;
	    nextData += DAQ_TRANSIT_DC_DATA_BLOCK_SIZE * new_cycle;
	    ixDataBlock = (daq_multi_dcu_data_t *)nextData;
	    sendLength = header_size + ixDataBlock->header.dataBlockSize;

	    if(new_cycle == 0)
	    {
		    printf("New cycle = %d\n", new_cycle);
		    printf("\t\tNum DCU = %d\n", ixDataBlock->header.dcuTotalModels);
		    printf("\t\tNew Size = %d\n", ixDataBlock->header.dataBlockSize);
		    printf("\t\tTime = %d\n", ixDataBlock->header.dcuheader[0].timeSec);
		    printf("\t\tSend Size = %d\n", sendLength);
	    }
	
	    // WRITEDATA
	    SCIMemCpy(sequence,ixDataBlock, remoteMap,xmitDataOffset,sendLength,memcpyFlag,&error);
	    if (error != SCI_ERR_OK) {
		fprintf(stderr,"SCIMemCpy failed - Error code 0x%x\n",error);
		return error;
	    }
	    myCrc = crc_ptr((char *)ixDataBlock, sendLength, 0);
	    myCrc = crc_len(sendLength, myCrc);
	    xmitHeader->curCycle = ifo_header->curCycle;
	    xmitHeader->maxCycle = ifo_header->maxCycle;
	    xmitHeader->cycleDataSize = sendLength;
	    xmitHeader->msgcrc = myCrc;
            SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
    } while(keepRunning);



    error = dolphin_closeout();
    return SCI_ERR_OK;
}

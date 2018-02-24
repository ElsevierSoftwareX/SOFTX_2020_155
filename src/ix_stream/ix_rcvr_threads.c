#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sisci_types.h"
#include "sisci_api.h"
#include "sisci_error.h"
#include "sisci_demolib.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "testlib.h"
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>


#include "../drv/crc.c"
#include "../include/daqmap.h"
#include "../include/drv/fb.h"
#include "../include/daq_core.h"



#define __CDECL


#define DO_HANDSHAKE 0

// #define MY_DCU_OFFSET	0x1a00000
#define MY_DCU_OFFSET	0x00000
#define MY_IPC_OFFSET	(MY_DCU_OFFSET + 0x1000)
#define MY_GDS_OFFSET	(MY_DCU_OFFSET + 0x9000)
#define MY_DAT_OFFSET	(MY_DCU_OFFSET + 0xa000)

#include "./dolphin_common.c"
extern void *findSharedMemorySize(char *,int);

static struct rmIpcStr *shmIpcPtr[128];
static char *shmDataPtr[128];
static struct cdsDaqNetGdsTpNum *shmTpTable[128];
static const int header_size = sizeof(struct daq_fe_header_t);
static const int buf_size = DAQ_DCU_BLOCK_SIZE;
char modelnames[DAQ_TRANSIT_MAX_DCU][64];
char *sysname;
int modelrates[DAQ_TRANSIT_MAX_DCU];
daq_multi_dcu_data_t *ixDataBlock;
static const int mdcu_header_size = sizeof(struct daq_multi_dcu_header_t);
daq_fe_data_t *zbuffer;
unsigned int            loops          = 170;
static const int ipcSize = sizeof(struct daq_msg_header_t);
unsigned int tstatus[16];
int thread_index[DCU_COUNT];
daq_multi_dcu_data_t mxDataBlockG[32][16];
int stop_working_threads = 0;
int start_acq = 0;
static volatile int keepRunning = 1;
daq_multi_cycle_data_t *mcd;


int waitSender(int rank,sci_sequence_t sequence,volatile unsigned int *readAddr,volatile unsigned int *writeAddr)
{

        int wait_loops = 0;
	int value;

        
        /* Lets write CMD_READY the to client, offset "myrank" */
        *(writeAddr+IX_SYNC_OFFSET+rank) = CMD_READY;
        SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
        
        printf("Wait for CMD_READY from master ...\n");

        /* Lets wait for the client to send me CMD_READY in offset 0 */
        do {
            
            value = (*(readAddr+IX_SYNC_OFFSET));  
            wait_loops++;

            if ((wait_loops % 100000000)==0) {
                /* Lets again write CMD_READY to the client, offset "myrank" */
                *(writeAddr+IX_SYNC_OFFSET+rank) = CMD_READY;
                SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
            }

        } while (value != CMD_READY);
        printf("Server received CMD_READY\n");
        *(writeAddr+IX_SYNC_OFFSET+rank) = 0;
        SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
}

void intHandler(int dummy) {
        keepRunning = 0;
}

static int64_t
s_clock (void)
{
struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
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



void *ix_rcvr_thread(void *arg)
{
    
    unsigned int          value;
    unsigned int          written_value = 0;
    double                average;
    int                   verbose = 1;
    int                   node_offset;
    int *mythread = (int *)arg;
    int mt = *mythread;
    int ii;
    int cycle = 0;
    int lastCycle = 0;
    int msgSize = 0;
    volatile unsigned char *daq_read_addr;
    daq_fe_data_t *ixbuffer;
    volatile unsigned int *myreadAddr;
    volatile unsigned int *mywriteAddr;

    myreadAddr = (unsigned int *)readAddr;
    mywriteAddr = (unsigned int *)writeAddr;

    daq_read_addr = (unsigned char *)readAddr + MY_DAT_OFFSET;
    ixbuffer = (daq_fe_data_t *)daq_read_addr;
    ixbuffer += mt;

    printf("Read = 0x%lx \n",(long)ixbuffer);

    /* Perform a barrier operation. The client acts as master. */
	waitSender(rank,sequence,readAddr,writeAddr);
    
    printf("\n***********************************************************\n\n");
    
    printf("Loops: %d\n", loops);
    
    mywriteAddr += 256;
    myreadAddr += 256;
    
    written_value=1;

    do {
        
            int wait_loops = 0;
            
            /* Lets wait for the client to send me a value in offset 0 */
            
            if (!verbose) {
                printf("Wait for broadcast message...\n");
            }
            
            do {
                
                value = (*(myreadAddr));  
                wait_loops++;
                // if ((wait_loops % 10000000)==0) {
                   //  printf("Value = %d delayed from Client - written_value=%d\n", value,written_value);
                // }
            } while (value != written_value); 

	    cycle = ixbuffer->header.dcuheader[0].cycle;
	    msgSize = header_size + ixbuffer->header.dataBlockSize;
	    char *localbuff = (char *)&mxDataBlockG[mt][cycle];
	    memcpy(localbuff,ixbuffer,msgSize);

#if 0
            
	    printf("zbuff count = %d\n",ixbuffer->header.dcuTotalModels);
	    printf("zbuff size  = %d\n",ixbuffer->header.dataBlockSize);
	    printf("zbuff cycle  = %d\n",cycle);
	    printf("zbuff dcuid  = %d\n",ixbuffer->header.dcuheader[0].dcuId);
	    printf("buff size  = %d\t%d\t%d\n",msgSize,header_size,ipcSize);

#endif
            if (!verbose) {
                printf("Received broadcast value %d \n",cycle); 
            }
	     tstatus[cycle] |= (1 << mt);
            
            written_value++;
            /* Lets write a value back received value +1 to the client, offset "myrank" */
            *(mywriteAddr+rank) = written_value;
            SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);


    } while (!stop_working_threads); /* do this number of loops */
    
    printf("Stopping thread %d\n",mt);
    usleep(200000);
    
    return 0;
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
    pthread_t thread_id[4];
    int ii,jj;
    int dataRdy = 0;
    int loop = 0;
    daq_msg_header_t *sendheader;
    daq_msg_header_t *rcvheader;
    char *senddata;
    char *rcvdata;
    int myc;
    char *mcdDataPtr;

    printf("\n %s compiled %s : %s\n\n",argv[0],__DATE__,__TIME__);
    printf("Size of mcd = %d\n",sizeof(mcd));
    
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

        if (!strcmp("-help",argv[counter])) {
            Usage();
            return(0);
        }
    }

    signal(SIGINT,intHandler);
    error = dolphin_init();
    printf("Read = 0x%lx \n Write = 0x%lx \n",(long)readAddr,(long)writeAddr);

    char *ifo = (char *)findSharedMemorySize("ifo",100);
    daq_multi_cycle_header_t *ifo_header = (daq_multi_cycle_header_t *)ifo;
    char *ifo_data = (char *)ifo + sizeof(daq_multi_cycle_header_t);

    printf("Starting recvr threads\n");
    for(ii=0;ii<nodes;ii++)
    {
    	thread_index[ii] = ii;
	pthread_create(&thread_id[ii],NULL,ix_rcvr_thread,(void *)&thread_index);
	dataRdy |= (1 << ii);
    }


	int timeout = 0;
	int resync = 1;
	int64_t mytime = 0;
        int64_t mylasttime = 0;
        int64_t myptime = 0;
        int mytotaldcu = 0;

	do {
		if(resync) {
			loop = 0;
			do {
				usleep(2000);
				timeout += 1;
			}while(tstatus[loop] == 0 && timeout < 5000);
			for(ii=0;ii<16;ii++) tstatus[ii] = 0;
			printf("RESYNC ***************** \n");
		}
		// Wait until received data from at least 1 FE
		timeout = 0;
		do {
			usleep(2000);
			timeout += 1;
		}while(tstatus[loop] == 0 && timeout < 5000 && !resync);
		resync = 0;
		// If timeout, not getting data from anyone.
		if(timeout >= 5000) resync = 1;
		if (resync) continue;

		// Wait until data received from everyone
		timeout = 0;
		do {
			usleep(1000);
			timeout += 1;
		}while(tstatus[loop] != dataRdy && timeout < 5);
		// If timeout, not getting data from everyone.
		// TODO: MARK MISSING FE DATA AS BAD
		 
		// Clear thread rdy for this cycle
		tstatus[loop] = 0;

		// Timing diagnostics
		mytime = s_clock();
		myptime = mytime - mylasttime;
		mylasttime = mytime;
		if(loop == 0) {
			printf("Data rdy for cycle = %d\t%ld\n",mxDataBlockG[0][loop].header.dcuheader[0].timeSec,myptime);
			printf("\tdatasize = %d\n",mxDataBlockG[0][loop].header.dataBlockSize);
		}
		// Reset total DCU counter
		mytotaldcu = 0;
    // daq_multi_cycle_header_t *ifo_header = (daq_multi_cycle_header_t header *)ifo;
    // char *ifo_data = (char *)ifo + sizeof(daq_multi_cycle_header_t);
    		mcdDataPtr = ifo_data;
		mcdDataPtr += loop * DAQ_TRANSIT_DC_DATA_BLOCK_SIZE;
		ixDataBlock = (daq_multi_dcu_data_t *)mcdDataPtr;
		sendheader = (daq_msg_header_t *) &ixDataBlock->header.dcuheader[0];
		senddata = (char *) &ixDataBlock->dataBlock[0];
		int sendDataBlockSize = 0;
		// Loop over all data buffers received from FE computers
		for(ii=0;ii<nodes;ii++) {
		// for(jj=0;jj<2;jj++) {
			// Get pointers to receiver header and data blocks
			rcvheader = (daq_msg_header_t *) &mxDataBlockG[ii][loop].header.dcuheader[0];
			rcvdata = (char *) &mxDataBlockG[ii][loop].dataBlock[0];
			// Get the receive number of dcu
			myc = mxDataBlockG[ii][loop].header.dcuTotalModels;
			// Add rcv dcu count to my total send count
			mytotaldcu += myc;
			// Calc size of receive header data to move
			int headersize = myc * ipcSize;
			// Copy rcv header area to send header
			memcpy(sendheader,rcvheader,headersize);
			// Increment send header for next dcu
			sendheader += myc;
			// Get the size of received data to move
			sendDataBlockSize +=  mxDataBlockG[ii][loop].header.dataBlockSize;
			// Copy rcv data to send data block
			memcpy(senddata,rcvdata,sendDataBlockSize);
			// Update the send header with dcu and size info
			ixDataBlock->header.dcuTotalModels = mytotaldcu;
			ixDataBlock->header.dataBlockSize = sendDataBlockSize;
		// }
		}
		ifo_header->curCycle = loop;
		ifo_header->cycleDataSize = DAQ_TRANSIT_DC_DATA_BLOCK_SIZE;
		ifo_header->maxCycle = 16;
		
#if 0
		if(loop == 0) {
			printf("New Send Data:\n");
			printf("\tNum of DCU: %d\n",ixDataBlock->header.dcuTotalModels);
			printf("\tDataSize: %d\n",ixDataBlock->header.dataBlockSize);
			for(ii=0;ii<ixDataBlock->header.dcuTotalModels;ii++) 
			{
				printf("\tDCUID %d = %d\n",ii,ixDataBlock->header.dcuheader[ii].cycle);
			}
		}
#endif

		loop ++;
		loop %= 16;
	}while (keepRunning);

	printf("stopping threads %d \n",nodes);
	stop_working_threads = 1;

	// Wait for threads to stop
	sleep(2);
    
    error = dolphin_closeout();

    return SCI_ERR_OK;
}

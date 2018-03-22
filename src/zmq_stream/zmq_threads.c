//
///	@file zmq_multi_rcvr.c
///	@brief  Test DAQ data receiver using ZeroMQ.
//

#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "../drv/crc.c"
#include <zmq.h>
#include <assert.h>
#include <time.h>
#include "zmq_daq.h"
#include "../include/daqmap.h"
#include "../include/daq_core.h"


extern void *findSharedMemorySize(char *,int);

int do_verbose;

int thread_index;
// int thread_index[DCU_COUNT];
void *daq_context[DCU_COUNT];
void *daq_subscriber[DCU_COUNT];
daq_multi_dcu_data_t mxDataBlockSingle[32];
const int mc_header_size = sizeof(daq_multi_cycle_header_t);
int stop_working_threads = 0;
int start_acq = 0;
static volatile int keepRunning = 1;
int thread_cycle[32];
int thread_timestamp[32];

void
usage()
{
	fprintf(stderr, "Usage: zmq_multi_rcvr [args] -s server name\n");
	fprintf(stderr, "-l filename - log file name\n"); 
	fprintf(stderr, "-s - server name eg x1lsc0, x1susex, etc.\n");
	fprintf(stderr, "-v - verbose prints diag test data\n");
	fprintf(stderr, "-h - help\n");
}

static int64_t
s_clock (void)
{
struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void intHandler(int dummy) {
	keepRunning = 0;
}

// *************************************************************************
// Thread for receiving DAQ data via ZMQ
// *************************************************************************
void *rcvr_thread(void *arg) {
	int *mythread = (int *)arg;
	int mt = *mythread;
	printf("myarg = %d\n",mt);
	zmq_msg_t message;
	int cycle = 0;
	daq_multi_dcu_data_t *mxDataBlock;

	printf("Starting receive loop for thread %d\n", mt);
	char *daqbuffer = (char *)&mxDataBlockSingle[mt];
	mxDataBlock = (daq_multi_dcu_data_t *)daqbuffer;
	do {
		// Initialize receiver buffer
		zmq_msg_init(&message);
		// Get data when message size > 0
		int size = zmq_msg_recv(&message,daq_subscriber[mt],0);
		assert(size >= 0);
		// Get pointer to message data
		char *string = (char *)zmq_msg_data(&message);
		// Copy data out of 0MQ message buffer to local memory buffer
		memcpy(daqbuffer,string,size);
		// Destroy the received message buffer
		zmq_msg_close(&message);

		// Get the message DAQ cycle number
		cycle = mxDataBlock->header.dcuheader[0].cycle;
		// Pass cycle and timestamp data back to main process
		thread_cycle[mt] = cycle;
		thread_timestamp[mt] = mxDataBlock->header.dcuheader[0].timeSec;

	// Run until told to stop by main thread
	} while(!stop_working_threads);
	printf("Stopping thread %d\n",mt);
	usleep(200000);
	return(0);

}

// *************************************************************************
// Main Process
// *************************************************************************
int
main(int argc, char **argv)
{
	pthread_t thread_id[32];
	unsigned int nsys = 1; // The number of mapped shared memories (number of data sources)
	char *sysname;
	char *sname[DCU_COUNT];	// Names of FE computers serving DAQ data 
	int c;
	int ii;					// Loop counter

	extern char *optarg;	// Needed to get arguments to program

	// Declare shared memory data variables
	daq_multi_cycle_header_t *ifo_header;
	char *ifo;
	char *ifo_data;
	size_t cycle_data_size;
	daq_multi_dcu_data_t *ifoDataBlock;
	char *nextData;
	int max_data_size = 100;

	// Declare 0MQ message pointers
	int rc;
	char loc[40];

	/* set up defaults */
	sysname = NULL;


	// Get arguments sent to process
	while ((c = getopt(argc, argv, "hd:s:m:l:Vvw:x")) != EOF) switch(c) {
	case 's':
		sysname = optarg;
		break;
	case 'v':
		do_verbose = 1;
		break;
	case 'm':
		max_data_size = atoi(optarg);
    	if (max_data_size < 20){
        	printf("Min data block size is 20 MB\n");
            return -1;
        }
        if (max_data_size > 100){
            printf("Max data block size is 100 MB\n");
            return -1;
        }
        break;
	case 'h':
	default:
		usage();
		exit(1);
	}

	if (sysname == NULL) { usage(); exit(1); }

	// set up to catch Control C
	signal(SIGINT,intHandler);

	printf("Server name: %s\n", sysname);

	// Parse names of data servers
	sname[0] = strtok(sysname, " ");
        for(;;) {
                printf("%s\n", sname[nsys - 1]);
                char *s = strtok(0, " ");
                if (!s) break;
				// do not overflow our fixed size buffers
				assert(nsys < DCU_COUNT);
                sname[nsys] = s;
                nsys++;
        }

	// Get pointers to local DAQ mbuf
    ifo = (char *)findSharedMemorySize("ifo",max_data_size);
    ifo_header = (daq_multi_cycle_header_t *)ifo;
    ifo_data = (char *)ifo + sizeof(daq_multi_cycle_header_t);
    cycle_data_size = (max_data_size - sizeof(daq_multi_cycle_header_t))/DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    cycle_data_size -= (cycle_data_size % 8);
	ifo_header->cycleDataSize = cycle_data_size;
    ifo_header->maxCycle = DAQ_NUM_DATA_BLOCKS_PER_SECOND;


	printf("nsys = %d\n",nsys);
	for(ii=0;ii<nsys;ii++) {
		printf("sys %d = %s\n",ii,sname[ii]);
	}

	// Make 0MQ socket connections
	for(ii=0;ii<nsys;ii++) {
		// Make 0MQ socket connection for rcvr threads
		daq_context[ii] = zmq_ctx_new();
		daq_subscriber[ii] = zmq_socket (daq_context[ii],ZMQ_SUB);

		// Subscribe to all data from the server
		rc = zmq_setsockopt(daq_subscriber[ii],ZMQ_SUBSCRIBE,"",0);
		assert (rc == 0);

		// connect to the publisher
		sprintf(loc,"%s%s%s%d","tcp://",sname[ii],":",DAQ_DATA_PORT);
		printf("sys connection %d = %s ...",ii,loc);
		rc = zmq_connect (daq_subscriber[ii], loc);
		assert (rc == 0);
		printf(" done\n");

		// Create a thread to receive data from each data server
		thread_index = ii;
		pthread_create(&thread_id[ii],NULL,rcvr_thread,(void *)&thread_index);
	}

	int nextCycle = 0;
	start_acq = 1;
	int64_t mytime = 0;
	int64_t mylasttime = 0;
	int64_t myptime = 0;
	int mytotaldcu = 0;
	char *zbuffer;
	int dc_datablock_size = 0;
	static const int header_size = sizeof(daq_multi_dcu_header_t);
	char dcstatus[2024];
	char dcs[24];
	int edcuid[10];
	int estatus[10];
	int edbs[10];
	unsigned long ets = 0;
	int timeout = 0;
	int dataRdy[32];
	int threads_rdy;
	int any_rdy = 0;

	do {
		// Reset counters
		timeout = 0;
		for(ii=0;ii<nsys;ii++) dataRdy[ii] = 0;
		for(ii=0;ii<nsys;ii++) thread_cycle[ii] = 50;
		threads_rdy = 0;
		any_rdy = 0;
		// Wait until received data from at least 1 FE or timeout
		do {
			usleep(2000);
			for(ii=0;ii<nsys;ii++) {
				if(nextCycle == thread_cycle[ii]) any_rdy = 1;
			}
			timeout += 1;
		}while(!any_rdy && timeout < 50);

		// Wait until data received from everyone or timeout
		timeout = 0;
		do {
			usleep(100);
			for(ii=0;ii<nsys;ii++) {
				if(nextCycle == thread_cycle[ii] && !dataRdy[ii]) threads_rdy ++;
				if(nextCycle == thread_cycle[ii]) dataRdy[ii] = 1;
			}
			timeout += 1;
		}while(threads_rdy < nsys && timeout < 50);

		if(any_rdy) {
			// Timing diagnostics
			mytime = s_clock();
			myptime = mytime - mylasttime;
			mylasttime = mytime;
			if(do_verbose)printf("Data rdy for cycle = %d\t\t%ld\n",nextCycle,myptime);
			// Reset total DCU counter
			mytotaldcu = 0;
			// Reset total DC data size counter
			dc_datablock_size = 0;
			// Get pointer to next data block in shared memory
			nextData = (char *)ifo_data;
        	nextData += cycle_data_size * nextCycle;
        	ifoDataBlock = (daq_multi_dcu_data_t *)nextData;
			zbuffer = (char *)nextData + header_size;

			// Loop over all data buffers received from FE computers
			for(ii=0;ii<nsys;ii++) {
		  		if(dataRdy[ii]) {
					int myc = mxDataBlockSingle[ii].header.dcuTotalModels;
					// printf("\tModel %d = %d\n",ii,myc);
					// For each model, copy over data header information
					for(int jj=0;jj<myc;jj++) {
						// Copy data header information
						ifoDataBlock->header.dcuheader[mytotaldcu].dcuId = mxDataBlockSingle[ii].header.dcuheader[jj].dcuId;
						ifoDataBlock->header.dcuheader[mytotaldcu].fileCrc = mxDataBlockSingle[ii].header.dcuheader[jj].fileCrc;
						ifoDataBlock->header.dcuheader[mytotaldcu].status = mxDataBlockSingle[ii].header.dcuheader[jj].status;
						ifoDataBlock->header.dcuheader[mytotaldcu].dataCrc = mxDataBlockSingle[ii].header.dcuheader[jj].dataCrc;
						ifoDataBlock->header.dcuheader[mytotaldcu].cycle = mxDataBlockSingle[ii].header.dcuheader[jj].cycle;
						ifoDataBlock->header.dcuheader[mytotaldcu].timeSec = mxDataBlockSingle[ii].header.dcuheader[jj].timeSec;
						ifoDataBlock->header.dcuheader[mytotaldcu].timeNSec = mxDataBlockSingle[ii].header.dcuheader[jj].timeNSec;
						ifoDataBlock->header.dcuheader[mytotaldcu].dataBlockSize = mxDataBlockSingle[ii].header.dcuheader[jj].dataBlockSize;
						// Get some diags
						if(ifoDataBlock->header.dcuheader[mytotaldcu].status == 0xbad)
							printf("Fault on dcuid %d\n",ifoDataBlock->header.dcuheader[mytotaldcu].dcuId );
						else ets = mxDataBlockSingle[ii].header.dcuheader[jj].timeSec;
						estatus[mytotaldcu] = ifoDataBlock->header.dcuheader[mytotaldcu].status;
						edcuid[mytotaldcu] = ifoDataBlock->header.dcuheader[mytotaldcu].dcuId;
						mytotaldcu ++;
						// printf("\t\tdcuid = %d\n",mydbs);
					}
					// Get the size of the data to transfer
					int mydbs = mxDataBlockSingle[ii].header.dataBlockSize;
					edbs[mytotaldcu] = mydbs;
					// Get pointer to data in receive data block
					char *mbuffer = (char *)&mxDataBlockSingle[ii].dataBlock[0];
					// Copy data from receive buffer to shared memory
					memcpy(zbuffer,mbuffer,mydbs);
					// Increment shared memory data buffer pointer for next data set
					zbuffer += mydbs;
					// Calc total size of data block for this cycle
					dc_datablock_size += mydbs;
		  		}
			}
			// Write total data block size to shared memory header
			ifoDataBlock->header.dataBlockSize = dc_datablock_size;
			// Write total dcu count to shared memory header
			ifoDataBlock->header.dcuTotalModels = mytotaldcu;
			// Set multi_cycle head cycle to indicate data ready for this cycle
			ifo_header->curCycle = nextCycle;
			if(do_verbose)printf("\tTotal DCU = %d\t\t\tSize = %d\n",mytotaldcu,dc_datablock_size);
		}

		sprintf(dcstatus,"%ld ",ets);
		for(ii=0;ii<mytotaldcu;ii++) {
			sprintf(dcs,"%d %d %d ",edcuid[ii],estatus[ii],edbs[ii]);
			strcat(dcstatus,dcs);
		}

		// Increment cycle count
		nextCycle ++;
		nextCycle %= 16;
	}while (keepRunning);	// End of infinite loop

	printf("stopping threads %d \n",nsys);
	stop_working_threads = 1;

	// Wait for threads to stop
	sleep(2);
	printf("closing out zmq\n");
	// Close out ZMQ connections
	for(ii=0;ii<nsys;ii++) {
		zmq_close(daq_subscriber[ii]);
		zmq_ctx_destroy(daq_context[ii]);
	}
  
	exit(0);
}

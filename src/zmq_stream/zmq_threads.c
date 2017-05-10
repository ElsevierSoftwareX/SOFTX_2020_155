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


int do_verbose;
unsigned int do_wait = 0; // Wait for this number of milliseconds before starting a cycle

extern void *findSharedMemory(char *);
unsigned int tstatus[16];
void *daq_context[DCU_COUNT];
void *daq_subscriber[DCU_COUNT];
daq_multi_dcu_data_t mxDataBlockFull[16];
daq_multi_dcu_data_t mxDataBlockG[12][16];
int stop_working_threads = 0;
int start_acq = 0;
static volatile int keepRunning = 1;

void
usage()
{
	fprintf(stderr, "Usage: zmq_multi_rcvr [args] -s server name\n");
	fprintf(stderr, "-l filename - log file name\n"); 
	fprintf(stderr, "-s - server name eg x1lsc0, x1susex, etc.\n");
	fprintf(stderr, "-v - verbose prints cpu_meter test data\n");
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

void *rcvr_thread(void *arg) {
	int *mythread = (int *)arg;
	int mt = *mythread;
	printf("myarg = %d\n",mt);
	zmq_msg_t message;
	int ii;
	int cycle = 0;
	int acquire = 0;
	daq_multi_dcu_data_t mxDataBlock;

	do {
		zmq_msg_init(&message);
		// Get data when message size > 0
		int size = zmq_msg_recv(&message,daq_subscriber[mt],0);
		assert(size >= 0);
		// Get pointer to message data
		char *string = (char *)zmq_msg_data(&message);
		char *daqbuffer = (char *)&mxDataBlock;
		// Copy data out of 0MQ message buffer to local memory buffer
		memcpy(daqbuffer,string,size);
		// Destroy the received message buffer
		zmq_msg_close(&message);
		for (ii = 0;ii<mxDataBlock.dcuTotalModels;ii++) {
			cycle = mxDataBlock.zmqheader[ii].cycle;
			// Copy data to global buffer
			char *localbuff = (char *)&mxDataBlockG[mt][cycle];
			memcpy(localbuff,daqbuffer,size);
		}
		// Always start on cycle 0 after told to start by main thread
		if(cycle == 0 && start_acq) acquire = 1;
		// Set the cycle data ready bit
		if(acquire)  {
			tstatus[cycle] |= (1 << mt);
		}
	// Run until told to stop by main thread
	} while(!stop_working_threads);
	printf("Stopping thread %d\n",mt);
	usleep(200000);
	return(0);

}

int
main(int argc, char **argv)
{


	pthread_t thread_id[4];
	unsigned int nsys = 1; // The number of mapped shared memories (number of data sources)
	char *sysname;
	char *sname[DCU_COUNT];
	int c;
	int dataRdy = 0;

	extern char *optarg;

	// Create DAQ message area in local memory
	daq_multi_dcu_data_t mxDataBlock;
	// Declare pointer to local memory message area
	printf("size of mxdata = %ld\n",sizeof(mxDataBlock));


	/* set up defaults */
	sysname = NULL;
	int ii;

	// Declare 0MQ message pointers
	int rc;
	char loc[40];


	while ((c = getopt(argc, argv, "hd:s:l:Vvw:x")) != EOF) switch(c) {
	case 's':
		sysname = optarg;
		break;
	case 'v':
		do_verbose = 1;
		break;
	case 'w':
		do_wait = atoi(optarg);
		break;
	case 'h':
	default:
		usage();
		exit(1);
	}

	if (sysname == NULL) { usage(); exit(1); }

	signal(SIGINT,intHandler);

	printf("Server name: %s\n", sysname);

	sname[0] = strtok(sysname, " ");
        for(;;) {
                printf("%s\n", sname[nsys - 1]);
                char *s = strtok(0, " ");
                if (!s) break;
                sname[nsys] = s;
                nsys++;
        }


	printf("nsys = %d\n",nsys);
	for(ii=0;ii<nsys;ii++) {
		printf("sys %d = %s\n",ii,sname[ii]);
	}
		// Make 0MQ socket connection
	for(ii=0;ii<nsys;ii++) {
		// Make 0MQ socket connection for rcvr threads
		daq_context[ii] = zmq_ctx_new();
		daq_subscriber[ii] = zmq_socket (daq_context[ii],ZMQ_SUB);
		sprintf(loc,"%s%s%s%d","tcp://",sname[ii],":",DAQ_DATA_PORT);
		printf("sys %d = %s\n",ii,loc);
		rc = zmq_connect (daq_subscriber[ii], loc);
		assert (rc == 0);
		// Subscribe to all data from the server
		rc = zmq_setsockopt(daq_subscriber[ii],ZMQ_SUBSCRIBE,"",0);
		assert (rc == 0);
		int tnum = ii;
		pthread_create(&thread_id[ii],NULL,rcvr_thread,(void *)&tnum); 
		dataRdy |= (1 << ii);
	}

	// Create 0MQ socket for DC data transmission
	void *dc_context;
	void *dc_publisher;

	dc_context = zmq_ctx_new();
	dc_publisher = zmq_socket(dc_context,ZMQ_PUB);
	sprintf(loc,"%s%d","tcp://eth2:",DAQ_DATA_PORT);
	rc = zmq_bind (dc_publisher,loc);
	assert(rc == 0);


	int loop = 0;
	start_acq = 1;
	int64_t mytime = 0;
	int64_t mylasttime = 0;
	int64_t myptime = 0;
	int mytotaldcu = 0;
	char *zbuffer;
	int dc_datablock_size = 0;
	char buffer[DAQ_ZMQ_DATA_BLOCK_SIZE];
	static const int header_size = DAQ_ZMQ_HEADER_SIZE;
	int sendLength = 0;
	int msg_size = 0;

	do {
		do {
			usleep(2000);
		}while(tstatus[loop] != dataRdy);
		tstatus[loop] = 0;
		// Timing diagnostics
		mytime = s_clock();
		myptime = mytime - mylasttime;
		mylasttime = mytime;
		printf("Data rday for cycle = %d\t%ld\n",loop,myptime);
		// Reset total DCU counter
		mytotaldcu = 0;
		// Set pointer to start of DC data block
		zbuffer = (char *)&mxDataBlockFull[loop].zmqDataBlock[0];
		// Reset total DC data size counter
		dc_datablock_size = 0;
		// Loop over all data buffers received from FE computers
		for(ii=0;ii<nsys;ii++) {
			int myc = mxDataBlockG[ii][loop].dcuTotalModels;
			// printf("\tModel %d = %d\n",ii,myc);
			for(int jj=0;jj<myc;jj++) {
				// Copy data header information
				mxDataBlockFull[loop].zmqheader[mytotaldcu].dcuId = mxDataBlockG[ii][loop].zmqheader[jj].dcuId;
				mxDataBlockFull[loop].zmqheader[mytotaldcu].fileCrc = mxDataBlockG[ii][loop].zmqheader[jj].fileCrc;
				mxDataBlockFull[loop].zmqheader[mytotaldcu].status = mxDataBlockG[ii][loop].zmqheader[jj].status;
				mxDataBlockFull[loop].zmqheader[mytotaldcu].cycle = mxDataBlockG[ii][loop].zmqheader[jj].cycle;
				mxDataBlockFull[loop].zmqheader[mytotaldcu].timeSec = mxDataBlockG[ii][loop].zmqheader[jj].timeSec;
				mxDataBlockFull[loop].zmqheader[mytotaldcu].timeNSec = mxDataBlockG[ii][loop].zmqheader[jj].timeNSec;
				int mydbs = mxDataBlockG[ii][loop].zmqheader[jj].dataBlockSize;
				// printf("\t\tdcuid = %d\n",mydbs);
				mxDataBlockFull[loop].zmqheader[mytotaldcu].dataBlockSize = mydbs;
				char *mbuffer = (char *)&mxDataBlockG[ii][loop].zmqDataBlock[0];
				// Copy data to DC buffer
				memcpy(zbuffer,mbuffer,mydbs);
				// Increment DC data buffer pointer for next data set
				zbuffer += mydbs;
				dc_datablock_size += mydbs;
				mytotaldcu ++;
			}
		}
		printf("\tTotal DCU = %d\tSize = %d\n",mytotaldcu,dc_datablock_size);
		sendLength = header_size + dc_datablock_size;
		zbuffer = (char *)&mxDataBlockFull[loop];
		// Copy DC data to 0MQ message block
		memcpy(buffer,zbuffer,sendLength);
		// Xmit the DC data block
		msg_size = zmq_send(dc_publisher,buffer,sendLength,0);

		loop ++;
		loop %= 16;
	}while (keepRunning);

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
	zmq_close(dc_publisher);
	zmq_ctx_destroy(dc_context);
  
	exit(0);
}

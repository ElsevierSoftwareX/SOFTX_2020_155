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
#include "zmq_daq.h"


int do_verbose;
unsigned int do_wait = 0; // Wait for this number of milliseconds before starting a cycle

extern void *findSharedMemory(char *);

void
usage()
{
	fprintf(stderr, "Usage: zmq_multi_rcvr [args] -s server name\n");
	fprintf(stderr, "-l filename - log file name\n"); 
	fprintf(stderr, "-s - server name eg x1lsc0, x1susex, etc.\n");
	fprintf(stderr, "-v - verbose prints cpu_meter test data\n");
	fprintf(stderr, "-h - help\n");
}


unsigned int nsys; // The number of mapped shared memories (number of data sources)
static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;
static const int header_size = sizeof(daq_msg_header_t) + 4;

int
main(int argc, char **argv)
{


	char *sysname;
	int c;

	extern char *optarg;

	// Create DAQ message area in local memory
	daq_multi_dcu_data_t mxDataBlock;
	// Declare pointer to local memory message area
	char *daqbuffer = (char *)&mxDataBlock;


	/* set up defaults */
	sysname = NULL;
	int myErrorSignal = 0;
	int size;
	char *dataPtr;
	int ii;

	// Declare 0MQ message pointers
	void *daq_context;
	void *daq_subscriber;
	int rc;
	zmq_msg_t message;

	// Test pointer to cpu meter data
	int *cpu_meter;
	int mycpu[6];

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

	printf("Server name: %s\n", sysname);



	// Make 0MQ socket connection
	daq_context = zmq_ctx_new();
	daq_subscriber = zmq_socket (daq_context,ZMQ_SUB);
	char loc[20];
	sprintf(loc,"%s%s%s%d","tcp://",sysname,":",DAQ_DATA_PORT);
	rc = zmq_connect (daq_subscriber, loc);
	assert (rc == 0);
	// Subscribe to all data from the server
	rc = zmq_setsockopt(daq_subscriber,ZMQ_SUBSCRIBE,"",0);
	assert (rc == 0);


	// Receive DAQ data in an infinite loop ***********************************

	do {
		// Initialize 0MQ message buffer
		zmq_msg_init(&message);
		// Get data when message size > 0
		size = zmq_msg_recv(&message,daq_subscriber,0);
		assert(size >= 0);
		// Get pointer to message data
		char *string = (char *)zmq_msg_data(&message);
		// Copy data out of 0MQ message buffer to local memory buffer
		memcpy(daqbuffer,string,size);
		// Destroy the received message buffer
		zmq_msg_close(&message);
		// *******************************************************************
		// Following is test finding cpu meter data
		// Set data pointer to start of received data block
		if(do_verbose) {
			dataPtr = (char *)&mxDataBlock.zmqDataBlock[0];;
			for(ii=0;ii<mxDataBlock.dcuTotalModels;ii++) {
				// Increment data pointer to start of next FE data block
				if(ii>0) dataPtr += mxDataBlock.zmqheader[ii-1].dataBlockSize;
				// Extract the cpu meter data for each FE
				cpu_meter = (int *)dataPtr;
				cpu_meter += 2;
				mycpu[ii] = *cpu_meter;
			}
			// Print the CPU METER info on each 1 second mark
			if(mxDataBlock.zmqheader[0].cycle == 0) {
				printf("\n*******************************************************\n");
				printf("Total DCU = %d\n", mxDataBlock.dcuTotalModels);
				for(ii=0;ii<mxDataBlock.dcuTotalModels;ii++) {
					printf("DCU = %d\tCPU METER = \t%d\n",mxDataBlock.zmqheader[ii].dcuId,mycpu[ii]);
					printf("\tTime = %d %d\tSize = \t%d bytes\n",
						mxDataBlock.zmqheader[ii].timeSec,
						mxDataBlock.zmqheader[ii].timeNSec,
						mxDataBlock.zmqheader[ii].dataBlockSize);
				}
			}
		}
		// *******************************************************************

	}while(!myErrorSignal);

	zmq_close(daq_subscriber);
	zmq_ctx_destroy(daq_context);
  
	exit(0);
}

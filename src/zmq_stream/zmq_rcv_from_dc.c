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
#include "../include/daqmap.h"


int do_verbose;
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

void intHandler(int dummy) {
        keepRunning = 0;
}



int
main(int argc, char **argv)
{


	char *sysname;
	int c;
	static const int header_size = sizeof(daq_multi_dcu_header_t);

	extern char *optarg;

	// Create DAQ message area in local memory
	daq_dc_data_t mxDataBlock;
	// Declare pointer to local memory message area
	printf("size of mxdata = %ld\n",sizeof(mxDataBlock));


	/* set up defaults */
	sysname = NULL;
	int size;
	int ii;

	// Declare 0MQ message pointers
	void *daq_context;
	void *daq_subscriber;
	int rc;
	zmq_msg_t message;
	char loc[32];


	while ((c = getopt(argc, argv, "hd:s:l:Vv:x")) != EOF) switch(c) {
	case 's':
		sysname = optarg;
		break;
	case 'v':
		do_verbose = 1;
		break;
	case 'h':
	default:
		usage();
		exit(1);
	}

	if (sysname == NULL) { usage(); exit(1); }

	printf("Server name: %s\n", sysname);

	signal(SIGINT,intHandler);

	//
	// Make 0MQ socket connection
	daq_context = zmq_ctx_new();
	daq_subscriber = zmq_socket (daq_context,ZMQ_SUB);
	sprintf(loc,"%s%s%s%d","tcp://",sysname,":",DAQ_DATA_PORT);
	printf("sys %d = %s\n",ii,loc);
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
		char *daqbuffer = (char *)&mxDataBlock;
		// Copy data out of 0MQ message buffer to local memory buffer
		memcpy(daqbuffer,string,size);
		// Destroy the received message buffer
		zmq_msg_close(&message);
		printf("RCV with tdcu = %d\n",mxDataBlock.header.dcuTotalModels);

		// *******************************************************************

	}while(keepRunning);

	zmq_close(daq_subscriber);
	zmq_ctx_destroy(daq_context);
  
	exit(0);
}

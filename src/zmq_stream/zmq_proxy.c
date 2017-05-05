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
typedef struct channel_t {
	char name[64];
	int type;
	int value;
}channel_t;

int
main(int argc, char **argv)
{


	char *sysname;
	int c;

	extern char *optarg;
	channel_t ndsdata[2000];
	channel_t *ndsptr;
	FILE *fr;
	char line[80];
	int ii = 0;
	int totalchans = 0;
	char tmpname[64];
	int tmpdatatype = 0;
	int tmpdatarate = 0;
	int lft = 0;

	fr = fopen("/opt/rtcds/tst/x1/chans/daq/X1ATS.ini","r");
	while(fgets(line,80,fr) != NULL) {
		if(strstr(line,"X1") != NULL && strstr(line,"#") == NULL) { 
			int sl = strlen(line) - 2;
			memmove(line, line+1, sl);
			line[sl-1] = 0;
			// printf("%s\n",line);
			sprintf(tmpname,"%s",line);
			lft = 1;
		}
		if(strstr(line,"datarate") != NULL && strstr(line,"#") == NULL && lft) { 
			if(strstr(line,"16") != NULL) tmpdatarate = 16;
			else lft = 0;
		}
		if(strstr(line,"datatype") != NULL && strstr(line,"#") == NULL && lft) { 
			if(strstr(line,"2") != NULL)  {
				sprintf(ndsdata[totalchans].name,"%s",tmpname);
				ndsdata[totalchans].type = 2;
				totalchans ++;
			}
			lft = 0;
		}
	}
	printf("Total chans = %d\n",totalchans);
	for(ii = 0;ii<totalchans;ii++) 
		printf("%s\t%d\n",ndsdata[ii].name,ndsdata[ii].type);


	// Create DAQ message area in local memory
	daq_multi_dcu_data_t mxDataBlock;
	// Declare pointer to local memory message area
	char *daqbuffer = (char *)&mxDataBlock;
	char ndsbuffer[1000];


	/* set up defaults */
	sysname = NULL;
	int myErrorSignal = 0;
	int size;
	char *dataPtr;
	int jj;

	// Declare 0MQ message pointers
	void *daq_context;
	void *daq_subscriber;
	void *nds_context;
	void *nds_publisher;
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
	printf("Rcv data on %s\n",loc);

	nds_context = zmq_ctx_new();
	nds_publisher = zmq_socket (nds_context,ZMQ_PUB);
	sprintf(loc,"%s","tcp://*:6666");
	rc = zmq_bind (nds_publisher,loc);
	assert (rc == 0);
	printf("send data on %s\n",loc);

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
				for(jj=0;jj<10;jj++) {
					ndsdata[jj].value = *cpu_meter;
					cpu_meter ++;
				}
			}
			// Print the CPU METER info on each 1 second mark
			if(mxDataBlock.zmqheader[0].cycle == 0) {
				ndsptr = &ndsdata[0];
				int xsize = sizeof(channel_t);
				for(jj=0;jj<totalchans;jj++) {
					#if 0
					sprintf(ndsbuffer,"%s %d %d",
						ndsdata[jj].name,
						ndsdata[jj].type,
						ndsdata[jj].value);
					#endif
					memcpy(ndsbuffer,(void *)ndsptr,xsize);
					zmq_send(nds_publisher,ndsbuffer,xsize,0);
					ndsptr ++;
				}
			}
		}
		// *******************************************************************

	}while(!myErrorSignal);

	zmq_close(daq_subscriber);
	zmq_ctx_destroy(daq_context);
	zmq_close(nds_publisher);
	zmq_ctx_destroy(nds_context);
  
	exit(0);
}

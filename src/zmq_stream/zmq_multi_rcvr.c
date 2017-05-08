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


unsigned int nsys = 1; // The number of mapped shared memories (number of data sources)
static const int header_size = sizeof(daq_msg_header_t) + 4;

int
main(int argc, char **argv)
{


	char *sysname;
	char *sname[DCU_COUNT];
	int c;

	extern char *optarg;

	// Create DAQ message area in local memory
	daq_multi_dcu_data_t mxDataBlock;
	daq_multi_dcu_data_t mxDataBlocktest[4];
	// Declare pointer to local memory message area
	printf("size of mxdata = %d\n",sizeof(mxDataBlock));


	/* set up defaults */
	sysname = NULL;
	int myErrorSignal = 0;
	int size;
	int ii;
	int mydatardy = 0;

	// Declare 0MQ message pointers
	void *daq_context[DCU_COUNT];
	void *daq_subscriber[DCU_COUNT];
	int rc;
	zmq_msg_t message;
	zmq_pollitem_t daq_items[DCU_COUNT];
	char loc[20];

	// Test pointer to cpu meter data
	int *cpu_meter;
	int mycpu[4][6];
	int myconnects = 0;

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
		// Make 0MQ socket connection
		daq_context[ii] = zmq_ctx_new();
		daq_subscriber[ii] = zmq_socket (daq_context[ii],ZMQ_SUB);
		sprintf(loc,"%s%s%s%d","tcp://",sname[ii],":",DAQ_DATA_PORT);
		printf("sys %d = %s\n",ii,loc);
		rc = zmq_connect (daq_subscriber[ii], loc);
		assert (rc == 0);
		// Subscribe to all data from the server
		rc = zmq_setsockopt(daq_subscriber[ii],ZMQ_SUBSCRIBE,"",0);
		assert (rc == 0);
		daq_items[ii].socket = daq_subscriber[ii];
		daq_items[ii].fd = 0;
		daq_items[ii].events = ZMQ_POLLIN;
		daq_items[ii].revents = 0;
	}


	// Receive DAQ data in an infinite loop ***********************************

		zmq_msg_init(&message);
	do {
		// Initialize 0MQ message buffer
		// zmq_msg_init(&message);
		for(ii=0;ii<nsys;ii++) {
		zmq_poll(daq_items,nsys,-1);
			if(daq_items[ii].revents & ZMQ_POLLIN) {
				// Get data when message size > 0
				size = zmq_msg_recv(&message,daq_subscriber[ii],0);
				assert(size >= 0);
				// Get pointer to message data
				char *string = (char *)zmq_msg_data(&message);
				char *daqbuffer = (char *)&mxDataBlocktest[ii];
				// Copy data out of 0MQ message buffer to local memory buffer
				memcpy(daqbuffer,string,size);
				// Destroy the received message buffer
				zmq_msg_close(&message);
				if (ii==0) myconnects |= 1;
				if (ii==1) myconnects |= 2;
				zmq_msg_init(&message);
			}
		}
		// *******************************************************************
		// Following is test finding cpu meter data
		// Set data pointer to start of received data block
		if(do_verbose) {
		int mytotaldcu = 0;
		int mytotaldata = 0;
		for(int jj=0;jj<3;jj++) {
			char *dataPtr = (char *)&mxDataBlocktest[jj].zmqDataBlock[0];;
			mytotaldcu += mxDataBlocktest[jj].dcuTotalModels;
			for(ii=0;ii<mxDataBlocktest[jj].dcuTotalModels;ii++) {
				// Increment data pointer to start of next FE data block
				if(ii>0) dataPtr += mxDataBlocktest[jj].zmqheader[ii-1].dataBlockSize;
				mytotaldata += mxDataBlocktest[jj].zmqheader[ii].dataBlockSize;
				// Extract the cpu meter data for each FE
				cpu_meter = (int *)dataPtr;
				cpu_meter += 2;
				mycpu[jj][ii] = *cpu_meter;
			}
			if(mxDataBlocktest[jj].zmqheader[0].cycle == 0) mydatardy |= (1 << jj);
		}
			// Print the CPU METER info on each 1 second mark
		// if(mxDataBlocktest[0].zmqheader[0].cycle == 0) {
		if(mydatardy == 7) {
				printf("Total DCU = %d %d totalsize = %d\n", mytotaldcu,myconnects,mytotaldata);
		for(int jj=0;jj<3;jj++) {
				for(ii=0;ii<mxDataBlocktest[jj].dcuTotalModels;ii++) {
					printf("DCU = %d\tCPU METER = \t%d",mxDataBlocktest[jj].zmqheader[ii].dcuId,mycpu[jj][ii]);
					printf("\tTime = %d %d\tSize = \t%d bytes\n",
						mxDataBlocktest[jj].zmqheader[ii].timeSec,
						mxDataBlocktest[jj].zmqheader[ii].timeNSec,
						mxDataBlocktest[jj].zmqheader[ii].dataBlockSize);
				}
		}
			}
			// if(mxDataBlocktest[0].zmqheader[0].cycle == 0) {
		if(mydatardy == 7) {
			mydatardy = 0;
				printf("\n*******************************************************\n");
				}
		}
		// *******************************************************************
		myErrorSignal ++;

	}while(myErrorSignal < 640);

	for(ii=0;ii<nsys;ii++) {
		zmq_close(daq_subscriber[ii]);
		zmq_ctx_destroy(daq_context[ii]);
	}
  
	exit(0);
}

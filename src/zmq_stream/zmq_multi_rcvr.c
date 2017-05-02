//
///	@file mx_stream.c
///	@brief  Open-MX data sender, supports sending data
///< from the IOP as well as from the slaves
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
volatile int threads_running;
unsigned int do_wait = 0; // Wait for this number of milliseconds before starting a cycle
unsigned int wait_delay = 4; // Wait before acknowledging sends with mx_wait() for this number of cycles times nsys

extern void *findSharedMemory(char *);

void
usage()
{
	fprintf(stderr, "Usage: mx_stream [args] -s sys_names -d rem_host:0\n");
	fprintf(stderr, "-l filename - log file name\n"); 
	fprintf(stderr, "-s - server names \"x1x12 x1lsc x1asc\"\n");
	fprintf(stderr, "-v - verbose\n");
	fprintf(stderr, "-w - wait a given number of milliseconds before starting a cycle\n");
	fprintf(stderr, "-h - help\n");
}

typedef struct blockProp {
  unsigned int status;
  unsigned int timeSec;
  unsigned int timeNSec;
  unsigned int run;
  unsigned int cycle;
  unsigned int crc; /* block data CRC checksum */
} blockPropT;

struct rmIpcStr {       /* IPC area structure                   */
  unsigned int cycle;  /* Copy of latest cycle num from blocks */
  unsigned int dcuId;          /* id of unit, unique within each site  */
  unsigned int crc;            /* Configuration file's checksum        */
  unsigned int command;        /* Allows DAQSC to command unit.        */
  unsigned int cmdAck;         /* Allows unit to acknowledge DAQS cmd. */
  unsigned int request;        /* DCU request of controller            */
  unsigned int reqAck;         /* controller acknowledge of DCU req.   */
  unsigned int status;         /* Status is set by the controller.     */
  unsigned int channelCount;   /* Number of data channels in a DCU     */
  unsigned int dataBlockSize; /* Num bytes actually written by DCU within a 1/16 data block */
  blockPropT bp [DAQ_NUM_DATA_BLOCKS];  /* An array of block property structures */
};

/* GDS test point table structure for FE to frame builder communication */
typedef struct cdsDaqNetGdsTpNum {
   int count; /* test points count */
   int tpNum[DAQ_GDS_MAX_TP_NUM];
} cdsDaqNetGdsTpNum;


struct daq0mqdata {
	daq_msg_header_t zmqheader;
	char zmqDataBlock[DAQ_DCU_BLOCK_SIZE];
};

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



	do {
		// Initialize 0MQ message buffer
		zmq_msg_init(&message);
		// Get data when message size > 0
		size = zmq_msg_recv(&message,daq_subscriber,0);
		assert(size >= 0);
		char *string = (char *)zmq_msg_data(&message);
		// Copy data out of message buffer to local memory buffer
		memcpy(daqbuffer,string,size);
		// Destroy the received message buffer
		zmq_msg_close(&message);
		// Following is test finding cpu meter data
		// Set data pointer to start of received data block
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
			printf("Total DCU = %d\n", mxDataBlock.dcuTotalModels);
			printf("CPU = \t%d\t%d\t%d\t%d \n",mycpu[0],mycpu[1],mycpu[2],mycpu[3]);
		}

	}while(!myErrorSignal);

	zmq_close(daq_subscriber);
	zmq_ctx_destroy(daq_context);
  
	exit(0);
}

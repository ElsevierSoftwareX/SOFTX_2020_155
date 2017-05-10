//
///	@file zmq_multi_stream.c
///	@brief  ZeroMQ DAQ data sender, supports sending data from
///		multiple models on the same FE computer using ZeroMQ.
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
unsigned int wait_delay = 4; // Wait before acknowledging sends with mx_wait() for this number of cycles times nsys

extern void *findSharedMemory(char *);

void
usage()
{
	fprintf(stderr, "Usage: zmq_multi_stream [args] -s sys_names -d rem_host:0\n");
	fprintf(stderr, "-l filename - log file name\n"); 
	fprintf(stderr, "-s - system names: \"x1x12 x1lsc x1asc\"\n");
	fprintf(stderr, "-v - verbose\n");
	fprintf(stderr, "-w - wait a given number of milliseconds before starting a cycle\n");
	fprintf(stderr, "-h - help\n");
}

static struct rmIpcStr *shmIpcPtr[DAQ_ZMQ_MAX_DCU ];
static char *shmDataPtr[DAQ_ZMQ_MAX_DCU ];
static struct cdsDaqNetGdsTpNum *shmTpTable[DAQ_ZMQ_MAX_DCU ];
static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;
// static const int header_size = sizeof(struct rmIpcStr) + sizeof(struct cdsDaqNetGdsTpNum);
static const int header_size = DAQ_ZMQ_HEADER_SIZE;

int
main(int argc, char **argv)
{


	char *sysname;
	int c;

	extern char *optarg;

	// Declare local DAQ data buffer
	daq_multi_dcu_data_t zmqDataBlock;
	// Set pointer to local DAQ data buffer
	char *daqbuffer = (char *) &zmqDataBlock;


	/* set up defaults */
	sysname = NULL;
	int nsys = 1;				// Default number of models to connect to
	char *sname[DAQ_ZMQ_MAX_DCU];	// Model names

	// 0MQ connection vars
	void *daq_context;
	void *daq_publisher;
	int rc;

	int new_cycle;
	int myErrorSignal = 0;
	int sendLength = 0;
	int daqStatBit[2];
	daqStatBit[0] = 1;
	daqStatBit[1] = 2;
	char *dataBuff;
	unsigned int myCrc = 0;
	char buffer[1024000];
	int msg_size = 0;
	char *zbuffer;
	int ii;


	while ((c = getopt(argc, argv, "hd:e::b:s:Vvw:x")) != EOF) switch(c) {
	case 's':
		sysname = optarg;
		printf ("sysnames = %s\n",sysname);
		break;
	case 'W':
		wait_delay = atoi(optarg);
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

	printf("System names: %s\n", sysname);
	sname[0] = strtok(sysname, " ");
        for(;;) {
                printf("%s\n", sname[nsys - 1]);
		char *s = strtok(0, " ");
		if (!s) break;
	        sname[nsys] = s;
	        nsys++;
	}
	// Map shared memory for all systems
	for (unsigned int i = 0; i < nsys; i++) {
                char shmem_fname[128];
                sprintf(shmem_fname, "%s_daq", sname[i]);
                void *dcu_addr = findSharedMemory(shmem_fname);
                if (dcu_addr <= 0) {
                        fprintf(stderr, "Can't map shmem\n");
                        exit(1);
                } else {
                        printf("mapped at 0x%lx\n",(unsigned long)dcu_addr);
	        }
                shmIpcPtr[i] = (struct rmIpcStr *)((char *)dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
                shmDataPtr[i] = (char *)((char *)dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
                shmTpTable[i] = (struct cdsDaqNetGdsTpNum *)((char *)dcu_addr + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
        }

	printf("Total number of systems = %d\n", nsys);




	// Set up the data publisher socket
	daq_context = zmq_ctx_new();
	daq_publisher = zmq_socket (daq_context,ZMQ_PUB);
	char loc[20];
	// sprintf(loc,"%s%d","tcp://*:",DAQ_DATA_PORT);
	sprintf(loc,"%s%d","tcp://eth1:",DAQ_DATA_PORT);
	rc = zmq_bind (daq_publisher, loc);
	assert (rc == 0);
	printf("sending data on %s\n",loc);


	// Find cycle zero
	for (;shmIpcPtr[0]->cycle;) usleep(1000);
	int lastCycle = 0;
	printf("Found cycle zero\n");



	// Enter Infinit Loop
	do {
		// Wait for a new 1/16Hz DAQ data cycle
		do{
			usleep(1000);
			new_cycle = shmIpcPtr[0]->cycle;
		}while (new_cycle == lastCycle);

		// Print diags in verbose mode
		if(new_cycle == 0 && do_verbose) {
			printf("\nTime = %d-%d with size = %d\n",shmIpcPtr[0]->bp[lastCycle].timeSec,shmIpcPtr[0]->bp[lastCycle].timeNSec,msg_size);
			printf("\tCycle = ");
			for(ii=0;ii<nsys;ii++) printf("\t%d",zmqDataBlock.zmqheader[ii].cycle);
			printf("\n\tTimeSec = ");
			for(ii=0;ii<nsys;ii++) printf("\t%d",zmqDataBlock.zmqheader[ii].timeSec);
			printf("\n\tTimeNSec = ");
			for(ii=0;ii<nsys;ii++) printf("\t%d",zmqDataBlock.zmqheader[ii].timeNSec);
			printf("\n\tDataSize = ");
			for(ii=0;ii<nsys;ii++) printf("\t%d",zmqDataBlock.zmqheader[ii].dataBlockSize);
			}

		// Increment the local DAQ cycle counter
		lastCycle ++;
		lastCycle %= 16;

		
		// Set pointer to 0MQ message data block
		zbuffer = (char *)&zmqDataBlock.zmqDataBlock[0];
		// Initialize data send length to size of message header
		sendLength = header_size;
		// Set number of FE models that have data in this message
		zmqDataBlock.dcuTotalModels = nsys;
		// Loop thru all FE models
		for (ii=0;ii<nsys;ii++) {
			// Set heartbeat monitor for return to DAQ software
			if (lastCycle == 0) shmIpcPtr[ii]->status ^= daqStatBit[0];
			// Set DCU ID in header
			zmqDataBlock.zmqheader[ii].dcuId = shmIpcPtr[ii]->dcuId;
			// Set DAQ .ini file CRC checksum
			zmqDataBlock.zmqheader[ii].fileCrc = shmIpcPtr[ii]->crc;
			// Set Status -- Need to update for models not running
			zmqDataBlock.zmqheader[ii].status = 0;
			// Set 1/16Hz cycle number
			zmqDataBlock.zmqheader[ii].cycle = shmIpcPtr[ii]->cycle;
			// Set GPS seconds
			zmqDataBlock.zmqheader[ii].timeSec = shmIpcPtr[ii]->bp[lastCycle].timeSec;
			// Set GPS nanoseconds
			zmqDataBlock.zmqheader[ii].timeNSec = shmIpcPtr[ii]->bp[lastCycle].timeNSec;
			// Indicate size of data block
			zmqDataBlock.zmqheader[ii].dataBlockSize = shmIpcPtr[ii]->dataBlockSize;
			// Prevent going beyond MAX allowed data size
			if (zmqDataBlock.zmqheader[ii].dataBlockSize > DAQ_DCU_BLOCK_SIZE)
				zmqDataBlock.zmqheader[ii].dataBlockSize = DAQ_DCU_BLOCK_SIZE;

			// Set pointer to dcu data in shared memory
			dataBuff = (char *)(shmDataPtr[ii] + lastCycle * buf_size);
			// Copy data from shared memory into local buffer
			memcpy((void *)zbuffer,dataBuff,zmqDataBlock.zmqheader[ii].dataBlockSize);
			// Increment the 0mq data buffer pointer for next FE
			zbuffer += zmqDataBlock.zmqheader[ii].dataBlockSize;
			// Increment the 0mq message size with size of FE data block
			sendLength += zmqDataBlock.zmqheader[ii].dataBlockSize;

			// Calculate CRC on the data and add to header info
			myCrc = crc_ptr((char *)&zmqDataBlock.zmqDataBlock[0],shmIpcPtr[ii]->bp[lastCycle].crc,0); // .crc is crcLength
			myCrc = crc_len(shmIpcPtr[ii]->bp[lastCycle].crc,myCrc);
			zmqDataBlock.zmqheader[ii].dataCrc = myCrc;

			// Update heartbeat monitor to DAQ code
			if (lastCycle == 0) shmIpcPtr[ii]->status ^= daqStatBit[1];
		}
		// Copy data to 0mq message buffer
		memcpy(buffer,daqbuffer,sendLength);
		usleep((do_wait * 500));
		// Send Data
		msg_size = zmq_send(daq_publisher,buffer,sendLength,0);


	}while(!myErrorSignal);

	zmq_close(daq_publisher);
	zmq_ctx_destroy(daq_context);
  
	exit(0);
}

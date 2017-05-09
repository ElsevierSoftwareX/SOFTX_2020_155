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
void *daq_context[DCU_COUNT];
void *daq_subscriber[DCU_COUNT];
daq_multi_dcu_data_t mxDataBlockFull[16];
unsigned int tstatus[16];
int stop_working_threads = 0;
int start_acq = 0;

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

void *rcvr_thread(void *arg) {
	int *mythread = (int *)arg;
	int mt = *mythread;
	printf("myarg = %d\n",mt);
	zmq_msg_t message;
	daq_multi_dcu_data_t mxDataBlock;
	int ii;
	int cycle = 0;
	int acquire = 0;
	unsigned int myts = 0;

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
			int tdd = mxDataBlock.zmqheader[ii].dcuId;
			cycle = mxDataBlock.zmqheader[ii].cycle;
			myts = mxDataBlock.zmqheader[ii].timeSec;
			mxDataBlockFull[cycle].zmqheader[tdd].dcuId = tdd;
			mxDataBlockFull[cycle].zmqheader[tdd].fileCrc = mxDataBlock.zmqheader[ii].fileCrc;
			mxDataBlockFull[cycle].zmqheader[tdd].status = mxDataBlock.zmqheader[ii].status;
			mxDataBlockFull[cycle].zmqheader[tdd].cycle = mxDataBlock.zmqheader[ii].cycle;
			mxDataBlockFull[cycle].zmqheader[tdd].timeSec = mxDataBlock.zmqheader[ii].timeSec;
			mxDataBlockFull[cycle].zmqheader[tdd].timeNSec = mxDataBlock.zmqheader[ii].timeNSec;
			mxDataBlockFull[cycle].zmqheader[tdd].dataCrc = mxDataBlock.zmqheader[ii].dataCrc;
			mxDataBlockFull[cycle].zmqheader[tdd].dataBlockSize = mxDataBlock.zmqheader[ii].dataBlockSize;
		}
		if(cycle == 0 && start_acq) acquire = 1;
		if(acquire)  {
			tstatus[cycle] |= (1 << mt);
			printf("c = %d\t%d\t%d\t%d\n",mt,myts,cycle,size);
		}
	} while(!stop_working_threads);
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
	int rc;
	zmq_msg_t message;
	zmq_pollitem_t daq_items[DCU_COUNT];
	char loc[30];

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
		int tnum = ii;
		pthread_create(&thread_id[ii],NULL,rcvr_thread,(void *)&tnum); 
		dataRdy |= (1 << ii);
	}


#if 0
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
#endif
	
	int mastercycle = 0;
	int loop = 0;
	start_acq = 1;
	int64_t mytime = 0;
	int64_t mylasttime = 0;
	int64_t myptime = 0;
	do {
		do {
			usleep(2000);
		}while(tstatus[loop] != dataRdy);
		tstatus[loop] = 0;
		mytime = s_clock();
		myptime = mytime - mylasttime;
		mylasttime = mytime;
		printf("Data rday for cycle = %d\t%ld\n",loop,myptime);
		loop ++;
		loop %= 16;
		myErrorSignal ++;
	}while (myErrorSignal < 132);

	stop_working_threads = 1;

	for(ii=0;ii<nsys;ii++) {
		pthread_join(thread_id[ii],NULL);
	}

	for(ii=0;ii<nsys;ii++) {
		zmq_close(daq_subscriber[ii]);
		zmq_ctx_destroy(daq_context[ii]);
	}
  
	exit(0);
}

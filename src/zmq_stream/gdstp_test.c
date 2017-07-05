//
///	@file gdstp_test.c
///	@brief  Test routine to verify gdstp data by channel from daqLib.c
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
#include <assert.h>
#include "zmq_daq.h"
#include "../include/daqmap.h"


#define DAQ_RDY_MAX_WAIT	80

int do_verbose;
unsigned int do_wait = 1; // Wait for this number of milliseconds before starting a cycle
unsigned int wait_delay = 1; // Wait before acknowledging sends with mx_wait() for this number of cycles times nsys

extern void *findSharedMemory(char *);
static volatile int keepRunning = 1;

static GDS_STATUS *gdsStatus[10];
int gdsValsPerChan[10];
static char *gdsDataAreaPtr[10];	// Pointer to beginning of GDS data area
char *gdsHdrBlockPtr[10];		// Pointer to Start of 16Hz GDS Hdr bloack
char *gdsDataBlockPtr[10];		// Pointer to Start of 16Hz GDS data bloack
GDS_DATA_HEADER *gdsHdrPtr[10];
float *gdsdata[10];



char modelnames[DAQ_ZMQ_MAX_DCU][64];


void intHandler(int dummy) {
        keepRunning = 0;
}

void
usage()
{
	fprintf(stderr, "Usage: gdstp_test [args] -s sys_names \n");
	fprintf(stderr, "-s - system names: \"x1x12 x1lsc x1asc\"\n");
	fprintf(stderr, "-l filename - log file name\n"); 
	fprintf(stderr, "-v - verbose\n");
	fprintf(stderr, "-h - help\n");
}

int sync2zero(GDS_STATUS *ipcPtr) {
	int lastCycle = 0;

	// Find cycle zero
	for (;ipcPtr->cycle;) usleep(1000);
	return(lastCycle);
}


int
main(int argc, char **argv)
{


	char *sysname;
	int c;
	int dcuId[10];
	char channel_name[64][64];

	extern char *optarg;

	/* set up defaults */
	sysname = NULL;
	int nsys = 1;				// Default number of models to connect to


	int new_cycle;
	int daqStatBit[2];
	daqStatBit[0] = 1;
	daqStatBit[1] = 2;
	int ii,jj,kk;
	int timeout = 0;


	while ((c = getopt(argc, argv, "hd:e:s:Vvw:x")) != EOF) switch(c) {
	case 's':
		sysname = optarg;
		printf ("sysnames = %s\n",sysname);
		break;
	case 'e':
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


	if(sysname != NULL) {
		printf("System names: %s\n", sysname);
	        sprintf(modelnames[0],"%s",strtok(sysname, " "));
		dcuId[0] = 0;
	        for(;;) {
	                char *s = strtok(0, " ");
	                if (!s) break;
	        	sprintf(modelnames[nsys],"%s",s);
			dcuId[nsys] = 0;
	                nsys++;
	        }
	} else {
		usage(); 
		exit(-1); 
	}
	   printf ("** Total number of systems is %d\n",nsys);

	// Setup to catch control C
	signal(SIGINT,intHandler);

	// Map shared memory for all systems
	for (unsigned int i = 0; i < nsys; i++) {
                char shmem_fname[128];
                sprintf(shmem_fname, "%s_gds", modelnames[i]);
                void *dcu_addr = findSharedMemory(shmem_fname);
                if (dcu_addr <= 0) {
                        fprintf(stderr, "Can't map shmem\n");
                        exit(1);
                } 
		gdsStatus[i] = (GDS_STATUS *) (dcu_addr + GDS_DATA_OFFSET - sizeof(GDS_STATUS));
		gdsValsPerChan[i] = gdsStatus[i]->valsperchan;
		gdsDataAreaPtr[i] = (char *)(dcu_addr + GDS_DATA_OFFSET);;
		gdsHdrBlockPtr[i] = (char *) gdsDataAreaPtr[i];
		gdsDataBlockPtr[i] = (char *) (gdsDataAreaPtr[i] + sizeof(GDS_DATA_HEADER));
		gdsHdrPtr[i] = (GDS_DATA_HEADER *) gdsDataAreaPtr[i];
		printf("Found GDS area for %s at 0x%lx \n",shmem_fname,(unsigned long)gdsStatus[i]);
        }


	// Find cycle zero
	// int lastCycle = sync2zero(shmIpcPtr[0]);
	// for (;shmIpcPtr[0]->cycle;) usleep(1000);
	int lastCycle = 0;

	int sync2iop = 1;

	// Enter Infinit Loop
	do {
		if(sync2iop) {
			printf("Syncing to IOP\n");
			lastCycle = sync2zero(gdsStatus[0]);
			sync2iop = 0;
			printf("Found cycle zero\n");
		}
		timeout = 0;
		// Wait for a new 1/16Hz DAQ data cycle
		do{
			usleep(1000);
			new_cycle = gdsStatus[0]->cycle;
			timeout += 1;
		}while (new_cycle == lastCycle && timeout < DAQ_RDY_MAX_WAIT);
		if(timeout >= DAQ_RDY_MAX_WAIT) {
			sync2iop = 1;
			lastCycle = sync2zero(gdsStatus[0]);
			printf("Iop model not running\n");
		}
		if(sync2iop) continue;
		// IOP will be first model ready
		// Need to wait for 2K models to reach end of their cycled
		usleep((do_wait * 1000));


		for(ii=0;ii<nsys;ii++) {
			gdsHdrBlockPtr[ii] = (char *) (gdsDataAreaPtr[ii] + (GDS_BUFF_SIZE * lastCycle));;
			gdsHdrPtr[ii] = (GDS_DATA_HEADER *) (gdsHdrBlockPtr[ii]) ;
			gdsDataBlockPtr[ii] = (char *) (gdsHdrBlockPtr[ii] + sizeof(GDS_DATA_HEADER));
			if(gdsHdrPtr[ii]->chan_count > 0) {
				printf("%s - New cycle found %d with %d GDS chans\n",modelnames[ii],lastCycle,gdsHdrPtr[ii]->chan_count);
				for(jj=0;jj<gdsHdrPtr[ii]->chan_count;jj++) {
					gdsdata[ii] = (float *)gdsDataBlockPtr[ii] + (jj * gdsValsPerChan[ii]);
					sprintf(channel_name,"%s",gdsHdrPtr[ii]->chan_name[jj]);
					printf("channel name = %s\n",channel_name);
					for(kk=0;kk<5;kk++) {
						printf("\tdata %d = %f\n",kk,*gdsdata[ii]);
						gdsdata[ii] ++;
					}
				}
			}
		}


		// Increment the local DAQ cycle counter
		lastCycle ++;
		lastCycle %= 16;

	}while(keepRunning);

	printf("Exiting test code\n");
  
	exit(0);
}

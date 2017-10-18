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


#define DAQ_RDY_MAX_WAIT	80

int do_verbose;
unsigned int do_wait = 1; // Wait for this number of milliseconds before starting a cycle
unsigned int wait_delay = 1; // Wait before acknowledging sends with mx_wait() for this number of cycles times nsys

extern void *findSharedMemory(char *);
static volatile int keepRunning = 1;

static struct rmIpcStr *shmIpcPtr[DAQ_ZMQ_MAX_DCU ];
static char *shmDataPtr[DAQ_ZMQ_MAX_DCU ];
static struct cdsDaqNetGdsTpNum *shmTpTable[DAQ_ZMQ_MAX_DCU ];
static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;
// static const int header_size = sizeof(struct rmIpcStr) + sizeof(struct cdsDaqNetGdsTpNum);
static const int header_size = DAQ_ZMQ_HEADER_SIZE;
char modelnames[DAQ_ZMQ_MAX_DCU][64];


void intHandler(int dummy) {
        keepRunning = 0;
}

void
usage()
{
	fprintf(stderr, "Usage: zmq_multi_stream [args] -s sys_names -e ethernet name\n");
	fprintf(stderr, "-e name of ethernet card to use (REQUIRED) \n"); 
	fprintf(stderr, "-s - system names: \"x1x12 x1lsc x1asc\"\n");
	fprintf(stderr, "If -s not specified, code with use hostname and testpoint.par files to auto start\n"); 
	fprintf(stderr, "-l filename - log file name\n"); 
	fprintf(stderr, "-v - verbose\n");
	fprintf(stderr, "-w - wait a given number of milliseconds before starting a cycle\n");
	fprintf(stderr, "-W - number of milliseconds before starting a cycle\n");
	fprintf(stderr, "-h - help\n");
}

// Auto determine control models for this FE computer
// Get Hostname
// Read testpoint.par file for computer and model info
int getmodelnames( int dcuid[]) {
	FILE *fr;
	int modelcount = 0;
	char myname[64];


	char str[64];
	sprintf(str,"%s","[X-node25]");

	char tmp[256];
	char line[80];
	int i,j,ii;
	int mydcuid = 0;
	char gdsfile[128];
	char *token;
	char *search = "=";
	int inmatch = 0;

	// Get computer name
	gethostname(myname,sizeof(myname));
	printf("My computer name is %s\n",myname);


	   /// Need to get IFO and SITE info from environment variables.
	   char *s = getenv("IFO");
	   for(ii=0;s[ii] != '\0';ii++) {
	           if(isupper(s[ii])) s[ii] = tolower(s[ii]);
	   }
	   char *s1 = getenv("SITE");
	   for(ii=0;s1[ii] != '\0';ii++) {
	           if(isupper(s1[ii])) s1[ii] = tolower(s1[ii]);
	   }

	// Create testpoint.par file name
	sprintf(gdsfile,"%s%s%s%s%s","/opt/rtcds/",s1,"/",s,"/target/gds/param/testpoint.par");
	// Open and read testpoint.par file
	fr = fopen(gdsfile,"r");
	if(fr == NULL) return (-1);
	while(fgets(line,80,fr) != NULL) {
	   	line[strcspn(line, "\n")] = 0;
		// If line contains "node", then this line contains DCU ID number
	   	if(strstr(line,"node") != NULL) {
			for(i=0;line[i];i++)
			{
			  j=0;
			  // Find the numbers in the line string
			   while(line[i]>='0' && line[i]<='9')
			   {
				tmp[j]=line[i];
				i++;
				j++;
			   }
			   tmp[j]=0;
			   // calc the dcuid
			   mydcuid = strtol(tmp, &tmp, 10);
			} 
		}
		// If line contains hostname, then get the computer name
	   	if(strstr(line,"hostname") != NULL) {
			token = strtok(line, search);
			token = strtok(NULL, search);

			// If computer name matches hostname, then set to capture data
			if(strcmp(myname,token) == 0) {
				inmatch = 1;
			}
		}
		// If line contains system, then this line contains the model name
		// If computer and hostname matched above, then use this model
	   	if(strstr(line,"system") != NULL && inmatch) {
			token = strtok(line, search);
			token = strtok(NULL, search);
			sprintf(modelnames[modelcount],"%s",token);;
			dcuid[modelcount] = mydcuid;
			modelcount ++;
			inmatch = 0;
		}
	   }
	   return(modelcount);
}

int sync2zero(struct rmIpcStr *ipcPtr) {
	int lastCycle = 0;

	// Find cycle zero
	for (;ipcPtr->cycle;) usleep(1000);
	return(lastCycle);
}


int
main(int argc, char **argv)
{


	char *sysname;
	char *eport;
	int c;
	int dcuId[10];

	extern char *optarg;

	// Declare local DAQ data buffer
	daq_multi_dcu_data_t zmqDataBlock;
	// Set pointer to local DAQ data buffer
	char *daqbuffer = (char *) &zmqDataBlock;


	/* set up defaults */
	sysname = NULL;
	eport = NULL;
	int nsys = 1;				// Default number of models to connect to

	// 0MQ connection vars
	void *daq_context;
	void *daq_publisher;
	int rc;

	int new_cycle;
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
	unsigned int reftimeSec = 0;
	unsigned int reftimeNSec = 0;
	int reftimeerror = 0;
	int refcycle = 0;
	int timeout = 0;


	while ((c = getopt(argc, argv, "hd:e:s:Vvw:x")) != EOF) switch(c) {
	case 's':
		sysname = optarg;
		printf ("sysnames = %s\n",sysname);
		break;
	case 'e':
		eport = optarg;
		printf ("eport = %s\n",eport);
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

	if (eport == NULL) { usage(); exit(1); }

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
		nsys = getmodelnames(dcuId); 
	}
	   printf ("** Total number of systems is %d\n",nsys);
	   for(ii=0;ii<nsys;ii++) {
	    	printf ("\t%s\t%d\n",modelnames[ii],dcuId[ii]);
		// Set data xmit header
		zmqDataBlock.zmqheader[ii].dcuId = dcuId[ii];;
		zmqDataBlock.zmqheader[ii].fileCrc = 0;
		zmqDataBlock.zmqheader[ii].status = 0xbad;
		zmqDataBlock.zmqheader[ii].cycle = 0;
		zmqDataBlock.zmqheader[ii].timeSec = 0;
		zmqDataBlock.zmqheader[ii].timeNSec = 0;
		zmqDataBlock.zmqheader[ii].dataCrc = 0;
		zmqDataBlock.zmqheader[ii].dataBlockSize = 0;
	   }

	// Setup to catch control C
	signal(SIGINT,intHandler);

	// Map shared memory for all systems
	for (unsigned int i = 0; i < nsys; i++) {
                char shmem_fname[128];
                sprintf(shmem_fname, "%s_daq", modelnames[i]);
                void *dcu_addr = findSharedMemory(shmem_fname);
                if (dcu_addr <= 0) {
                        fprintf(stderr, "Can't map shmem\n");
                        exit(1);
                } 
                shmIpcPtr[i] = (struct rmIpcStr *)((char *)dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
                shmDataPtr[i] = (char *)((char *)dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
                shmTpTable[i] = (struct cdsDaqNetGdsTpNum *)((char *)dcu_addr + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
        }

	// Set up the data publisher socket
	daq_context = zmq_ctx_new();
	daq_publisher = zmq_socket (daq_context,ZMQ_PUB);
	char loc[20];
	// sprintf(loc,"%s%d","tcp://*:",DAQ_DATA_PORT);
	sprintf(loc,"%s%s%s%d","tcp://",eport,":",DAQ_DATA_PORT);
	rc = zmq_bind (daq_publisher, loc);
	assert (rc == 0);
	printf("sending data on %s\n",loc);


	// Find cycle zero
	// int lastCycle = sync2zero(shmIpcPtr[0]);
	// for (;shmIpcPtr[0]->cycle;) usleep(1000);
	int lastCycle = 0;

	int sync2iop = 1;

	// Enter Infinit Loop
	do {
		if(sync2iop) {
			printf("Syncing to IOP\n");
			lastCycle = sync2zero(shmIpcPtr[0]);
			sync2iop = 0;
			printf("Found cycle zero\n");
		}
		timeout = 0;
		// Wait for a new 1/16Hz DAQ data cycle
		do{
			usleep(1000);
			new_cycle = shmIpcPtr[0]->cycle;
			timeout += 1;
		}while (new_cycle == lastCycle && timeout < DAQ_RDY_MAX_WAIT);
		if(timeout >= DAQ_RDY_MAX_WAIT) {
			sync2iop = 1;
			lastCycle = sync2zero(shmIpcPtr[0]);
			printf("Iop model not running\n");
		}
		if(sync2iop) continue;
		// IOP will be first model ready
		// Need to wait for 2K models to reach end of their cycled
		usleep((do_wait * 1000));

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
			reftimeerror = 0;
			// Set heartbeat monitor for return to DAQ software
			if (lastCycle == 0) shmIpcPtr[ii]->reqAck ^= daqStatBit[0];
			// Set DCU ID in header
			zmqDataBlock.zmqheader[ii].dcuId = shmIpcPtr[ii]->dcuId;
			// Set DAQ .ini file CRC checksum
			zmqDataBlock.zmqheader[ii].fileCrc = shmIpcPtr[ii]->crc;
			// Set 1/16Hz cycle number
			zmqDataBlock.zmqheader[ii].cycle = shmIpcPtr[ii]->cycle;
			if(ii == 0) refcycle = shmIpcPtr[ii]->cycle;
			// Set GPS seconds
			zmqDataBlock.zmqheader[ii].timeSec = shmIpcPtr[ii]->bp[lastCycle].timeSec;
			if (ii == 0) reftimeSec = shmIpcPtr[ii]->bp[lastCycle].timeSec;
			// Set GPS nanoseconds
			zmqDataBlock.zmqheader[ii].timeNSec = shmIpcPtr[ii]->bp[lastCycle].timeNSec;
			if (ii == 0) reftimeNSec = shmIpcPtr[ii]->bp[lastCycle].timeNSec;
			if (ii != 0 && reftimeSec != shmIpcPtr[ii]->bp[lastCycle].timeSec) 
				reftimeerror = 1;;
			if (ii != 0 && reftimeNSec != shmIpcPtr[ii]->bp[lastCycle].timeNSec) 
				reftimeerror |= 2;;
			if(reftimeerror) {
				zmqDataBlock.zmqheader[ii].cycle = refcycle;
				// printf("Timing error model %d\n",ii);
				// Set Status -- Need to update for models not running
				zmqDataBlock.zmqheader[ii].status = 0xbad;
				// Indicate size of data block
				zmqDataBlock.zmqheader[ii].dataBlockSize = 0;
			} else {
				// Set Status -- Need to update for models not running
				zmqDataBlock.zmqheader[ii].status = 0;
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
			if (lastCycle == 0) shmIpcPtr[ii]->reqAck ^= daqStatBit[1];
			}
		}
		// Copy data to 0mq message buffer
		memcpy(buffer,daqbuffer,sendLength);
		// Send Data
		msg_size = zmq_send(daq_publisher,buffer,sendLength,0);
		// printf("Sending data size = %d\n",msg_size);


	}while(keepRunning);

	printf("Closing out ZMQ and exiting\n");
	zmq_close(daq_publisher);
	zmq_ctx_destroy(daq_context);
  
	exit(0);
}

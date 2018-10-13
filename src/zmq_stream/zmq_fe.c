//
///// @file fe_dc.c
///// @brief  Front End data concentrator
////
//
#define _GNU_SOURCE
#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "../drv/crc.c"
#include "../include/daqmap.h"
#include "../include/drv/fb.h"
// #include "../include/daq_core.h"
#include "../drv/gpstime/gpstime.h"
#include <zmq.h>
#include <assert.h>
#include "zmq_daq.h"
#include "zmq_transport.h"
#include "dc_utils.h"
#include "simple_pv.h"


#define __CDECL

static struct rmIpcStr *shmIpcPtr[128];
static char *shmDataPtr[128];
static struct cdsDaqNetGdsTpNum *shmTpTable[128];
static const int header_size = sizeof(struct daq_multi_dcu_header_t);
static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;
int modelrates[DAQ_TRANSIT_MAX_DCU];
int dcuid[DAQ_TRANSIT_MAX_DCU];
daq_multi_dcu_data_t *ixDataBlock;
daq_multi_cycle_header_t *ifo_header;
char *zbuffer;

extern void *findSharedMemory(char *);
extern void *findSharedMemorySize(char *,int);

char modelnames[DAQ_TRANSIT_MAX_DCU][64];
char *sysname;
int do_verbose = 0;
// int sendLength = 0;
static volatile int keepRunning = 1;
char *ifo;
char *ifo_data;
size_t cycle_data_size;

// ZMQ defines
//char msg_buffer[0x200000];
daq_multi_dcu_data_t msg_buffer;
void *daq_context;
void *daq_publisher;


int symmetricom_fd = -1;
unsigned long  window_start[16] = {63500,135000,197500,260000,322500,395000,447500,510000,572500,635000,697500,760000,822500,885000,947500,10000};
unsigned long  window_stop[16] =  {135000,197500,260000,322500,395000,447500,510000,572500,635000,697500,760000,822500,885000,947500,999990,63500};


/*********************************************************************************/
/*                                U S A G E                                      */
/*                                                                               */
/*********************************************************************************/

void Usage()
{
    printf("Usage of zmq_fe:\n");
    printf("zmq_fe  -s <models> <OPTIONS>\n");
    printf(" -b <buffer>    : Name of the mbuf to concentrate the data to locally (defaults to ifo)\n");
    printf(" -s <value>     : Name of FE control models\n");
    printf(" -m <value>     : Local memory buffer size in megabytes\n");
    printf(" -v 1           : Enable verbose output\n");
    printf(" -e <interface> : Name of the interface to broadcast data through\n");
    printf(" -d <directory> : Path to the gds tp dir used to lookup model rates\n");
    printf(" -D <value>     : Add a delay in ms to sending the data.  Used to spread the load\n");
    printf("                : when working with multiple sending systems.  Defaults to 0.");
    printf(" -h             : This helpscreen\n");
    printf("\n");
}

void zmq_make_connection(char *eport)
{
  char loc[200];
  int rc;
	
	// Set up the data publisher socket
	daq_context = zmq_ctx_new();
    daq_publisher = zmq_socket (daq_context,ZMQ_PUB);
    // sprintf(loc,"%s%d","tcp://*:",DAQ_DATA_PORT);
	//sprintf(loc,"%s%s%s%d","tcp://",eport,":",DAQ_DATA_PORT);
	if (!dc_generate_connection_string(loc, eport, sizeof(loc), 0)) {
		fprintf(stderr, "Unable to generate connection string for '%s'\n", eport);
		exit(1);
	}
    dc_set_zmq_options(daq_publisher);
	printf("Binding to '%s'\n", loc);
    rc = zmq_bind (daq_publisher, loc);
    if (rc < 0) {
    	fprintf(stderr, "Errno = %d: %s\n", errno, strerror(errno));

    }
    assert (rc == 0);
   	printf("sending data on %s\n",loc);
}

// **********************************************************************************************
/// Get current GPS time from the symmetricom IRIG-B card
unsigned long
symm_gps_time(unsigned long *frac, int *stt) {
	unsigned long t[3];

    ioctl (symmetricom_fd, IOCTL_SYMMETRICOM_TIME, &t);
    t[1] *= 1000;
    t[1] += t[2];
    if (frac) *frac = t[1];
    if (stt) *stt = 0;
    return  t[0];
}

// **********************************************************************************************
/// See if the GPS card is locked.
int
symm_ok() {
	unsigned long req = 0;
   	ioctl (symmetricom_fd, IOCTL_SYMMETRICOM_STATUS, &req);
    printf("Symmetricom status: %s\n", req? "LOCKED": "UNCLOCKED");
	return req;
}

// **********************************************************************************************
// This function uses time from IRIG-B card
// Abandoned due to interference with IOP model reading same card
int 
waitNextCycle(	unsigned int cyclereq,				// Cycle to wait for
				int reset,					// Request to reset model ipc shared memory
				struct rmIpcStr *ipcPtr) 	// Pointer to IOP IPC shared memory
{
	unsigned long gps_frac;
	int gps_stt;
	unsigned long gps_time;
	int iopRunning = 0;
	int runontimer = 0;

		// if reset, want to set IOP cycle to impossible number
		if(reset) ipcPtr->cycle = 50;
        // Find cycle 
		do {
     		usleep(1000);
			// Get GPS time from gpstime driver
			gps_time = symm_gps_time(&gps_frac, &gps_stt);
			// Convert nanoseconds to microseconds
			gps_frac /= 1000;
			// If requested cycle matches IOP shared memory cycle, 
			// then IOP is running and requested cycle is found
			if(ipcPtr->cycle == cyclereq) iopRunning = 1;
			// If GPS time is within window that requested cycle should be ready,
			// then return on timer event.
			if(gps_frac > window_start[cyclereq] && gps_frac < window_stop[cyclereq]) runontimer = 1;
		}while(iopRunning == 0 && runontimer == 0 && keepRunning);
		// Return iopRunning:
		// 	- One (1) if synced to IOP 
		// 	- Zero (0) if synced by GPS time ie IOP not running.
        return(iopRunning);
}
// **********************************************************************************************
int 
waitNextCycle2(	int nsys,
				unsigned int cyclereq,				// Cycle to wait for
				int reset,					// Request to reset model ipc shared memory
				int dataRdy[],
				struct rmIpcStr *ipcPtr[]) 	// Pointer to IOP IPC shared memory
{
	int iopRunning = 0;
	int ii;
	int threads_rdy = 0;
	int timeout = 0;

		// if reset, want to set IOP cycle to impossible number
		if(reset) ipcPtr[0]->cycle = 50;
        // Find cycle 
		// do {
     		usleep(1000);
			// Wait until received data from at least 1 FE or timeout
			do {
				usleep(2000);
				if(ipcPtr[0]->cycle == cyclereq) 
				{
						iopRunning = 1;
						dataRdy[0] = 1;
				}
			    timeout += 1;
			}while(!iopRunning && timeout < 500);

            // Wait until data received from everyone or timeout
            timeout = 0;
            do {
             	usleep(100);
				for(ii=1;ii<nsys;ii++) {
             		if(ipcPtr[ii]->cycle == cyclereq && !dataRdy[ii]) threads_rdy ++;
             		if(ipcPtr[ii]->cycle == cyclereq) dataRdy[ii] = 1;
                }
                timeout += 1;
            }while(threads_rdy < nsys && timeout < 20);

		// }while(iopRunning == 0 && keepRunning);
		// Return iopRunning:
		// 	- One (1) if synced to IOP 
		// 	- Zero (0) if synced by GPS time ie IOP not running.
        return(iopRunning);
}

// **********************************************************************************************
// Capture SIGHALT from ControlC
void intHandler(int dummy) {
        keepRunning = 0;
}

// **********************************************************************************************
void print_diags(int nsys, int lastCycle, int sendLength, daq_multi_dcu_data_t *ixDataBlock) {
		int ii = 0;
		unsigned long sym_gps_sec = 0;
		unsigned long sym_gps_nsec = 0;

		sym_gps_sec = symm_gps_time(&sym_gps_nsec, 0);
		// Print diags in verbose mode
		printf("\nTime = %d-%d size = %d\n",shmIpcPtr[0]->bp[lastCycle].timeSec,shmIpcPtr[0]->bp[lastCycle].timeNSec,sendLength);
		printf("Sym gps = %d-%d (time received)\n", (int)sym_gps_sec, (int)sym_gps_nsec);
		printf("\tCycle = ");
		for(ii=0;ii<nsys;ii++) printf("\t\t%d",ixDataBlock->header.dcuheader[ii].cycle);
		printf("\n\tTimeSec = ");
		for(ii=0;ii<nsys;ii++) printf("\t%d",ixDataBlock->header.dcuheader[ii].timeSec);
		printf("\n\tTimeNSec = ");
		for(ii=0;ii<nsys;ii++) printf("\t\t%d",ixDataBlock->header.dcuheader[ii].timeNSec);
		printf("\n\tDataSize = ");
		for(ii=0;ii<nsys;ii++) printf("\t\t%d",ixDataBlock->header.dcuheader[ii].dataBlockSize);
	   	printf("\n\tTPCount = ");
	   	for(ii=0;ii<nsys;ii++) printf("\t\t%d",ixDataBlock->header.dcuheader[ii].tpCount);
	   	printf("\n\tTPSize = ");
	   	for(ii=0;ii<nsys;ii++) printf("\t\t%d",ixDataBlock->header.dcuheader[ii].tpBlockSize);
	   	printf("\n\tXmitSize = ");
	   	for(ii=0;ii<nsys;ii++) printf("\t\t%d",shmIpcPtr[ii]->dataBlockSize);
	   	printf("\n\n ");
}

// **********************************************************************************************
// Get control model loop rates from GDS param files
// Needed to properly size TP data into the data stream
int getmodelrate( int *rate, int *dcuid, char *modelname, char *gds_tp_dir) {
    char gdsfile[128];
    int ii = 0;
    FILE *f = 0;
    char *token = 0;
    char *search = "=";
    char line[80];
	char *s = 0;
	char *s1 = 0;

	if (gds_tp_dir) {
		sprintf(gdsfile, "%s/tpchn_%s.par", gds_tp_dir, modelname);
	} else {
		/// Need to get IFO and SITE info from environment variables.
		s = getenv("IFO");
		for (ii = 0; s[ii] != '\0'; ii++) {
			if (isupper(s[ii])) s[ii] = (char) tolower(s[ii]);
		}
		s1 = getenv("SITE");
		for (ii = 0; s1[ii] != '\0'; ii++) {
			if (isupper(s1[ii])) s1[ii] = (char) tolower(s1[ii]);
		}
		sprintf(gdsfile, "/opt/rtcds/%s/%s/target/gds/param/tpchn_%s.par", s1, s, modelname);
	}
    f = fopen(gdsfile, "rt");
    if (!f) return 0;
    while(fgets(line,80,f) != NULL) {
        line[strcspn(line, "\n")] = 0;
        if (strstr(line, "datarate") != NULL) {
            token = strtok(line, search);
            token = strtok(NULL, search);
            if (!token) continue;
            while (*token && *token == ' ') {
                ++token;
            }
            *rate = atoi(token);
            break;
        }
    }
    fclose(f);
    f = fopen(gdsfile, "rt");
    if (!f) return 0;
    while(fgets(line,80,f) != NULL) {
        line[strcspn(line, "\n")] = 0;
        if (strstr(line, "rmid") != NULL) {
            token = strtok(line, search);
            token = strtok(NULL, search);
            if (!token) continue;
            while (*token && *token == ' ') {
                ++token;
            }
            *dcuid = atoi(token);
            break;
        }
    }
    fclose(f);

    return 0;
}


// **********************************************************************************************
int loadMessageBuffer(	int nsys, 
						int lastCycle,
						int status,
						int dataRdy[],
						int *nsys_ready
					 )
{
	int sendLength = 0;
	int ii;
	int daqStatBit[2];
	daqStatBit[0] = 1;
	daqStatBit[1] = 2;
    int dataXferSize;
	char *dataBuff;
	int myCrc = 0;
	int crcLength = 0;

		// Set pointer to 0MQ message data block
		zbuffer = (char *)&ixDataBlock->dataBlock[0];
		// Initialize data send length to size of message header
		sendLength = header_size;
		// Set number of FE models that have data in this message
		ixDataBlock->header.fullDataBlockSize = 0;
		int db = 0;
		// Loop thru all FE models
		for (ii=0;ii<nsys;ii++) {
			if(dataRdy[ii]) {
				++(*nsys_ready);
				// Set heartbeat monitor for return to DAQ software
				if (lastCycle == 0) shmIpcPtr[ii]->reqAck ^= daqStatBit[0];
				// Set DCU ID in header
				ixDataBlock->header.dcuheader[db].dcuId = shmIpcPtr[ii]->dcuId;
				// Set DAQ .ini file CRC checksum
				ixDataBlock->header.dcuheader[db].fileCrc = shmIpcPtr[ii]->crc;
				// Set 1/16Hz cycle number
				ixDataBlock->header.dcuheader[db].cycle = shmIpcPtr[ii]->cycle;
				// Set GPS seconds
				ixDataBlock->header.dcuheader[db].timeSec = shmIpcPtr[ii]->bp[lastCycle].timeSec;
				// Set GPS nanoseconds
				ixDataBlock->header.dcuheader[db].timeNSec = shmIpcPtr[ii]->bp[lastCycle].timeNSec;
				crcLength = shmIpcPtr[ii]->bp[lastCycle].crc;
				// Set Status -- as running
				ixDataBlock->header.dcuheader[ii].status = 2;
				// Indicate size of data block
				// ********ixDataBlock->header.dcuheader[db].dataBlockSize = shmIpcPtr[ii]->dataBlockSize;
				ixDataBlock->header.dcuheader[db].dataBlockSize = crcLength;
				// Prevent going beyond MAX allowed data size
				if (ixDataBlock->header.dcuheader[db].dataBlockSize > DAQ_DCU_BLOCK_SIZE)
					ixDataBlock->header.dcuheader[db].dataBlockSize = DAQ_DCU_BLOCK_SIZE;
				// Calculate TP data size
                ixDataBlock->header.dcuheader[db].tpCount = (unsigned int)shmTpTable[ii]->count;
				ixDataBlock->header.dcuheader[db].tpBlockSize = sizeof(float) * modelrates[ii] * ixDataBlock->header.dcuheader[ii].tpCount /  DAQ_NUM_DATA_BLOCKS_PER_SECOND;

				// Copy GDSTP table to xmission buffer header
				memcpy(&(ixDataBlock->header.dcuheader[db].tpNum[0]),
				       &(shmTpTable[ii]->tpNum[0]),
					   sizeof(int)*ixDataBlock->header.dcuheader[db].tpCount);

				// Set pointer to dcu data in shared memory
				dataBuff = (char *)(shmDataPtr[ii] + lastCycle * buf_size);
				// Copy data from shared memory into local buffer
				dataXferSize = ixDataBlock->header.dcuheader[db].dataBlockSize + ixDataBlock->header.dcuheader[db].tpBlockSize;
				//dataXferSize = shmIpcPtr[ii]->dataBlockSize;
				memcpy((void *)zbuffer, dataBuff, dataXferSize);

				// Calculate CRC on the data and add to header info
				myCrc = 0;
				myCrc = crc_ptr((char *)zbuffer, crcLength, 0);
				myCrc = crc_len(crcLength, myCrc);
				ixDataBlock->header.dcuheader[db].dataCrc = myCrc;

				// Increment the 0mq data buffer pointer for next FE
				zbuffer += dataXferSize;
				// Increment the 0mq message size with size of FE data block
				sendLength += dataXferSize;
				// Increment the data block size for the message, this includes regular data + TP data
				ixDataBlock->header.fullDataBlockSize += dataXferSize;

				// Update heartbeat monitor to DAQ code
				if (lastCycle == 0) shmIpcPtr[ii]->reqAck ^= daqStatBit[1];
				db ++;
			}
		}
		ixDataBlock->header.dcuTotalModels = db;
		return sendLength;
}

// **********************************************************************************************
int send_to_local_memory(int nsys, int xmitData, int send_delay_ms, int pv_debug_pipe, char *pv_prefix)
{
    int do_wait = 1;
    int daqStatBit[2];
    daqStatBit[0] = 1;
    daqStatBit[1] = 2;
	char *nextData;

    
    int ii;
    int lastCycle = 0;
	unsigned int nextCycle = 0;

  	int sync2iop = 1;
	int status = 0;
	int dataRdy[10];
	//int msg_size;
	int sendLength = 0;
	int time_delta = 0;
	int expected_nsys = nsys;
	int actual_nsys = 0;


	SimplePV pvs[] = {
		{
			"FE_EXPECTED_DCUS",
			SIMPLE_PV_INT,
				(void*)&expected_nsys,
			nsys + 2,
			0,
			nsys + 1,
			nsys-1,
		},
		{
			"FE_ACTUAL_DCUS",
			SIMPLE_PV_INT,
			(void*)&actual_nsys,
			nsys + 2,
			0,
			nsys + 1,
			nsys-1,
		},
		{
			"FE_DATA_SIZE",
			SIMPLE_PV_INT,
			(void *)&sendLength,
			4*1024*1024,
			0,
			4*1024*1024,
			0,
		},
		{
			"FE_TIME_DELTA_MS",
			SIMPLE_PV_INT,
			(void *)&time_delta,
			62,
			0,
			60,
			1,
		},
	};


	for(ii=0;ii<10;ii++) dataRdy[ii] = 0;


	long cur_gps=0, cur_nano=0;
	long seg_gps=0, seg_nano=0;
	int dummy = 0;

    do {
        
		for(ii=0;ii<nsys;ii++) dataRdy[ii] = 0;

		status = waitNextCycle2(nsys,nextCycle,sync2iop,dataRdy,shmIpcPtr);
		// status = waitNextCycle(nextCycle,sync2iop,shmIpcPtr[0]);
		if(!status) {
			keepRunning = 0;;
			return(0);
		}
		else sync2iop = 0;

		// IOP will be first model ready
		// Need to wait for 2K models to reach end of their cycled
		usleep((do_wait * 1000));


		nextData = (char *)ifo_data;
		nextData += cycle_data_size * nextCycle;
		ixDataBlock = (daq_multi_dcu_data_t *)nextData;
		actual_nsys = 0;
		sendLength = loadMessageBuffer(nsys, nextCycle, status, dataRdy, &actual_nsys);
		// Print diags in verbose mode
		if(nextCycle == 0 && do_verbose) print_diags(nsys,lastCycle,sendLength,ixDataBlock);
		// Write header info
		ifo_header->curCycle = nextCycle;
        ifo_header->cycleDataSize = cycle_data_size;
        ifo_header->maxCycle = DAQ_NUM_DATA_BLOCKS_PER_SECOND;

		if(xmitData) {
			// Copy data to 0mq message buffer
			memcpy((void*)&msg_buffer,nextData,sendLength);
			// Send Data
			//msg_size = zmq_send(daq_publisher,(void*)&msg_buffer,sendLength,0);
			usleep(send_delay_ms * 1000);
			zmq_send_daq_multi_dcu_t(&msg_buffer, daq_publisher, 0);
			// printf("Sending data size = %d\n",msg_size);
		}
		seg_gps = ixDataBlock->header.dcuheader[0].timeSec;
		seg_nano = ixDataBlock->header.dcuheader[0].timeNSec;
		cur_gps = (long)symm_gps_time((unsigned long*)&cur_nano, &dummy);

		time_delta = (cur_gps - seg_gps)*1000 + (cur_nano - seg_nano)/1000000;

		send_pv_update(pv_debug_pipe, pv_prefix, pvs, sizeof(pvs)/sizeof(pvs[0]));

		nextCycle = (nextCycle + 1) % 16;
            
    } while (keepRunning); /* do this until sighalt */
    
    printf("\n***********************************************************\n\n");
    
    
    return 0;
}


/*********************************************************************************/
/*                                M A I N                                        */
/*                                                                               */
/*********************************************************************************/

int __CDECL
main(int argc,char *argv[])
{
    int counter = 0;
    int nsys = 1;
    int dcuId[10];
    int ii = 0;
    char *gds_tp_dir = 0;
	int max_data_size_mb = 64;
	int max_data_size = 0;
	int error = 0;
	int status = -1;
	unsigned long gps_frac = 0;
	int gps_stt = 0;
	int gps_ok = 0;
	unsigned long gps_time = 0;
	char *eport = 0;
	int sendViaZmq = 0;
	char *buffer_name = "ifo";
	int send_delay_ms = 0;
	char *pv_prefix = 0;
	char *pv_debug_pipe_name = 0;
	int pv_debug_pipe = -1;

    printf("\n %s compiled %s : %s\n\n",argv[0],__DATE__,__TIME__);
   
    printf("argc = %d\n", argc);
    for (ii = 0; ii < argc; ++ii)
    {
    	printf("argv[%d] = %s\n", ii, argv[ii]);
    }
    ii = 0;
    
    if (argc<3) {
        Usage();
        return(-1);
    }

    /* Get the parameters */
     while ((counter = getopt(argc, argv, "b:e:m:h:v:s:d:D:p:P:")) != EOF)
      switch(counter) {
        case 'b':
            buffer_name = optarg;
        break;

        case 'm':
            max_data_size_mb = atoi(optarg);
            if (max_data_size_mb < 20){
                printf("Min data block size is 20 MB\n");
                return -1;
            }
            if (max_data_size_mb > 100){
                printf("Max data block size is 100 MB\n");
                return -1;
            }
            break;

        case 's':
	    sysname = optarg;
	    printf ("sysnames = %s\n",sysname);
            continue;
        case 'v':
	    	do_verbose = atoi(optarg);
			break;
		case 'e':
	        eport = optarg;
	        printf ("eport = %s\n",eport);
			sendViaZmq = 1;
	        break;
	    case 'd':
	    	gds_tp_dir = optarg;
	    	break;
        case 'h':
            Usage();
            return(0);
        case 'D':
            send_delay_ms = atoi(optarg);
            break;
     	case 'p':
     		pv_prefix = optarg;
     		break;
     	case 'P':
     		pv_debug_pipe_name = optarg;
     		break;
    }
    if (pv_debug_pipe_name)
	{
    	pv_debug_pipe = open(pv_debug_pipe_name, O_NONBLOCK | O_RDWR, 0);
    	if (pv_debug_pipe < 0)
		{
    		fprintf(stderr, "Unable to open %s for writing (pv status)\n", pv_debug_pipe_name);
    		exit(1);
		}
	}

    max_data_size = max_data_size_mb * 1024*1024;

	if (sendViaZmq) printf("Writing DAQ data to local shared memory and sending out on ZMQ\n");
	else	printf("Writing DAQ data to local shared memory only \n");
    if(sysname != NULL) {
	printf("System names: %s\n", sysname);
        sprintf(modelnames[0],"%s",strtok(sysname, " "));
        for(;;) {
        	char *s = strtok(0, " ");
	        if (!s) break;
	        sprintf(modelnames[nsys],"%s",s);
	        dcuId[nsys] = 0;
	        nsys++;
	}
    } else {
	Usage();
        return(0);
    }

	if(sendViaZmq) zmq_make_connection(eport);
	// Open file descriptor for the gpstime driver
	symmetricom_fd =  open ("/dev/gpstime", O_RDWR | O_SYNC);
	if (symmetricom_fd < 0) {
	    perror("/dev/gpstime");
	exit(1);
	}
	
	gps_ok = symm_ok();
	gps_time = symm_gps_time(&gps_frac, &gps_stt);
	printf("GPS TIME = %ld\tfrac = %ld\tstt = %d\n",gps_time,gps_frac,gps_stt);

	// Parse the model names from the command line entry
    for(ii=0;ii<nsys;ii++) {
		char shmem_fname[128];
		sprintf(shmem_fname, "%s_daq", modelnames[ii]);
	 	void *dcu_addr = findSharedMemory(shmem_fname);
	 	if (dcu_addr == NULL) {
	 		fprintf(stderr, "Can't map shmem\n");
			exit(-1);
		} else {
			printf(" %s mapped at 0x%lx\n",modelnames[ii],(unsigned long)dcu_addr);
		}
	 	shmIpcPtr[ii] = (struct rmIpcStr *)((char *)dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
	 	shmDataPtr[ii] = ((char *)dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
	 	shmTpTable[ii] = (struct cdsDaqNetGdsTpNum *)((char *)dcu_addr + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
    }
	// Get model rates to get GDS TP data sizes.
    for (ii = 0; ii < nsys; ii++) {
        status = getmodelrate(&modelrates[ii],&dcuid[ii],modelnames[ii], gds_tp_dir);
		printf("Model %s rate = %d dcuid = %d\n",modelnames[ii],modelrates[ii],dcuid[ii]);
	    if (status != 0) {
	    	fprintf(stderr, "Unable to determine the rate of %s\n", modelnames[ii]);
	        exit(1);
        }
    }

	// Get pointers to local DAQ mbuf
	ifo = (char *)findSharedMemorySize(buffer_name, max_data_size_mb);
    ifo_header = (daq_multi_cycle_header_t *)ifo;
	ifo_data = (char *)ifo + sizeof(daq_multi_cycle_header_t);
	cycle_data_size = (max_data_size - sizeof(daq_multi_cycle_header_t))/DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    cycle_data_size -= (cycle_data_size % 8);

    signal(SIGINT,intHandler);
	sleep(1);



	// Enter infinite loop of reading control model data and writing to local shared memory
    do {
	    error = send_to_local_memory(nsys, sendViaZmq, send_delay_ms, pv_debug_pipe, pv_prefix);
    } while (error == 0 && keepRunning == 1);
	if(sendViaZmq) {
		printf("Closing out ZMQ and exiting\n");
		zmq_close(daq_publisher);
		zmq_ctx_destroy(daq_context);
	}

    return 0;
}

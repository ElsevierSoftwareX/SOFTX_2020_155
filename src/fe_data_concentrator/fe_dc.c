//
///// @file fe_dc.c
///// @brief  Front End data concentrator
////
//
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
#include "../include/daq_core.h"
#include "../drv/gpstime/gpstime.h"


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


int symmetricom_fd = -1;
unsigned long  window_start[16] = {63500,135000,197500,260000,322500,395000,447500,510000,572500,635000,697500,760000,822500,885000,947500,10000};
unsigned long  window_stop[16] =  {135000,197500,260000,322500,395000,447500,510000,572500,635000,697500,760000,822500,885000,947500,999990,63500};


/*********************************************************************************/
/*                                U S A G E                                      */
/*                                                                               */
/*********************************************************************************/

void Usage()
{
    printf("Usage of ix_multi_stream:\n");
    printf("fe_dc  -s <models> -m <memory size> \n");
    printf(" -s <value>     : Name of FE control models\n");
    printf(" -m <value>     : Local memory buffer size in megabytes\n");
    printf(" -h             : This helpscreen\n");
    printf("\n");
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
		do {
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
			}while(!iopRunning && timeout < 50);

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

		}while(iopRunning == 0 && keepRunning);
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
void print_diags(int nsys, int lastCycle, int sendLength) {
	int ii = 0;
		// Print diags in verbose mode
		printf("\nTime = %d-%d size = %d\n",shmIpcPtr[0]->bp[lastCycle].timeSec,shmIpcPtr[0]->bp[lastCycle].timeNSec,sendLength);
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
						unsigned int localtime,
						int dataRdy[]
					 )
{
	int sendLength = 0;
	int ii;
	int daqStatBit[2];
	daqStatBit[0] = 1;
	daqStatBit[1] = 2;
	int reftimeerror = 0;
    static int refcycle = 0;
    static unsigned int reftimeSec = 0;
    static unsigned int reftimeNSec = 0;
    int dataTPLength;
	char *dataBuff;
	int myCrc = 0;

		// Set pointer to 0MQ message data block
		zbuffer = (char *)&ixDataBlock->dataBlock[0];
		// Initialize data send length to size of message header
		sendLength = header_size;
		// Set number of FE models that have data in this message
		ixDataBlock->header.dataBlockSize = 0;
		int db = 0;
		// Loop thru all FE models
		for (ii=0;ii<nsys;ii++) {
			if(dataRdy[ii]) {
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
				// Set Status -- as running
				ixDataBlock->header.dcuheader[ii].status = 2;
				// Indicate size of data block
				ixDataBlock->header.dcuheader[db].dataBlockSize = shmIpcPtr[ii]->dataBlockSize;
				// Prevent going beyond MAX allowed data size
				if (ixDataBlock->header.dcuheader[db].dataBlockSize > DAQ_DCU_BLOCK_SIZE)
					ixDataBlock->header.dcuheader[db].dataBlockSize = DAQ_DCU_BLOCK_SIZE;
                ixDataBlock->header.dcuheader[db].tpCount = (unsigned int)shmTpTable[ii]->count;
				ixDataBlock->header.dcuheader[db].tpBlockSize = sizeof(float) * modelrates[ii] * ixDataBlock->header.dcuheader[ii].tpCount;
				// Prevent going beyond MAX allowed data size
				if (ixDataBlock->header.dcuheader[db].tpBlockSize > DAQ_DCU_BLOCK_SIZE)
					ixDataBlock->header.dcuheader[db].tpBlockSize = DAQ_DCU_BLOCK_SIZE;

				memcpy(&(ixDataBlock->header.dcuheader[db].tpNum[0]),
				       &(shmTpTable[ii]->tpNum[0]),
					   sizeof(int)*ixDataBlock->header.dcuheader[db].tpCount);

				// Set pointer to dcu data in shared memory
				dataBuff = (char *)(shmDataPtr[ii] + lastCycle * buf_size);
				// Copy data from shared memory into local buffer
				dataTPLength = ixDataBlock->header.dcuheader[db].dataBlockSize + ixDataBlock->header.dcuheader[db].tpBlockSize;
				memcpy((void *)zbuffer, dataBuff, dataTPLength);

				// Calculate CRC on the data and add to header info
				myCrc = 0;
				myCrc = crc_ptr((char *)zbuffer, ixDataBlock->header.dcuheader[db].dataBlockSize, 0);
				myCrc = crc_len(ixDataBlock->header.dcuheader[db].dataBlockSize, myCrc);
				ixDataBlock->header.dcuheader[db].dataCrc = myCrc;

				// Increment the 0mq data buffer pointer for next FE
				zbuffer += dataTPLength;
				// Increment the 0mq message size with size of FE data block
				sendLength += dataTPLength;
				// Increment the data block size for the message
				ixDataBlock->header.dataBlockSize += (unsigned int)dataTPLength;

				// Update heartbeat monitor to DAQ code
				if (lastCycle == 0) shmIpcPtr[ii]->reqAck ^= daqStatBit[1];
				db ++;
			}
		}
		ixDataBlock->header.dcuTotalModels = db;
		return sendLength;
}

// **********************************************************************************************
int send_to_local_memory(int nsys)
{
    int                   verbose = 1;
    int                   node_offset;


    int timeout;
    int myCrc;
    int do_wait = 1;
    int daqStatBit[2];
    daqStatBit[0] = 1;
    daqStatBit[1] = 2;
    int reftimeerror = 0;
    int refcycle;
    int reftimeSec;
    int reftimeNSec;
    int dataTPLength;
	char *nextData;
	unsigned long gps_frac;
	int gps_stt;
	unsigned long gps_time;

    
    int ii,jj;
	int new_cycle = 0;
    int lastCycle = 0;
	unsigned int nextCycle = 0;

  	int sync2iop = 1;
	int status = 0;
	int dataRdy[10];
	for(ii=0;ii<10;ii++) dataRdy[ii] = 0;


    do {
        
		for(ii=0;ii<nsys;ii++) dataRdy[ii] = 0;
		status = waitNextCycle2(nsys,nextCycle,sync2iop,dataRdy,shmIpcPtr);
		// status = waitNextCycle(nextCycle,sync2iop,shmIpcPtr[0]);
		if(status) sync2iop = 0;
		else sync2iop = 1;

		gps_time = symm_gps_time(&gps_frac, &gps_stt);
		gps_frac /= 1000;
		// printf("GPS TIME = %ld\tfrac = %ld\tcycle = %d\tstatus = %d\n",gps_time,gps_frac,nextCycle,status);

		// IOP will be first model ready
		// Need to wait for 2K models to reach end of their cycled
		usleep((do_wait * 1000));


		nextData = (char *)ifo_data;
		nextData += cycle_data_size * nextCycle;
		ixDataBlock = (daq_multi_dcu_data_t *)nextData;
		int sendLength = loadMessageBuffer(nsys, nextCycle, status,gps_time,dataRdy);
		// Print diags in verbose mode
		if(nextCycle == 0 && do_verbose) print_diags(nsys,lastCycle,sendLength);
		// Write header info
		ifo_header->curCycle = nextCycle;
        ifo_header->cycleDataSize = cycle_data_size;
        ifo_header->maxCycle = DAQ_NUM_DATA_BLOCKS_PER_SECOND;
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
    int counter; 
    int nsys = 1;
    int dcuId[10];
    int ii;
    char *gds_tp_dir = 0;
	int max_data_size = 64;
	int error = 0;
	int status = -1;
	unsigned long gps_frac;
	int gps_stt;
	int gps_ok;
	unsigned long gps_time;

    printf("\n %s compiled %s : %s\n\n",argv[0],__DATE__,__TIME__);
    
    if (argc<3) {
        Usage();
        return(-1);
    }

    /* Get the parameters */
     while ((counter = getopt(argc, argv, "r:n:g:l:s:m:h:a:v:")) != EOF) 
      switch(counter) {
      
        case 'm':
            max_data_size = atoi(optarg);
            if (max_data_size < 20){
                printf("Min data block size is 20 MB\n");
                return -1;
            }
            if (max_data_size > 100){
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

        case 'h':
            Usage();
            return(0);
    }
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
	ifo = (char *)findSharedMemorySize("ifo",max_data_size);
    ifo_header = (daq_multi_cycle_header_t *)ifo;
	ifo_data = (char *)ifo + sizeof(daq_multi_cycle_header_t);
	cycle_data_size = (max_data_size - sizeof(daq_multi_cycle_header_t))/DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    cycle_data_size -= (cycle_data_size % 8);

    signal(SIGINT,intHandler);
	sleep(1);



	// Enter infinite loop of reading control model data and writing to local shared memory
    do {
	    error = send_to_local_memory(nsys);
    } while (error == 0 && keepRunning == 1);

    return 0;
}
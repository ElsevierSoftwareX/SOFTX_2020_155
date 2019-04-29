//
///// @file mx_fe.c
///// @brief  Front End data concentrator
////
//

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
#include "../include/daq_core.h"
#include "../drv/gpstime/gpstime.h"
#include "myriexpress.h"
#include "mx_extensions.h"
#include <pthread.h>

#define MX_MUTEX_T pthread_mutex_t
#define MX_MUTEX_INIT(mutex_) pthread_mutex_init(mutex_, 0)
#define MX_MUTEX_LOCK(mutex_) pthread_mutex_lock(mutex_)
#define MX_MUTEX_UNLOCK(mutex_) pthread_mutex_unlock(mutex_)

#define MX_THREAD_T pthread_t
#define MX_THREAD_CREATE(thread_, start_routine_, arg_) \
pthread_create(thread_, 0, start_routine_, arg_)
#define MX_THREAD_JOIN(thread_) pthread_join(thread, 0)


MX_MUTEX_T stream_mutex;
#define FILTER     0x12345
#define MATCH_VAL  0xabcdef
#define DFLT_EID   64
#define DFLT_LEN   8192
#define DFLT_END   128
#define DFLT_ITER  1000
#define NUM_RREQ   16  /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/
#define NUM_SREQ   256  /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/
#define MSG_BUF_SIZE    0x200000

#define DO_HANDSHAKE 0
#define MATCH_VAL_MAIN (1 << 31)
#define MATCH_VAL_THREAD 1

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
static volatile int keepRunning = 1;
char *ifo;
char *ifo_data;
size_t cycle_data_size;

char msg_buffer[MSG_BUF_SIZE];

int symmetricom_fd = -1;


/*********************************************************************************/
/*                                U S A G E                                      */
/*                                                                               */
/*********************************************************************************/

void Usage()
{
    fprintf(stderr,"Usage of mx_fe:\n");
    fprintf(stderr,"mx_fe  -s <models> <OPTIONS>\n");
    fprintf(stderr," -b <buffer>    : Name of the mbuf to concentrate the data to locally (defaults to ifo)\n");
    fprintf(stderr," -s <value>     : Name of FE control models\n");
    fprintf(stderr," -m <value>     : Local memory buffer size in megabytes\n");
    fprintf(stderr," -l <filename>  : log file name\n");
    fprintf(stderr," -v 1           : Enable verbose output\n");
    fprintf(stderr," -e <0-31>	    : Number of the local mx end point to transmit on\n");
    fprintf(stderr," -r <0-31>      : Number of the remote mx end point to transmit to\n");
    fprintf(stderr," -t <target name: Name of MX target computer to transmit to\n");
    fprintf(stderr," -d <directory> : Path to the gds tp dir used to lookup model rates\n");
    fprintf(stderr," -D <value>     : Add a delay in ms to sending the data.  Used to spread the load\n");
    fprintf(stderr,"                : when working with multiple sending systems.  Defaults to 0.\n");
    fprintf(stderr," -h             : This helpscreen\n");
    fprintf(stderr,"\n");
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

// *******************************************************************************
/// See if the GPS card is locked.
int
symm_ok() {
	unsigned long req = 0;
	ioctl (symmetricom_fd, IOCTL_SYMMETRICOM_STATUS, &req);
    fprintf(stderr,"Symmetricom status: %s\n", req? "LOCKED": "UNCLOCKED");
	return req;
}

// *******************************************************************************
// Wait for data ready from FE models
// *******************************************************************************
int 
waitNextCycle2(	int nsys,
	    unsigned int cyclereq,		// Cycle to wait for
	    int reset,					// Request to reset model ipc shared memory
	    int dataRdy[],
	    struct rmIpcStr *ipcPtr[])	// Pointer to IOP IPC shared memory
{
int iopRunning = 0;
int ii;
int threads_rdy = 0;
int timeout = 0;

	// if reset, want to set IOP cycle to impossible number
	if(reset) ipcPtr[0]->cycle = 50;
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
		fprintf(stderr,"\nTime = %d-%d size = %d\n",shmIpcPtr[0]->bp[lastCycle].timeSec,shmIpcPtr[0]->bp[lastCycle].timeNSec,sendLength);
		fprintf(stderr,"Sym gps = %d-%d (time received)\n", (int)sym_gps_sec, (int)sym_gps_nsec);
		fprintf(stderr,"\tCycle = ");
		for(ii=0;ii<nsys;ii++) fprintf(stderr,"\t\t%d",ixDataBlock->header.dcuheader[ii].cycle);
		fprintf(stderr,"\n\tTimeSec = ");
		for(ii=0;ii<nsys;ii++) fprintf(stderr,"\t%d",ixDataBlock->header.dcuheader[ii].timeSec);
		fprintf(stderr,"\n\tTimeNSec = ");
		for(ii=0;ii<nsys;ii++) fprintf(stderr,"\t\t%d",ixDataBlock->header.dcuheader[ii].timeNSec);
		fprintf(stderr,"\n\tDataSize = ");
		for(ii=0;ii<nsys;ii++) fprintf(stderr,"\t\t%d",ixDataBlock->header.dcuheader[ii].dataBlockSize);
		fprintf(stderr,"\n\tTPCount = ");
		for(ii=0;ii<nsys;ii++) fprintf(stderr,"\t\t%d",ixDataBlock->header.dcuheader[ii].tpCount);
		fprintf(stderr,"\n\tTPSize = ");
		for(ii=0;ii<nsys;ii++) fprintf(stderr,"\t\t%d",ixDataBlock->header.dcuheader[ii].tpBlockSize);
		fprintf(stderr,"\n\tXmitSize = ");
		for(ii=0;ii<nsys;ii++) fprintf(stderr,"\t\t%d",shmIpcPtr[ii]->dataBlockSize);
		fprintf(stderr,"\n\n ");
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
			int dataRdy[]
			 )
{
	int sendLength = 0;
	int ii;
	int daqStatBit[2];
	daqStatBit[0] = 4;
	daqStatBit[1] = 8;
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
                ixDataBlock->header.dcuheader[db].tpCount = (unsigned int)shmTpTable[ii]->count & 0xff;
				ixDataBlock->header.dcuheader[db].tpBlockSize = sizeof(float) * modelrates[ii] * ixDataBlock->header.dcuheader[ii].tpCount /  DAQ_NUM_DATA_BLOCKS_PER_SECOND;

				// Copy GDSTP table to xmission buffer header
				memcpy(&(ixDataBlock->header.dcuheader[db].tpNum[0]),
				       &(shmTpTable[ii]->tpNum[0]),
					   sizeof(int)*ixDataBlock->header.dcuheader[db].tpCount);

				// Set pointer to dcu data in shared memory
				dataBuff = (char *)(shmDataPtr[ii] + lastCycle * buf_size);
				// Copy data from shared memory into local buffer
				dataXferSize = ixDataBlock->header.dcuheader[db].dataBlockSize + ixDataBlock->header.dcuheader[db].tpBlockSize;
				// if the dataXferSize is too large, something is wrong so return error message.
                if(dataXferSize > DAQ_DCU_BLOCK_SIZE) return(-1);
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
int send_to_local_memory(int nsys, 
			int xmitData, 
			int send_delay_ms,
			mx_endpoint_t ep,
			int64_t his_nic_id,
			uint16_t his_eid,
			int len,
			uint32_t match_val, 
			uint16_t my_dcu)
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

	int cur_req;
	mx_status_t stat;
	mx_request_t req[NUM_SREQ];
	mx_segment_t seg;
	uint32_t result;
	mx_endpoint_addr_t dest;
	uint32_t filter = FILTER;
	int init_mx = 1;

	for(ii=0;ii<10;ii++) dataRdy[ii] = 0;

	mx_set_error_handler(MX_ERRORS_RETURN);
    int myErrorSignal = 1;

    do {

        if(init_mx && xmitData) {
	        do{
	            mx_return_t conStat = mx_connect(ep, his_nic_id, his_eid, filter,
			        1000, &dest);
	            if (conStat != MX_SUCCESS) myErrorSignal = 1;
		        else {
			        myErrorSignal = 0;
			        fprintf(stderr, "Connection Made\n");
                    mx_return_t ret = mx_set_request_timeout(ep, 0, 1); // Set one second timeout
                    if (ret != MX_SUCCESS) {
                        fprintf(stderr, "Failed to set request timeout %s\n", mx_strerror(ret));
                        exit(1);
                    }

		        }
	        }while(myErrorSignal);
	        init_mx = 0;
	    }
	    myErrorSignal = 0;
        
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
		int sendLength = loadMessageBuffer(nsys, nextCycle, status,dataRdy);
        if(sendLength == -1 || sendLength > MSG_BUF_SIZE) {
		    fprintf(stderr, "Message buffer overflow error\n");
            return(-1);
        }
		// Print diags in verbose mode
		if(nextCycle == 8 && do_verbose) print_diags(nsys,lastCycle,sendLength,ixDataBlock);
		// Write header info
		ifo_header->curCycle = nextCycle;
        ifo_header->cycleDataSize = cycle_data_size;
        ifo_header->maxCycle = DAQ_NUM_DATA_BLOCKS_PER_SECOND;

		if(xmitData) {
		    // Copy data to 0mq message buffer
		    memcpy((void*)&msg_buffer,nextData,sendLength);
		    // Send Data
            usleep(send_delay_ms * 1000);
		    seg.segment_ptr = &msg_buffer;
		    seg.segment_length = sendLength;
		    mx_return_t res = mx_isend(ep, &seg, 1, dest, match_val, NULL, &req[cur_req]);
		    if (res != MX_SUCCESS) {
			    fprintf(stderr, "mx_isend failed ret=%d\n", res);
		        myErrorSignal = 1;
                break;
            }
again:
	        res = mx_wait(ep, &req[cur_req], 50, &stat, &result);
            if (res != MX_SUCCESS) {
                fprintf(stderr, "mx_cancel() failed with status %s\n", mx_strerror(res));
                exit(1);
            }
		    if (result == 0) {
                fprintf(stderr, "trying again \n");
                goto again;
                // myErrorSignal = 1;
            }
            if (stat.code != MX_STATUS_SUCCESS) {
                fprintf(stderr, "isendxxx failed with status %s\n", mx_strstatus(stat.code));
                myErrorSignal = 1;
            }
		}
		
		nextCycle = (nextCycle + 1) % 16;
            
    } while (keepRunning && !myErrorSignal); /* do this until sighalt */
    
    fprintf(stderr,"\n***********************************************************\n\n");
    
    
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
	int sendViaOmx = 0;
	char *buffer_name = "ifo";
	int send_delay_ms = 0;

	mx_endpoint_t ep;
    uint16_t my_eid;
    uint64_t his_nic_id;
    uint32_t board_id;
    uint32_t filter;
    uint16_t his_eid;
    char *rem_host;
    char *sysname;
    int len;
    int iter;
    int do_wait;
    int do_bothways;
    extern char *optarg;
	mx_return_t ret;


	rem_host = NULL;
    sysname = NULL;
    filter = FILTER;
    my_eid = DFLT_EID;
    his_eid = DFLT_EID;
    board_id = MX_ANY_NIC;
    len = DFLT_LEN;
    iter = DFLT_ITER;
    do_wait = 0;
    do_bothways = 0;



    fprintf(stderr,"\n %s compiled %s : %s\n\n",argv[0],__DATE__,__TIME__);
   
    ii = 0;
    
    if (argc<3) {
        Usage();
        return(-1);
    }

    /* Get the parameters */
     while ((counter = getopt(argc, argv, "b:e:m:h:v:s:r:t:d:l:D:")) != EOF)
      switch(counter) {
        case 't':
			rem_host = optarg;
			sendViaOmx = 1;
        break;
        case 'b':
            buffer_name = optarg;
        break;

        case 'm':
            max_data_size_mb = atoi(optarg);
            if (max_data_size_mb < 20){
                fprintf(stderr,"Min data block size is 20 MB\n");
                return -1;
            }
            if (max_data_size_mb > 100){
                fprintf(stderr,"Max data block size is 100 MB\n");
                return -1;
            }
            break;

        case 's':
	        sysname = optarg;
	        fprintf (stderr,"sysnames = %s\n",sysname);
            continue;
        case 'l':
            if (0 == freopen(optarg, "w", stdout)) {
                perror ("freopen");
                exit (1);
            }
            setvbuf(stdout, NULL, _IOLBF, 0);
            stderr = stdout;
            break;
		case 'r':
			his_eid = atoi(optarg);
            break;
        case 'v':
		    do_verbose = atoi(optarg);
			break;
		case 'e':
		    my_eid = atoi(optarg);
		    fprintf (stderr,"myeid = %d\n",my_eid);
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
    }


    max_data_size = max_data_size_mb * 1024*1024;

    // If sending to DAQ via net enabled, ensure all options have been set
	if (sendViaOmx) {
        mx_init();
        MX_MUTEX_INIT(&stream_mutex);
        if(my_eid == DFLT_EID || his_eid == DFLT_EID){
        fprintf(stderr, "\n***ERROR\n***Must set both -e and -r options to send data to DAQ\n\n");
        Usage();
        return(0);
        }
        fprintf(stderr,"Writing DAQ data to local shared memory and sending out on Open-MX\n");
	} else	{
        fprintf(stderr,"Writing DAQ data to local shared memory only \n");
    }

    // Parse the model names
    if(sysname != NULL) {
	    fprintf(stderr,"System names: %s\n", sysname);
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
	fprintf(stderr,"GPS TIME = %ld\tfrac = %ld\tstt = %d\n",gps_time,gps_frac,gps_stt);

	// Find the shared memory locations for the various model names
    for(ii=0;ii<nsys;ii++) {
		char shmem_fname[128];
		sprintf(shmem_fname, "%s_daq", modelnames[ii]);
		void *dcu_addr = findSharedMemory(shmem_fname);
		if (dcu_addr == NULL) {
			fprintf(stderr, "Can't map shmem\n");
			exit(-1);
		} else {
			fprintf(stderr," %s mapped at 0x%lx\n",modelnames[ii],(unsigned long)dcu_addr);
		}
		shmIpcPtr[ii] = (struct rmIpcStr *)((char *)dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
		shmDataPtr[ii] = ((char *)dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
		shmTpTable[ii] = (struct cdsDaqNetGdsTpNum *)((char *)dcu_addr + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
    }

	// Get model rates to get GDS TP data sizes.
    for (ii = 0; ii < nsys; ii++) {
        status = getmodelrate(&modelrates[ii],&dcuid[ii],modelnames[ii], gds_tp_dir);
		fprintf(stderr,"Model %s rate = %d dcuid = %d\n",modelnames[ii],modelrates[ii],dcuid[ii]);
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

    // Setup signal handler to catch Control C
    signal(SIGINT,intHandler);
	sleep(1);

    if(sendViaOmx) {
        // Open the NIC endpoint to send data
	    fprintf(stderr,"Open endpoint \n");
	    ret = mx_open_endpoint(board_id, my_eid, filter, NULL, 0, &ep);
        if (ret != MX_SUCCESS) {
            fprintf(stderr, "Failed to open endpoint %s\n", mx_strerror(ret));
            exit(1);
        }
	    sleep(1);
	    mx_hostname_to_nic_id(rem_host, &his_nic_id);
    }

	// Enter infinite loop of reading control model data and writing to local shared memory
    do {
	    error = send_to_local_memory(nsys, sendViaOmx, send_delay_ms,ep,his_nic_id,his_eid,len,MATCH_VAL_MAIN,my_eid);
    } while (error == 0 && keepRunning == 1);

    // Cleanup Open-MX stuff
	if(sendViaOmx) {
		fprintf(stderr,"Closing out OpenMX and exiting\n");
		mx_close_endpoint(ep);
		mx_finalize();
	}

    return 0;
}

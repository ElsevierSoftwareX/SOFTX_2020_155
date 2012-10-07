//
// Open-MX data sender, supports sending data
// from the IOP as well as from the slaves
//

#include "myriexpress.h"
#include "mx_extensions.h"
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
#include "../drv/crc.c"

#define FILTER     0x12345
#define MATCH_VAL  0xabcdef
#define DFLT_EID   1
#define DFLT_LEN   8192
#define DFLT_END   128
#define MAX_LEN    (1024*1024*1024) 
#define DFLT_ITER  1000
#define NUM_RREQ   16  /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/
#define NUM_SREQ   256  /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/

#define DO_HANDSHAKE 0
#define MATCH_VAL_MAIN (1 << 31)
#define MATCH_VAL_THREAD 1

#define DAQ_NUM_DATA_BLOCKS     16
#define DAQ_NUM_DATA_BLOCKS_PER_SECOND  16
#define CDS_DAQ_NET_IPC_OFFSET 0x0
#define CDS_DAQ_NET_GDS_TP_TABLE_OFFSET 0x1000
#define CDS_DAQ_NET_DATA_OFFSET 0x2000
#define DAQ_DCU_SIZE            0x400000
#define DAQ_DCU_BLOCK_SIZE      (DAQ_DCU_SIZE/DAQ_NUM_DATA_BLOCKS)
#define DAQ_GDS_MAX_TP_NUM           0x100
#define MMAP_SIZE 1024*1024*64-5000


int do_verbose;
int num_threads;
volatile int threads_running;


extern void *findSharedMemory(char *);

void
usage()
{
	fprintf(stderr, "Usage: mx_stream [args] -s sys_names -d rem_host:0\n");
	fprintf(stderr, "-l filename - log file name\n"); 
	fprintf(stderr, "-n nic_id - local NIC ID [MX_ANY_NIC]\n");
	fprintf(stderr, "-b board_id - local Board ID [MX_ANY_NIC]\n");
	fprintf(stderr, "-e local_eid - local endpoint ID [%d]\n", DFLT_EID);
	fprintf(stderr, "-r remote_eid - remote endpoint ID [%d]\n", DFLT_EID);
	fprintf(stderr, "-d hostname - destination hostname, required for sender\n");
	//fprintf(stderr, "-f filter - endpoint filter, default %x\n", FILTER);
	//fprintf(stderr, "-N iter - iterations, default %d\n", DFLT_ITER);
	fprintf(stderr, "-s - system names: \"x1x12 x1lsc x1asc\"\n");
	fprintf(stderr, "-v - verbose\n");
	//fprintf(stderr, "-x - bothways\n");
	fprintf(stderr, "-w - wait\n");
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


struct daqMXdata {
	struct rmIpcStr mxIpcData;
	cdsDaqNetGdsTpNum mxTpTable;
	char mxDataBlock[DAQ_DCU_BLOCK_SIZE];
};
int nsys; // The number of mapped shared memories (number of data sources)
static struct rmIpcStr *shmIpcPtr[128];
static char *shmDataPtr[128];
static struct cdsDaqNetGdsTpNum *shmTpTable[128];
static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;
static const int header_size = sizeof(struct rmIpcStr) + sizeof(struct cdsDaqNetGdsTpNum);


static inline void
sender(	mx_endpoint_t ep,
	int64_t his_nic_id,
	uint16_t his_eid,
	int len,
	uint32_t match_val, uint16_t my_dcu)
{

int cur_req;
mx_status_t stat;	
mx_request_t req[NUM_SREQ];
mx_segment_t seg;
uint32_t result;
struct daqMXdata mxDataBlock;
int myErrorSignal;
mx_endpoint_addr_t dest;
uint32_t filter = FILTER;
//struct timeval start_time;
char *dataBuff;
int sendLength = 0;
int myCrc = 0;

	

mx_set_error_handler(MX_ERRORS_RETURN);

do {
	int lastCycle = 0;

	myErrorSignal = 0;
	do {

		mx_return_t conStat = mx_connect(ep, his_nic_id, his_eid, filter, 
			   1000, &dest);
			   // MX_INFINITE, &dest);
		if (conStat != MX_SUCCESS) {
			fprintf(stderr, "mx_connect failed %s\n", mx_strerror(conStat));
			myErrorSignal = 1;
			for (int i = 0; i < nsys; i++) shmIpcPtr[i]->status ^= 1;
			exit(1);
		}
		else {
			myErrorSignal = 0;
			for (int i = 0; i < nsys; i++) shmIpcPtr[i]->status ^= 2;
			fprintf(stderr, "Connection Made\n");
			fflush(stderr);
		}
	}while(myErrorSignal);

	mx_return_t ret = mx_set_request_timeout(ep, 0, 1); // Set one second timeout
	if (ret != MX_SUCCESS) {
		fprintf(stderr, "Failed to set request timeout %s\n", mx_strerror(ret));
		exit(1);
	}

// Have a connection
	myErrorSignal = 0;
	cur_req = 0;
	usleep(1000000);

	// Waity for cycle 0
	for (;shmIpcPtr[0]->cycle;) usleep(1000);
	lastCycle = 0;

	if (!myErrorSignal) {
	do {
		int new_cycle;
		// Wait for cycle count update from FE
		do{
			usleep(1000);
			//printf("%d\n", shmIpcPtr[0]->cycle);
			new_cycle = shmIpcPtr[0]->cycle;
	        } while (new_cycle == lastCycle);
		        
		 lastCycle++;
		 lastCycle %= 16;

		// Send data for each system
		for (int i = 0; i < nsys; i++) {
		  mxDataBlock.mxTpTable = shmTpTable[i][0];
		  // printf("data copy\n");

		  // Copy values from shmmem to MX buffer
		  if (lastCycle == 0) shmIpcPtr[i]->status ^= 1;
		  cur_req = (cur_req + 1) % NUM_SREQ;
		  mxDataBlock.mxIpcData.cycle = lastCycle;
		  mxDataBlock.mxIpcData.crc = shmIpcPtr[i]->crc;
		  mxDataBlock.mxIpcData.dcuId = shmIpcPtr[i]->dcuId;
		  mxDataBlock.mxIpcData.dataBlockSize = shmIpcPtr[i]->dataBlockSize;
		  if (mxDataBlock.mxIpcData.dataBlockSize > DAQ_DCU_BLOCK_SIZE)
		  	mxDataBlock.mxIpcData.dataBlockSize = DAQ_DCU_BLOCK_SIZE;
		  mxDataBlock.mxIpcData.bp[lastCycle].timeSec = shmIpcPtr[i]->bp[lastCycle].timeSec;
		  mxDataBlock.mxIpcData.bp[lastCycle].timeNSec = shmIpcPtr[i]->bp[lastCycle].timeNSec;
		  mxDataBlock.mxIpcData.bp[lastCycle].cycle = shmIpcPtr[i]->bp[lastCycle].cycle;
		  mxDataBlock.mxIpcData.bp[lastCycle].crc = shmIpcPtr[i]->bp[lastCycle].crc;

		  dataBuff = (char *)(shmDataPtr[i] + lastCycle * buf_size);
		  memcpy((void *)&mxDataBlock.mxDataBlock[0],dataBuff,mxDataBlock.mxIpcData.dataBlockSize);
		  myCrc = crc_ptr((char *)&mxDataBlock.mxDataBlock[0],mxDataBlock.mxIpcData.dataBlockSize,0);
		  myCrc = crc_len(mxDataBlock.mxIpcData.dataBlockSize,myCrc);
		  //if(myCrc != mxDataBlock.mxIpcData.bp[lastCycle].crc) printf("CRC error in sender\n");
		  sendLength = header_size + mxDataBlock.mxIpcData.dataBlockSize;
 //printf("send length = %d  total length = %ld\n",sendLength,sizeof(struct daqMXdata));

		  seg.segment_ptr = &mxDataBlock;
		  // seg.segment_length = len;
		  seg.segment_length = sendLength;
		  //printf("isend req %d\n", cur_req);
		  mx_return_t res = mx_isend(ep, &seg, 1, dest, match_val, NULL, &req[cur_req]);
		  if (res != MX_SUCCESS) {
			fprintf(stderr, "mx_isend failed ret=%d\n", res);
			//exit(1);
			myErrorSignal = 1;
			break;
		  }
		  // mx_isend(ep, &seg, 1, dest, 1, NULL, &req[cur_req]);

		  /* wait for the send to complete */
		  mx_return_t ret;
again:
		  ret = mx_wait(ep, &req[cur_req], 25/*MX_INFINITE*/, &stat, &result);
                  if (ret != MX_SUCCESS) {
                          fprintf(stderr, "mx_cancel() failed with status %s\n", mx_strerror(ret));
                          exit(1);
                  }
                  if (result == 0) { // Request incomplete
                        goto again; // Restart incomplete request
                  } else { // Request is complete
		     if (stat.code != MX_STATUS_SUCCESS) {
			fprintf(stderr, "isendxxx failed with status %s\n", mx_strstatus(stat.code));
                        ret = mx_disconnect(ep, dest);
                        if (ret != MX_SUCCESS) {
                           fprintf(stderr, "mx_disconnect() failed with status %s\n", mx_strerror(ret));
                        } else {
                           fprintf(stderr, "disconnected from the sender\n");
                        }
			exit(1);
			myErrorSignal = 1;
		     }
		  }
		 // if(lastCycle == 15) myErrorSignal = 1;
		  if(lastCycle == 15) shmIpcPtr[i]->status ^= 2;
		}

	} while(!myErrorSignal);
	//mx_disconnect(ep, dest);
	}
// printf("test loop error\n");
} while(1);
}
	
int 
main(int argc, char **argv)
{
	mx_endpoint_t ep;
	uint64_t nic_id;
	uint16_t my_eid;
	uint64_t his_nic_id;
	uint32_t board_id;
	uint32_t filter;
	uint16_t his_eid;
	char *rem_host;
	char *sysname;
	int len;
	int iter;
	int c;
	int do_wait;
	int do_bothways;
	extern char *optarg;
	mx_return_t ret;

#if DEBUG
	extern int mx_debug_mask;
	mx_debug_mask = 0xFFF;
#endif
	// So that openmx is not aborting on connection loss
	putenv("OMX_FATAL_ERRORS=0");
	putenv("MX_FATAL_ERRORS=0");

	mx_init();
	/* set up defaults */
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
	num_threads = 1;


	while ((c = getopt(argc, argv, "hd:e:f:n:b:r:s:l:N:Vvwx")) != EOF) switch(c) {
	case 'd':
		rem_host = optarg;
		break;
	case 's':
		sysname = optarg;
		//printf ("sysnames = %s\n",sysname);
		break;
	case 'e':
		my_eid = atoi(optarg);
		break;
	case 'f':
		filter = atoi(optarg);
		break;
	case 'n':
		sscanf(optarg, "%"SCNx64, &nic_id);
		mx_nic_id_to_board_number(nic_id, &board_id);
		break;
	case 'b':
		board_id = atoi(optarg);
		break;
	case 'r':
		his_eid = atoi(optarg);
		break;
	case 'l':
        	if (0 == freopen(optarg, "w", stdout)) {
			perror ("freopen");
			exit (1);
		}
		setvbuf(stdout, NULL, _IOLBF, 0);
		stderr = stdout;
		break;
	case 'N':
		iter = atoi(optarg);
		break;
	case 'v':
		do_verbose = 1;
		break;
	case 'w':
		do_wait = 1;
		break;
	case 'x':
#if MX_THREAD_SAFE
		do_bothways = 1;
#else
		fprintf(stderr, "bi-directional mode only supported with threadsafe mx lib\n");
		exit(1);
#endif
		break;
	case 'h':
	default:
		usage();
		exit(1);
	}

	if (sysname == NULL) { usage(); exit(1); }
	if (rem_host == NULL) { usage(); exit (1); }

	//printf("System names: %s\n", sysname);

	if (rem_host != NULL)
		num_threads += do_bothways;
	ret = mx_open_endpoint(board_id, my_eid, filter, NULL, 0, &ep);
	if (ret != MX_SUCCESS) {
		fprintf(stderr, "Failed to open endpoint %s\n", mx_strerror(ret));
		exit(1);
	}

	/* get address of destination */
	mx_hostname_to_nic_id(rem_host, &his_nic_id);
	// mx_connect(ep, his_nic_id, his_eid, filter, MX_INFINITE, &his_addr);
	if (do_verbose) printf("Starting streaming send to host %s\n", rem_host);
	nsys = 1;
	char *sname[128];
	sname[0] = strtok(sysname, " ");
	for(;;) {
		printf("%s\n", sname[nsys - 1]);
		char *s = strtok(0, " ");
		if (!s) break;
		sname[nsys] = s;
		nsys++;
	}

	// Map shared memory for all systems
	for (int i = 0; i < nsys; i++) {
		char shmem_fname[128];
		sprintf(shmem_fname, "%s_daq", sname[i]);
		void *dcu_addr = findSharedMemory(shmem_fname);
		if (dcu_addr <= 0) {
			fprintf(stderr, "Can't map shmem\n");
			exit(1);
		} else printf("mapped at 0x%lx\n",(unsigned long)dcu_addr);
		shmIpcPtr[i] = (struct rmIpcStr *)(dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
		shmDataPtr[i] = (char *)(dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
		shmTpTable[i] = (struct cdsDaqNetGdsTpNum *)(dcu_addr + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
	}

	len = sizeof(struct daqMXdata);
	printf("send len = %d\n",len);
	sender(ep,his_nic_id, his_eid, len,MATCH_VAL_MAIN,my_eid); 
  
	mx_close_endpoint(ep);
	mx_finalize();
	exit(0);
}

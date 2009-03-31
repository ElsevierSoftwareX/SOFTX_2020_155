//
// Myrinet MX DQ data sender/receiver
//
// RECVR USAGE:./mx_stream -v -b 1
// XMIT USAGE:./mx_stream -v -b 1 -d scipe12:1

#include "myriexpress.h"

#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include <pthread.h>
#define MX_MUTEX_T pthread_mutex_t
#define MX_MUTEX_INIT(mutex_) pthread_mutex_init(mutex_, 0)
#define MX_MUTEX_LOCK(mutex_) pthread_mutex_lock(mutex_)
#define MX_MUTEX_UNLOCK(mutex_) pthread_mutex_unlock(mutex_)

#define MX_THREAD_T pthread_t
#define MX_THREAD_CREATE(thread_, start_routine_, arg_) \
pthread_create(thread_, 0, start_routine_, arg_)
#define MX_THREAD_JOIN(thread_) pthread_join(thread, 0)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mx_byteswap.h"
#include "test_common.h"

MX_MUTEX_T stream_mutex;

#define FILTER     0x12345
#define MATCH_VAL  0xabcdef
#define DFLT_EID   1
#define DFLT_LEN   8192
#define DFLT_END   128
#define MAX_LEN    (1024*1024*1024) 
#define DFLT_ITER  1000
#define NUM_RREQ   16  /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/
#define NUM_SREQ   4  /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/

#define DO_HANDSHAKE 1
#define MATCH_VAL_MAIN (1 << 31)
#define MATCH_VAL_THREAD 1

#define DAQ_NUM_DATA_BLOCKS     16
#define DAQ_NUM_DATA_BLOCKS_PER_SECOND  16
#define CDS_DAQ_NET_IPC_OFFSET 0x0
#define CDS_DAQ_NET_GDS_TP_TABLE_OFFSET 0x1000
#define CDS_DAQ_NET_DATA_OFFSET 0x2000
#define DAQ_DCU_SIZE            0x200000
#define DAQ_DCU_BLOCK_SIZE      (DAQ_DCU_SIZE/DAQ_NUM_DATA_BLOCKS)
#define DAQ_GDS_MAX_TP_NUM           0x100
#define MMAP_SIZE 1024*1024*64-5000


int Verify;
int do_verbose;
int num_threads;
volatile int threads_running;
unsigned char *dcu_addr;


void
usage()
{
	fprintf(stderr, "Usage: mx_stream [args] sys_name \n");
	fprintf(stderr, "sys_name - three letter name of a control system, e.g. tsa\n");
	fprintf(stderr, "-n nic_id - local NIC ID [MX_ANY_NIC]\n");
	fprintf(stderr, "-b board_id - local Board ID [MX_ANY_NIC]\n");
	fprintf(stderr, "-e local_eid - local endpoint ID [%d]\n", DFLT_EID);
	fprintf(stderr, "-r remote_eid - remote endpoint ID [%d]\n", DFLT_EID);
	fprintf(stderr, "-d hostname - destination hostname, required for sender\n");
	fprintf(stderr, "-f filter - endpoint filter, default %x\n", FILTER);
	fprintf(stderr, "-l length - message length, default %d\n", DFLT_LEN);
	fprintf(stderr, "-N iter - iterations, default %d\n", DFLT_ITER);
	fprintf(stderr, "-v - verbose\n");
	fprintf(stderr, "-x - bothways\n");
	fprintf(stderr, "-w - wait\n");
	fprintf(stderr, "-V - verify msg content [OFF]\n");
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
static struct rmIpcStr *shmIpcPtr;
static char *shmDataPtr;
static struct cdsDaqNetGdsTpNum *shmTpTable;
static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;
static const int header_size = sizeof(struct rmIpcStr) + sizeof(struct cdsDaqNetGdsTpNum);

static inline void
receiver(mx_endpoint_t ep, uint32_t match_val, uint32_t filter)
{
	int count, len, cur_req;
	mx_status_t stat;	
	mx_request_t req[NUM_RREQ];
	mx_segment_t seg;
	uint32_t result;
	struct timeval start_time, end_time;
	struct daqMXdata *dataPtr;
	char *buffer;
	int ii,jj,kk;
	int myErrorStat = 0;
	char *dataDest;
	char *dataSource;
	int copySize;
	//float *testData;

	printf("waiting for someone to connect\n");
	len = sizeof(struct daqMXdata);
	buffer = malloc(len * NUM_RREQ);
	if (buffer == NULL) {
		fprintf(stderr, "Can't allocate buffers here\n");
		exit(1);
}




	/* pre-post our receives */
	for (cur_req = 0; cur_req < NUM_RREQ; cur_req++) {
		seg.segment_ptr = &buffer[cur_req * len];
		seg.segment_length = len;
		mx_irecv(ep, &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, 
			 &req[cur_req]);
	}

	/* start the test */
mx_set_error_handler(MX_ERRORS_RETURN);
	gettimeofday(&start_time, NULL);
		kk = 0;
do{
	for (count = 0; count < NUM_RREQ; count++) {
		/* wait for the receive to complete */
		cur_req = count & (16 - 1);
		
		mx_wait(ep, &req[cur_req], 
				150, &stat, &result);
			myErrorStat = 0;
		 if (!result) {
			// fprintf(stderr, "mx_wait failed in rcvr %d\n",count);
			myErrorStat = 1;
			count --;
			// exit(1);
		}
		if (stat.code != MX_STATUS_SUCCESS) {
			fprintf(stderr, "irecv failed with status %s\n", mx_strstatus(stat.code));
			myErrorStat = 2;
			// exit(1);
		}

		copySize = stat.xfer_length;
		// seg.segment_ptr = buffer[cur_req];
		seg.segment_ptr = &buffer[cur_req * len];
		seg.segment_length = len;
		if(!myErrorStat)
		{
			dataPtr = (struct daqMXdata *) seg.segment_ptr;
			ii = dataPtr->mxIpcData.cycle;
			jj = dataPtr->mxIpcData.dataBlockSize;
			// printf("data from dcuid %d %d %d %d %d\n",dataPtr->mxIpcData.dcuId,dataPtr->mxIpcData.bp[ii].timeSec,
			// 	dataPtr->mxIpcData.bp[ii].timeNSec,ii,jj);
			dataSource = (char *)dataPtr->mxDataBlock;
			dataDest = (char *)(shmDataPtr + buf_size * ii);
			memcpy(dataDest,dataSource,jj);
#if 0
			testData = (float *)dataDest;
			testData += 4094;
			for(ii=0;ii<5;ii++)
			{
			printf("%f  ",*testData);
			testData ++;
			}
			printf("\n\n  ");
#endif
			shmIpcPtr->crc = dataPtr->mxIpcData.crc;
			shmIpcPtr->dcuId = dataPtr->mxIpcData.dcuId;
			shmIpcPtr->bp[ii].timeSec = dataPtr->mxIpcData.bp[ii].timeSec;
			shmIpcPtr->bp[ii].crc = dataPtr->mxIpcData.bp[ii].crc;
			shmIpcPtr->bp[ii].cycle = dataPtr->mxIpcData.bp[ii].cycle;
			shmIpcPtr->dataBlockSize = dataPtr->mxIpcData.dataBlockSize;
			shmIpcPtr->cycle = dataPtr->mxIpcData.cycle;
			//  printf("crc = 0x%x\n  ",shmIpcPtr->bp[ii].crc);
			


#if 0
		TODO: copy Ipc data after memcpy

		shmIpcPtr = (struct rmIpcStr *)(dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
		shmDataPtr = (char *)(dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
		shmTpTable = (struct cdsDaqNetGdsTpNum *)(dcu_addr + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
struct daqMXdata {
	struct rmIpcStr mxIpcData;
	cdsDaqNetGdsTpNum mxTpTable;
	char mxDataBlock[DAQ_DCU_BLOCK_SIZE];
};
			// ii = dataPtr->dcuId;
			datadest = (char *)(shmDataPtr + dataPtr->cycle * len);
			printf("data from dcuid %d %d %d %d\n",dataPtr->dcuId,dataPtr->timeSec,dataPtr->timeNSec,dataPtr->cycle);
			printf(" 0x%x 0x%x \n",datadest,seg.segment_ptr);
			if(dataPtr->timeSec != 0) memcpy((void *)datadest,(void *)seg.segment_ptr,len);
#endif
			mx_irecv(ep, &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, &req[cur_req]);
		}
		if(kk != myErrorStat)
		{
			kk = myErrorStat;
			if(kk==1) fprintf(stderr, "mx_wait failed in rcvr %d\n",count);
			if(kk==0) fprintf(stderr, "RCVR running %d\n",count);
		}
	}
}while(1);
	gettimeofday(&end_time, NULL);
			fprintf(stderr, "mx_wait failed in rcvr after do loop\n");
	free(buffer);
			exit(1);
}

static inline void
sender(mx_endpoint_t ep,uint64_t his_nic_id,  uint16_t his_eid,int len, uint32_t match_val, uint16_t my_dcu)
{
	int cur_req;
	mx_status_t stat;	
	mx_request_t req[NUM_SREQ];
	mx_segment_t seg;
	mx_return_t conStat;
	uint32_t result;
	struct daqMXdata mxDataBlock;
	//int ii;
	//int jj = 0;
	int myErrorSignal;
	mx_endpoint_addr_t dest;
	uint32_t filter = FILTER;
	//struct timeval start_time;
	int lastCycle = 0;
	char *dataBuff;
	int sendLength = 0;

	

mx_set_error_handler(MX_ERRORS_RETURN);

do{
	myErrorSignal = 0;
	do {
		conStat = mx_connect(ep, his_nic_id, his_eid, filter, 
			   MX_INFINITE, &dest);
		if (conStat != MX_SUCCESS) {
			fprintf(stderr, "mx_connect failed\n");
			myErrorSignal = 1;
			usleep(1000000);
			// exit(1);
		}
		else {
			myErrorSignal = 0;
		fprintf(stderr, "Connection Made\n");
		}
	}while(myErrorSignal);

// Have a connection
	myErrorSignal = 0;
usleep(1000);
if(!myErrorSignal)
{
	cur_req = 0;
	do {
		// Wait for cycle count update from FE
		do{
			usleep(1000);
		}while(shmIpcPtr->cycle == lastCycle);

		// Copy values from shmmem to MX buffer
		lastCycle = shmIpcPtr->cycle;
		mxDataBlock.mxIpcData.cycle = lastCycle;
		mxDataBlock.mxIpcData.crc = shmIpcPtr->crc;
		mxDataBlock.mxIpcData.dcuId = shmIpcPtr->dcuId;
		mxDataBlock.mxIpcData.dataBlockSize = shmIpcPtr->dataBlockSize;
		mxDataBlock.mxIpcData.bp[lastCycle].timeSec = shmIpcPtr->bp[lastCycle].timeSec;;
		mxDataBlock.mxIpcData.bp[lastCycle].timeNSec = shmIpcPtr->bp[lastCycle].timeNSec;;
		mxDataBlock.mxIpcData.bp[lastCycle].cycle = shmIpcPtr->bp[lastCycle].cycle;;
		mxDataBlock.mxIpcData.bp[lastCycle].crc = shmIpcPtr->bp[lastCycle].crc;;

		dataBuff = (char *)(shmDataPtr + lastCycle * buf_size);
		memcpy((void *)&mxDataBlock.mxDataBlock[0],dataBuff,mxDataBlock.mxIpcData.dataBlockSize);
		sendLength = header_size + mxDataBlock.mxIpcData.dataBlockSize;
// printf("send length = %d  total length = %ld\n",sendLength,sizeof(struct daqMXdata));
#if 0
	char *dataBuff;
struct daqMXdata {
	struct rmIpcStr mxIpcData;
	cdsDaqNetGdsTpNum mxTpTable;
	char mxDataBlock[DAQ_DCU_BLOCK_SIZE];
		shmDataPtr = (char *)(dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
};
#endif

		seg.segment_ptr = &mxDataBlock;
		// seg.segment_length = len;
		seg.segment_length = sendLength;
		mx_isend(ep, &seg, 1, dest, match_val, NULL, &req[cur_req]);

		/* wait for the send to complete */
		mx_wait(ep, &req[cur_req], 100, &stat, &result);
		if (!result) {
			fprintf(stderr, "mxWait failed with status %s\n", mx_strstatus(stat.code));
			// exit(1);
			myErrorSignal = 1;
		}
		if (stat.code != MX_STATUS_SUCCESS) {
			fprintf(stderr, "isendxxx failed with status %s\n", mx_strstatus(stat.code));
			// exit(1);
			myErrorSignal = 1;
		}

	}while(!myErrorSignal);
}
printf("test loop error\n");
}while(1);
	
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
	int len;
	int iter;
	int c;
	int do_wait;
	int do_bothways;
	extern char *optarg;
	mx_return_t ret;
	int fd;
	//char shmem1[64] = "/rtl_mem_tsa_daq";
	//char shmem2[64] = "/rtl_mem_tsb_daq";
	//char shmem3[64] = "/rtl_mem_tsc_daq";
	char *shmem_fname_format = "/rtl_mem_%s_daq";
	char shmem_fname[256];
	//int status;

#if DEBUG
	extern int mx_debug_mask;
	mx_debug_mask = 0xFFF;
#endif

	mx_init();
	MX_MUTEX_INIT(&stream_mutex);
	/* set up defaults */
	rem_host = NULL;
	filter = FILTER;
	my_eid = DFLT_EID;
	his_eid = DFLT_EID;
	board_id = MX_ANY_NIC;
	len = DFLT_LEN;
	iter = DFLT_ITER;
	do_wait = 0;
	do_bothways = 0;
	num_threads = 1;

	//printf("file = %s %s %s\n",shmem1,shmem2,shmem3);

	while ((c = getopt(argc, argv, "hd:e:f:n:b:r:l:N:Vvwx")) != EOF) switch(c) {
	case 'd':
		rem_host = optarg;
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
		len = atoi(optarg);
		if (len > MAX_LEN) {
			fprintf(stderr, "len too large, max is %d\n", MAX_LEN);
			exit(1);
		}
		break;
	case 'N':
		iter = atoi(optarg);
		break;
	case 'V':
		Verify = 1;
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

        if (argc != optind + 1) {  usage(); exit(1); }
	printf("System name: %s\n", argv[optind]);
	sprintf(shmem_fname, shmem_fname_format, argv[optind]);
        //if (argc) { int i; for (i = 0; i < argc; i++) printf ("%s\n", argv[i]);}

	if (rem_host != NULL)
		num_threads += do_bothways;
	ret = mx_open_endpoint(board_id, my_eid, filter, NULL, 0, &ep);
	if (ret != MX_SUCCESS) {
		fprintf(stderr, "Failed to open endpoint %s\n", mx_strerror(ret));
		exit(1);
	}

	/* If no host, we are receiver */
	if (rem_host == NULL) {
		if (do_verbose)
			printf("Starting streaming receiver\n");
		if (Verify) {
			fprintf(stderr, "-V ignored.  Verify must be set by sender\n");
			Verify = 0;
		}
#if 0
		// Open shared memory to FE DAQ
		if ((fd = open("/rtl_mem_tsc_daq", O_RDWR))<0) {
			fprintf(stderr, "Can't open shmem\n");
			exit(1);
	        }
#endif

		fd = open(shmem_fname, O_CREAT | O_RDWR, 0666);
		if(fd == -1)
		{
			printf("shmem open failed %s\n",shmem_fname);
			exit(1);
		}
#if 0
		/* Set the shared area to the right size */
		status = ftruncate(fd,MMAP_SIZE) ;
		if(status != 0){
			printf("shmem trunc failed %s %d\n",shmem3,MMAP_SIZE);
			perror("ftruncate failed");
			exit(1);
		}
			printf("WORKED\n");
#endif

		// Map shared memory
		dcu_addr = (unsigned char *)mmap(0, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd , 0);
		if (dcu_addr <= 0) {
			fprintf(stderr, "Can't map shmem\n");
			exit(1);
		}
		else printf("mapped at 0x%lx\n",(unsigned long)dcu_addr);

		shmIpcPtr = (struct rmIpcStr *)(dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
		shmDataPtr = (char *)(dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
		shmTpTable = (struct cdsDaqNetGdsTpNum *)(dcu_addr + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);

		receiver(ep, MATCH_VAL_MAIN, filter);
		

	} else {
		/* get address of destination */
		mx_hostname_to_nic_id(rem_host, &his_nic_id);
		// mx_connect(ep, his_nic_id, his_eid, filter, MX_INFINITE, &his_addr);
		if (do_verbose)
			printf("Starting streaming send to host %s\n", 
			       rem_host);
		if (Verify) printf("Verifying results\n");

		// Open shared memory to FE DAQ
		if ((fd = open("/rtl_mem_tsc_daq", O_RDWR))<0) {
			fprintf(stderr, "Can't open shmem\n");
			exit(1);
	        }

		// Map shared memory
		dcu_addr = (unsigned char *)mmap(0, 64*1024*1024-5000, PROT_READ | PROT_WRITE, MAP_SHARED, fd , 0);
		if (dcu_addr <= 0) {
			fprintf(stderr, "Can't map shmem\n");
			exit(1);
		}
		else printf("mapped at 0x%lx\n",(unsigned long)dcu_addr);

		shmIpcPtr = (struct rmIpcStr *)(dcu_addr + CDS_DAQ_NET_IPC_OFFSET);
		shmDataPtr = (char *)(dcu_addr + CDS_DAQ_NET_DATA_OFFSET);
		shmTpTable = (struct cdsDaqNetGdsTpNum *)(dcu_addr + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);

len = sizeof(struct daqMXdata);
printf("send len = %d\n",len);
sender(ep,his_nic_id, his_eid, len,MATCH_VAL_MAIN,my_eid); 
	}		

  
	mx_close_endpoint(ep);
	mx_finalize();
	exit(0);
}

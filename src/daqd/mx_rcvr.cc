// Myrinet MX DQ data receiver
//#define MX_THREAD_SAFE 1
#include "config.h"
#include "myriexpress.h"
#include "mx_extensions.h"
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define SHMEM_DAQ 1
#include "../include/daqmap.h"
#include "debug.h"
#include "daqd.hh"

extern daqd_c daqd;
#ifndef  USE_SYMMETRICOM
extern int controller_cycle;
#endif
extern struct rmIpcStr gmDaqIpc[DCU_COUNT];
extern struct cdsDaqNetGdsTpNum * gdsTpNum[2][DCU_COUNT];
extern void *directed_receive_buffer[DCU_COUNT];

#define FILTER     0x12345
#define MATCH_VAL  0xabcdef
#define DFLT_EID   1
#define DFLT_LEN   8192
#define DFLT_END   128
#define MAX_LEN    (1024*1024*1024) 
#define DFLT_ITER  1000
#define NUM_RREQ   256
#define NUM_SREQ   4  /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/

#define MATCH_VAL_MAIN (1 << 31)
#define MATCH_VAL_THREAD 1

struct daqMXdata {
	struct rmIpcStr mxIpcData;
	cdsDaqNetGdsTpNum mxTpTable;
	char mxDataBlock[DAQ_DCU_BLOCK_SIZE];
};

static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;


void
receiver_mx(int neid)
{
	mx_status_t stat;	
	mx_segment_t seg;
	uint32_t result;
	struct timeval start_time, end_time;
	char *buffer;
	int myErrorStat = 0;
	int copySize;
	//float *testData;
	uint32_t match_val=MATCH_VAL_MAIN;
	mx_request_t req[NUM_RREQ];
	mx_endpoint_t ep[256];


	const struct sched_param param = { 10 };
   	//seteuid (0);
   	pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
   	//seteuid (getuid ());

	unsigned int board_num = neid >> 8;
	neid &= 0xff;

        mx_return_t ret = mx_open_endpoint(board_num, neid, FILTER, NULL, 0, &ep[neid]);
        if (ret != MX_SUCCESS) {
	                fprintf(stderr, "Failed to open endpoint %s\n", mx_strerror(ret));
	                exit(1);
        }
        printf("MX endpoint %d opened\n", neid);
	#if 0
	ret = mx_set_request_timeout(ep[neid], 0, 1);
        if (ret != MX_SUCCESS) {
	                fprintf(stderr, "Failed to et request timeout %s\n", mx_strerror(ret));
			exit(1);
	}
	#endif

	// Allocate NUM_RREQ MX packet receive buffers
	int len = sizeof(struct daqMXdata);
	buffer = (char *)malloc(len * NUM_RREQ);
	if (buffer == NULL) {
		fprintf(stderr, "Can't allocate buffers here\n");
		exit(1);
	}


	/* pre-post our receives */
	for (int cur_req = 0; cur_req < NUM_RREQ; cur_req++) {
		seg.segment_ptr = &buffer[cur_req * len];
		seg.segment_length = len;
		mx_endpoint_t ep1 = ep[neid];
		mx_irecv(ep1, &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, 
			 &req[cur_req]);
	}

	mx_set_error_handler(MX_ERRORS_RETURN);
	gettimeofday(&start_time, NULL);
        int kk = 0;
	do {
	  for (int count = 0; count < NUM_RREQ; count++) {
		/* Wait for the receive to complete */
		int cur_req = count & (NUM_RREQ - 1);
		
// 	mx_test_or_wait(blocking, ep, &sreq, MX_INFINITE, &stat, &result);
again:
		mx_return_t ret = mx_wait(ep[neid], &req[cur_req], MX_INFINITE, &stat, &result);
		myErrorStat = 0;
		if (ret != MX_SUCCESS) {
			fprintf(stderr, "mx_cancel() failed with status %s\n", mx_strerror(ret));
			exit(1); // Not clear what to do in this case; this indicates shortage of memory or resources
		}
		if (result == 0) { // Request incomplete
			goto again; // Restart incomplete request
		} else { // Request is complete
			if (stat.code != MX_STATUS_SUCCESS) { // Request completed, but bad code
				fprintf(stderr, "mx_wait failed in rcvr eid=%d, reqn=%d; wait did not complete; status code is %s\n",
					neid, count, mx_strstatus(stat.code));
				//mx_return_t ret = mx_cancel(ep[neid], &req[cur_req], &result);
				mx_endpoint_addr_t ep_addr;
				ret = mx_get_endpoint_addr(ep[neid], &ep_addr);
				if (ret != MX_SUCCESS) {
					fprintf(stderr, "mx_get_endpoint_addr() failed with status %s\n", mx_strerror(ret));
				} else {
					ret = mx_disconnect(ep[neid], ep_addr);
					if (ret != MX_SUCCESS) {
						fprintf(stderr, "mx_disconnect() failed with status %s\n", mx_strerror(ret));
					} else {
						fprintf(stderr, "disconnected from the sender on endpoint %d\n", neid);
					}
				}
				myErrorStat = 1;
			}
		}
		// Fall through if the request is complete and the code is sucess
		//

		copySize = stat.xfer_length;
		// seg.segment_ptr = buffer[cur_req];
		seg.segment_ptr = &buffer[cur_req * len];
		seg.segment_length = len;

		if (!myErrorStat) {
		  //printf("received one\n");
		  struct daqMXdata *dataPtr = (struct daqMXdata *) seg.segment_ptr;
		  int dcu_id = dataPtr->mxIpcData.dcuId;
		  #ifndef USE_MAIN
		  if (dcu_id < 0 || dcu_id > (DCU_COUNT-1)) {
			mx_irecv(ep[neid], &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, &req[cur_req]);
		  	continue; // Bad DCU ID
		  }
		  if (daqd.dcuSize[0][dcu_id] == 0) {
			mx_irecv(ep[neid], &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, &req[cur_req]);
		  	continue; // Unconfigured DCU
	  	  }
		  #endif
		  int cycle = dataPtr->mxIpcData.cycle;
		  int len = dataPtr->mxIpcData.dataBlockSize;

#if 0
		  //if (dcu_id == 9)
		    printf("data dcuid=%d sec=%d nsec=%d cycle=%d dlen=%d filecrc=0x%x crc=0x%x\n",
			dcu_id, dataPtr->mxIpcData.bp[cycle].timeSec,
		 	dataPtr->mxIpcData.bp[cycle].timeNSec, cycle, len,
			dataPtr->mxIpcData.crc, dataPtr->mxIpcData.bp[cycle].crc);
#endif

		  char *dataSource = (char *)dataPtr->mxDataBlock;
		  char *dataDest = (char *)((char *)(directed_receive_buffer[dcu_id]) + buf_size * cycle);

#ifndef USE_MAIN
		  // Move the block data into the buffer
		  memcpy (dataDest, dataSource, len);
		  #endif

		  // Assign IPC data
		  gmDaqIpc[dcu_id].crc = dataPtr->mxIpcData.crc;
		  gmDaqIpc[dcu_id].dcuId = dataPtr->mxIpcData.dcuId;
		  gmDaqIpc[dcu_id].bp[cycle].timeSec = dataPtr->mxIpcData.bp[cycle].timeSec;
		  gmDaqIpc[dcu_id].bp[cycle].crc = dataPtr->mxIpcData.bp[cycle].crc;
		  gmDaqIpc[dcu_id].bp[cycle].cycle = dataPtr->mxIpcData.bp[cycle].cycle;
		  gmDaqIpc[dcu_id].dataBlockSize = dataPtr->mxIpcData.dataBlockSize;
			
		  system_log(6, "dcu %d gps %d %d %d\n", dcu_id, dataPtr->mxIpcData.bp[cycle].timeSec, cycle, dataPtr->mxIpcData.bp[cycle].timeNSec);

		  // Assign test points table
		  *gdsTpNum[0][dcu_id] = dataPtr->mxTpTable;

		  gmDaqIpc[dcu_id].cycle = cycle;
		  #ifndef USE_MAIN
#ifndef  USE_SYMMETRICOM
		  if (daqd.controller_dcu == dcu_id)  {
		  	   controller_cycle = cycle;
	                   DEBUG(6, printf("Timing dcu=%d cycle=%d\n", dcu_id, controller_cycle));
		  }
#endif
		  #endif
		  daqd.producer1.rcvr_stats[dcu_id].tick();
		}
		//daqd.producer1.rcvr_stats[neid].tick();
		mx_irecv(ep[neid], &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, &req[cur_req]);
		#if 0
		if (kk != myErrorStat){
		  kk = myErrorStat;
		  if(kk==1) fprintf(stderr, "mx_wait failed in rcvr %d\n",count);
		  if(kk==0) fprintf(stderr, "RCVR running %d\n",count);
		}
		#endif
	  }
	} while(1);
	gettimeofday(&end_time, NULL);
	fprintf(stderr, "mx_wait failed in rcvr after do loop\n");
	free(buffer);
	//exit(1);
}

int mx_ep_opened = 0;

/// Initialize MX library.
/// Returns the maximum number of end-points supoprted in the system.
unsigned int
open_mx(void)
{
	uint16_t my_eid;
	uint32_t board_id;
	uint32_t filter;
	char *sysname;
	int c;
	extern char *optarg;
	mx_return_t ret;
	int fd;
	static uint32_t max_endpoints = 0;
	static uint32_t nics_available = 0;
	

	if (mx_ep_opened) return max_endpoints | (nics_available << 8);

        printf("%ld\n", sizeof(struct daqMXdata));

	// So that openmx is not aborting on connection loss
	putenv("OMX_ERRORS_ARE_FATAL=0");
	putenv("MX_ERRORS_ARE_FATAL=0");

	mx_init();

	/* set up defaults */
	sysname = NULL;
	filter = FILTER;
#if 0
	my_eid = 1; //DFLT_EID;
	board_id = MX_ANY_NIC;
	ret = mx_open_endpoint(board_id, my_eid, FILTER, NULL, 0, &ep[1]);
	if (ret != MX_SUCCESS) {
		fprintf(stderr, "Failed to open endpoint %s\n", mx_strerror(ret));
		exit(1);
	}
	printf("MX endpoint opened\n");

        my_eid = 100;

	ret = mx_open_endpoint(board_id, my_eid, FILTER, NULL, 0, &ep[100]);
	if (ret != MX_SUCCESS) {
		fprintf(stderr, "Failed to open endpoint %s\n", mx_strerror(ret));
		exit(1);
	}
	printf("100 MX endpoint opened\n");
#endif
	ret = mx_get_info(0, MX_MAX_NATIVE_ENDPOINTS, 0, 0, &max_endpoints, sizeof(max_endpoints));
	if (ret != MX_SUCCESS) {
		fprintf(stderr, "Failed to do mx_get_info: %s\n", mx_strerror(ret));
		exit(1);
	}
	fprintf(stderr, "MX has %d maximum end-points configured\n", max_endpoints);
	ret = mx_get_info(0, MX_NIC_COUNT, 0, 0, &nics_available, sizeof(nics_available));
	if (ret != MX_SUCCESS) {
		fprintf(stderr, "Failed to do mx_get_info: %s\n", mx_strerror(ret));
		exit(1);
	}
	fprintf(stderr, "%d MX NICs available\n", nics_available);

	mx_ep_opened = 1;
	return max_endpoints | (nics_available << 8);
}

void close_mx() {
	//mx_close_endpoint(ep[0]);
	//mx_close_endpoint(ep[1]);
	mx_finalize();
}

#ifdef USE_MAIN
struct rmIpcStr gmDaqIpc[DCU_COUNT];
struct cdsDaqNetGdsTpNum gdsTpNumSpace[2][DCU_COUNT];
struct cdsDaqNetGdsTpNum * gdsTpNum[2][DCU_COUNT];
void *directed_receive_buffer[DCU_COUNT];
int _log_level = 10;
main() {
	for (int i = 0; i < 2; i++)
	  for (int j=0; j < DCU_COUNT; j++)
	    gdsTpNum[i][j] = &gdsTpNumSpace[i][j];

	// Allocate receive buffers for each configured DCU
	for (int i = 9; i < DCU_COUNT; i++) {
		//if (0 == daqd.dcuSize[0][i]) continue;

  		directed_receive_buffer[i] = malloc(2*DAQ_DCU_BLOCK_SIZE*DAQ_NUM_DATA_BLOCKS);
  		if (directed_receive_buffer[i] == 0) {
      			system_log (1, "[MX recv] Couldn't allocate recv buffer\n");
      			exit(1);
    		}
	}
	open_mx();
	receiver_mx(0);
}
#endif

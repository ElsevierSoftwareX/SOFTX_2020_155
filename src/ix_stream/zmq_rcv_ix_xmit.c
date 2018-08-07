//
///	@file zmq_rcv_ix_xmitcc
///	@brief  DAQ data concentrator code. Receives data via ZMQ and sends via Dolphin IX..
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
#include "../include/daqmap.h"
#include "../include/daq_core.h"
#include "dc_utils.h"

#include "sisci_types.h"
#include "sisci_api.h"
#include "sisci_error.h"
#include "sisci_demolib.h"
#include "testlib.h"

#include "zmq_transport.h"
#include "simple_pv.h"

#define __CDECL

#define DO_HANDSHAKE 0


#include "./dolphin_common.c"

#define MIN_DELAY_MS 5
#define MAX_DELAY_MS 40

extern void *findSharedMemorySize(char *,int);

int do_verbose = 0;
int debug_zmq = 0;

struct thread_info {
    int index;
    char *src_iface;
};
struct thread_mon_info {
    int index;
    void *ctx;
};
struct thread_info thread_index[DCU_COUNT];
void *daq_context[DCU_COUNT];
void *daq_subscriber[DCU_COUNT];
char *sname[DCU_COUNT];	// Names of FE computers serving DAQ data
char *local_iface[32];
daq_multi_dcu_data_t mxDataBlockSingle[32];
int64_t dataRecvTime[32];
const int mc_header_size = sizeof(daq_multi_cycle_header_t);
int stop_working_threads = 0;
int start_acq = 0;
static volatile int keepRunning = 1;
int thread_cycle[32];
int thread_timestamp[32];
int rcv_errors = 0;

void
usage()
{
	fprintf(stderr, "Usage: zmq_rcv_ix_xmit [args] -s server names -m shared memory size -g IX channel \n");
	fprintf(stderr, "-l filename - log file name\n");
	fprintf(stderr, "-b buffername - name of the mbuf to write to\n");
	fprintf(stderr, "-s - server names separated by spaces eg \"x1lsc0 x1susex etc.\"\n");
	fprintf(stderr, "-v - verbose prints diag test data\n");
	fprintf(stderr, "-g - Dolphin IX channel to xmit on\n");
	fprintf(stderr, "-p - Debug pv prefix, requires -P as well\n");
	fprintf(stderr, "-P - Path to a named pipe to send PV debug information to\n");
	fprintf(stderr, "-d - Max delay in milli seconds to wait for a FE to send data, defaults to 10\n");
	fprintf(stderr, "-L ifaces - local interfaces to listen on [used for load balancing], eg \"eth0 eth1\"\n");
    fprintf(stderr, "-z - output zmq debug information (state changes)\n");
	fprintf(stderr, "-h - help\n");
}

// *************************************************************************
// Timing Diagnostic Routine
// *************************************************************************
static int64_t
s_clock (void)
{
struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

// *************************************************************************
// Catch Control C to end cod in controlled manner
// *************************************************************************
void intHandler(int dummy) {
	keepRunning = 0;
}

void sigpipeHandler(int dummy)
{}

// **********************************************************************************************
void print_diags(int nsys, int lastCycle, int sendLength, daq_multi_dcu_data_t *ixDataBlock,int dbs[]) {
// **********************************************************************************************
	int ii = 0;
		// Print diags in verbose mode
		printf("Receive errors = %d\n",rcv_errors);
		printf("Time = %d\t size = %d\n",ixDataBlock->header.dcuheader[0].timeSec,sendLength);
		printf("DCU ID\tCycle \t TimeSec\tTimeNSec\tDataSize\tTPCount\tTPSize\tXmitSize\n");
		for(ii=0;ii<nsys;ii++) {
			printf("%d",ixDataBlock->header.dcuheader[ii].dcuId);
			printf("\t%d",ixDataBlock->header.dcuheader[ii].cycle);
			printf("\t%d",ixDataBlock->header.dcuheader[ii].timeSec);
			printf("\t%d",ixDataBlock->header.dcuheader[ii].timeNSec);
			printf("\t\t%d",ixDataBlock->header.dcuheader[ii].dataBlockSize);
	   		printf("\t\t%d",ixDataBlock->header.dcuheader[ii].tpCount);
	   		printf("\t%d",ixDataBlock->header.dcuheader[ii].tpBlockSize);
	   		printf("\t%d",dbs[ii]);
	   		printf("\n ");
		}
}

const char *pevent_zmq(int event, char *buffer, size_t max_size)
{
    if (!buffer || max_size < 1) {
        return "";
    }
    switch (event) {
        default:
            snprintf(buffer, max_size, "%d", event);
            break;
        case ZMQ_EVENT_CONNECTED:
            strncpy(buffer, "CONNECTED", max_size);   
            break;
        case ZMQ_EVENT_CONNECT_DELAYED:
            strncpy(buffer, "CONNECT_DELAYED", max_size);
            break;
        case ZMQ_EVENT_CONNECT_RETRIED:
            strncpy(buffer, "CONNECT_RETRIED", max_size);
            break;
        case ZMQ_EVENT_LISTENING:
            strncpy(buffer, "LISTENING", max_size);
            break;
        case ZMQ_EVENT_BIND_FAILED:
            strncpy(buffer, "BIND_FAILED", max_size);
            break;
        case ZMQ_EVENT_ACCEPTED:
            strncpy(buffer, "ACCEPTED", max_size);
            break;
        case ZMQ_EVENT_ACCEPT_FAILED:
            strncpy(buffer, "ACCEPT_FAILED", max_size);
            break;
        case ZMQ_EVENT_CLOSED:
            strncpy(buffer, "CLOSED", max_size);
            break;
        case ZMQ_EVENT_CLOSE_FAILED:
            strncpy(buffer, "CLOSE_FAILED", max_size);
            break;
        case ZMQ_EVENT_DISCONNECTED:
            strncpy(buffer, "DISCONNECTED", max_size);
            break;
        case ZMQ_EVENT_MONITOR_STOPPED:
            strncpy(buffer, "MONITOR_STOPPED", max_size);
            break;
    }
    buffer[max_size-1] = '\0';
    return buffer;
}

// *****
// monitoring thread callback
// *****
void *rcvr_thread_mon(void *args)
{
    int rc;
    char msg_buf[100];
    struct thread_mon_info *info = (struct thread_mon_info*)args;
    
    void *s = zmq_socket(info->ctx, ZMQ_PAIR);
    rc = zmq_connect(s, "inproc://monitor.req");
    while (1)
    {
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        rc = zmq_recvmsg(s, &msg, 0);
        if (rc == -1) break;
        unsigned short *event = (unsigned short*)zmq_msg_data(&msg);
        unsigned int *value = (unsigned int*)(event+1);
        fprintf(stderr, "%s) %d - %s\n", pevent_zmq(*event, msg_buf, sizeof(msg_buf)), *value, sname[info->index]);
        if (zmq_msg_more(&msg)) {
            zmq_msg_init(&msg);
            zmq_msg_recv(&msg, s, 0);
        }
    }
    zmq_close(s);
    free(args);
    return NULL;
}

// *************************************************************************
// Thread for receiving DAQ data via ZMQ
// *************************************************************************
void *rcvr_thread(void *arg) {
    struct thread_mon_info* mon_info = 0;
	struct thread_info* my_info = (struct thread_info*)arg;
	int mt = my_info->index;
	printf("myarg = %d on iface %s\n", mt, (my_info->src_iface ? my_info->src_iface : "default interface"));
	int cycle = 0;
	daq_multi_dcu_data_t *mxDataBlock;
    char loc[256];

    void *zctx = 0;
    void *zsocket = 0;
    int rc = 0;
    pthread_t mon_th_id;


    zctx = zmq_ctx_new();
    zsocket = zmq_socket(zctx, ZMQ_SUB);

    if (debug_zmq) {
        zmq_socket_monitor(zsocket, "inproc://monitor.req", ZMQ_EVENT_ALL);
        mon_info = calloc(1, sizeof(struct thread_mon_info));
        if (!mon_info) {
            fprintf(stderr, "Unable to initialize monitoring thread for %s\n", sname[mt]);
        } else {
            mon_info->index = mt;
            mon_info->ctx = zctx;
            pthread_create(&mon_th_id, NULL, rcvr_thread_mon, mon_info);
        }
    }

    rc = zmq_setsockopt(zsocket, ZMQ_SUBSCRIBE, "", 0);
    assert(rc == 0);
    if (!dc_generate_connection_string(loc, sname[mt], sizeof(loc), my_info->src_iface)) {
    	fprintf(stderr, "Unable to create connection string for '%s'\n", sname[mt]);
    	exit(1);
    }
	dc_set_zmq_options(zsocket);
    rc = zmq_connect(zsocket, loc);
    assert(rc == 0);


	printf("Starting receive loop for thread %d %s\n", mt, loc);
	char *daqbuffer = (char *)&mxDataBlockSingle[mt];
	mxDataBlock = (daq_multi_dcu_data_t *)daqbuffer;
	do {
        zmq_recv_daq_multi_dcu_t((daq_multi_dcu_data_t*)daqbuffer, zsocket);

		// Get the message DAQ cycle number
		cycle = mxDataBlock->header.dcuheader[0].cycle;
		// Pass cycle and timestamp data back to main process
		thread_cycle[mt] = cycle;
		thread_timestamp[mt] = mxDataBlock->header.dcuheader[0].timeSec;
        dataRecvTime[mt] = s_clock();

	// Run until told to stop by main thread
	} while(!stop_working_threads);
	printf("Stopping thread %d\n",mt);
	usleep(200000);

    zmq_close(zsocket);
    zmq_ctx_destroy(zctx);
	return(0);

}

// *************************************************************************
// Main Process
// *************************************************************************
int
main(int argc, char **argv)
{
	pthread_t thread_id[32];
	unsigned int nsys = 1; // The number of mapped shared memories (number of data sources)
	char *sysname;
	char *buffer_name = "ifo";
	int c;
	int ii;					// Loop counter
	int delay_ms = 10;
	int delay_cycles = 0;
	unsigned int niface = 0; // The number of local interfaces to split receives across.
    char *local_iface_names = 0;

	extern char *optarg;	// Needed to get arguments to program

	// PV/debug information
	char *pv_prefix = 0;
	char *pv_debug_pipe_name = 0;
	int pv_debug_pipe = -1;

	// Declare shared memory data variables
	daq_multi_cycle_header_t *ifo_header;
	char *ifo;
	char *ifo_data;
	int cycle_data_size;
	daq_multi_dcu_data_t *ifoDataBlock;
	char *nextData;
	int max_data_size_mb = 100;
	int max_data_size = 0;
	char *mywriteaddr;
	daq_multi_cycle_header_t *xmitHeader[IX_BLOCK_COUNT];
	static int xmitDataOffset[IX_BLOCK_COUNT];
	int xmitBlockNum = 0;

	/* set up defaults */
	sysname = NULL;
	int xmitData = 0;


	// Get arguments sent to process
	while ((c = getopt(argc, argv, "b:hs:m:g:vp:P:d:L:z")) != EOF) switch(c) {
	case 's':
		sysname = optarg;
		break;
	case 'v':
		do_verbose = 1;
		break;
    case 'z':
        debug_zmq = 1;
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
	case 'g':
            segmentId = atoi(optarg);
			xmitData = 1;
            break;
    case 'b':
        buffer_name = optarg;
        break;
	case 'p':
		pv_prefix = optarg;
		break;
	case 'P':
		pv_debug_pipe_name = optarg;
		break;
	case 'd':
		delay_ms = atoi(optarg);
		if (delay_ms < MIN_DELAY_MS || delay_ms > MAX_DELAY_MS) {
			printf("The delay factor must be between 5ms and 40ms\n");
			return -1;
		}
		break;
    case 'h':
    case 'L':
        local_iface_names = optarg;
        break;
	default:
		usage();
		exit(1);
	}
	max_data_size = max_data_size_mb * 1024*1024;
	delay_cycles = delay_ms * 10;

	if (sysname == NULL) { usage(); exit(1); }

	// set up to catch Control C
	signal(SIGINT,intHandler);
	// setup to ignore sig pipe
	signal(SIGPIPE, sigpipeHandler);

	printf("Server name: %s\n", sysname);

	// Parse names of data servers
	sname[0] = strtok(sysname, " ");
        for(;;) {
                printf("%s\n", sname[nsys - 1]);
                char *s = strtok(0, " ");
                if (!s) break;
				// do not overflow our fixed size buffers
				assert(nsys < DCU_COUNT);
                sname[nsys] = s;
                nsys++;
        }

    // Parse local interface names
    if (local_iface_names) {
        local_iface[0] = strtok(local_iface_names, " ");
        niface++;
        for (;;) {
            printf("local interface %s\n", local_iface[niface-1]);
            char *s = strtok(0, " ");
            if (!s) break;
            assert(niface < 32);
            local_iface[niface] = s;
            niface++;
        }
    }

	// Get pointers to local DAQ mbuf
    ifo = (char *)findSharedMemorySize(buffer_name,max_data_size_mb);
    ifo_header = (daq_multi_cycle_header_t *)ifo;
    ifo_data = (char *)ifo + sizeof(daq_multi_cycle_header_t);
	// Following line breaks daqd for some reason
	// max_data_size *= 1024 * 1024;
    cycle_data_size = (max_data_size - sizeof(daq_multi_cycle_header_t)) / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    cycle_data_size -= (cycle_data_size % 8);
	printf ("cycle data size = %d\t%d\n",cycle_data_size, max_data_size_mb);
	sleep(3);
	ifo_header->cycleDataSize = cycle_data_size;
    ifo_header->maxCycle = DAQ_NUM_DATA_BLOCKS_PER_SECOND;


	printf("nsys = %d\n",nsys);
	for(ii=0;ii<nsys;ii++) {
		printf("sys %d = %s\n",ii,sname[ii]);
	}

	// Make 0MQ socket connections
	for(ii=0;ii<nsys;ii++) {
		// Create a thread to receive data from each data server
		thread_index[ii].index = ii;
		thread_index[ii].src_iface = (niface ? local_iface[ii % niface]: 0);
		pthread_create(&thread_id[ii],NULL,rcvr_thread,(void *)&thread_index[ii]);
	}

	if(xmitData) {
		// Connect to Dolphin
		error = dolphin_init();
		printf("Read = 0x%lx \n Write = 0x%lx \n",(long)readAddr,(long)writeAddr);

		// Set pointer to xmit header in Dolphin xmit data area.
		mywriteaddr = (char *)writeAddr;
		for(ii=0;ii<IX_BLOCK_COUNT;ii++) {
			xmitHeader[ii] = (daq_multi_cycle_header_t *)mywriteaddr;
			mywriteaddr += IX_BLOCK_SIZE;
			xmitDataOffset[ii] = IX_BLOCK_SIZE * ii + sizeof(struct daq_multi_cycle_header_t);
		}
	}
	//

	int nextCycle = 0;
	start_acq = 1;
	int64_t mytime = 0;
	int64_t mylasttime = 0;
	int64_t myptime = 0;
	int64_t n_cycle_time = 0;
	int mytotaldcu = 0;
	char *zbuffer;
	size_t zbuffer_remaining = 0;
	int dc_datablock_size = 0;
	static const int header_size = sizeof(daq_multi_dcu_header_t);
	char dcstatus[2024];
	char dcs[24];
	int edcuid[10];
	int estatus[10];
	int edbs[10];
	unsigned long ets = 0;
	int timeout = 0;
	int dataRdy[32];
	int threads_rdy;
	int any_rdy = 0;
	int jj,kk;
	int sendLength = 0;

	int min_cycle_time = 1 << 30;
	int max_cycle_time = 0;
	int mean_cycle_time = 0;
	int pv_dcu_count = 0;
	int pv_total_datablock_size = 0;
	int endpoint_min_count = 1 << 30;
	int endpoint_max_count = 0;
	int endpoint_mean_count = 0;
	int cur_endpoint_ready_count;
	char endpoints_missed_buffer[256];
	int endpoints_missed_remaining;
	int missed_flag = 0;
	int missed_nsys[32];
    int64_t recv_time[32];
    int64_t min_recv_time = 0;
    int64_t cur_ref_time = 0;
    int recv_buckets[(MAX_DELAY_MS/5)+2];
    int entry_binned = 0;
	SimplePV pvs[] = {
            {
                    "RECV_BIN_MISS",
                    SIMPLE_PV_INT,
                    &recv_buckets[0],
                    1,
                    -1,
                    1,
                    -1,
            },
            {       
                    "RECV_BIN_0",
                    SIMPLE_PV_INT,
                    &recv_buckets[1],
                    1 << 30,
                    -1,
                    1 << 30,
                    -1,                                                                                  
            },
            {       
                    "RECV_BIN_1",
                    SIMPLE_PV_INT,
                    &recv_buckets[2],
                    1 << 30,
                    -1,
                    1 << 30,
                    -1,
            },
            {       
                    "RECV_BIN_2",
                    SIMPLE_PV_INT,
                    &recv_buckets[3],
                    1 << 30,
                    -1,
                    1 << 30,
                    -1,
            },
            {       
                    "RECV_BIN_3",
                    SIMPLE_PV_INT,
                    &recv_buckets[4],
                    1 << 30,
                    -1,
                    1 << 30,
                    -1,
            },
            {
                    "RECV_BIN_4",
                    SIMPLE_PV_INT,
                    &recv_buckets[5],
                    1 << 30,
                    -1,
                    1 << 30,
                    -1,
            },
            {       
                    "RECV_BIN_5",
                    SIMPLE_PV_INT,
                    &recv_buckets[6],
                    1 << 30,
                    -1,
                    1 << 30,
                    -1,
            },
            {       
                    "RECV_BIN_6",
                    SIMPLE_PV_INT,
                    &recv_buckets[7],
                    1 << 30,
                    -1,
                    1 << 30,
                    -1,
            },
            {      
                    "RECV_BIN_7",
                    SIMPLE_PV_INT,
                    &recv_buckets[8],
                    nsys,
                    -1,
                    nsys,
                    -1,
            },
            {
                    "RECV_BIN_8",
                    SIMPLE_PV_INT,
                    &recv_buckets[9],
                    1 << 30,
                    -1,
                    1 << 30,
                    -1,
            },
			{
					"RECV_MIN_MS",
                    SIMPLE_PV_INT,
					&min_cycle_time,

					80,
					45,
					70,
					54,
			},
			{
					"RECV_MAX_MS",
                    SIMPLE_PV_INT,
					&max_cycle_time,

					80,
					45,
					70,
					54,
			},
			{
					"RECV_MEAN_MS",
                    SIMPLE_PV_INT,
					&mean_cycle_time,

					80,
					45,
					70,
					54,
			},
			{
					"DCU_COUNT",
                    SIMPLE_PV_INT,
					&pv_dcu_count,

					120,
					0,
					115,
					0,
			},
			{
					"DATA_SIZE",
                    SIMPLE_PV_INT,
					&pv_total_datablock_size,

					100*1024*1024,
					0,
					90*1024*1024,
					1*1024*1024,
			},
			{
					"ENDPOINT_MIN_COUNT",
                    SIMPLE_PV_INT,
					&endpoint_min_count,

					32,
					0,
					30,
					1,
			},
			{
					"ENDPOINT_MAX_COUNT",
                    SIMPLE_PV_INT,
					&endpoint_max_count,

					32,
					0,
					30,
					1,
			},
			{
					"ENDPOINT_MEAN_COUNT",
                    SIMPLE_PV_INT,
					&endpoint_mean_count,

					32,
					0,
					30,
					1,
			},
            {
                "ENDPOINTS_MISSED",
                        SIMPLE_PV_STRING,
                        endpoints_missed_buffer,

                        0, 0, 0, 0,
            },

	};
	if (pv_debug_pipe_name)
	{
		pv_debug_pipe = open(pv_debug_pipe_name, O_NONBLOCK | O_RDWR, 0);
		if (pv_debug_pipe < 0) {
			fprintf(stderr, "Unable to open %s for writting (pv status)\n", pv_debug_pipe_name);
			exit(1);
		}
	}

	missed_flag = 1;
	memset(&missed_nsys[0], 0, sizeof(missed_nsys));
    memset(recv_buckets, 0, sizeof(recv_buckets));
	do {
		// Reset counters
		timeout = 0;
		for(ii=0;ii<nsys;ii++) dataRdy[ii] = 0;
		for(ii=0;ii<nsys;ii++) thread_cycle[ii] = 50;
		threads_rdy = 0;
		any_rdy = 0;

		// Wait up to 100ms until received data from at least 1 FE or timeout
		do {
			usleep(2000);
			for(ii=0;ii<nsys;ii++) {
				if(nextCycle == thread_cycle[ii]) any_rdy = 1;
			}
			timeout += 1;
		}while(!any_rdy && timeout < 50);

		// Wait up to delay_ms ms in 1/10ms intervals until data received from everyone or timeout
		timeout = 0;
		do {
			usleep(100);
			for(ii=0;ii<nsys;ii++) {
				if(nextCycle == thread_cycle[ii] && !dataRdy[ii]) threads_rdy ++;
				if(nextCycle == thread_cycle[ii]) dataRdy[ii] = 1;
			}
			timeout += 1;
		}while(threads_rdy < nsys && timeout < delay_cycles);
		if(timeout >= 100) rcv_errors += (nsys - threads_rdy);

		if(any_rdy) {
			// Timing diagnostics
			mytime = s_clock();
			myptime = mytime - mylasttime;
			mylasttime = mytime;
			if (myptime < min_cycle_time) {
				min_cycle_time = myptime;
			}
			if (myptime > max_cycle_time) {
				max_cycle_time = myptime;
			}
			mean_cycle_time += myptime;
			++n_cycle_time;
			if(do_verbose)printf("Data rdy for cycle = %d\t\t%ld\n",nextCycle,myptime);

			// Reset total DCU counter
			mytotaldcu = 0;
			// Reset total DC data size counter
			dc_datablock_size = 0;
			// Get pointer to next data block in shared memory
			nextData = (char *)ifo_data;
        	nextData += cycle_data_size * nextCycle;
        	ifoDataBlock = (daq_multi_dcu_data_t *)nextData;
			zbuffer = (char *)nextData + header_size;
			zbuffer_remaining = cycle_data_size - header_size;

			cur_endpoint_ready_count = 0;
			endpoints_missed_remaining = sizeof(endpoints_missed_buffer);
			endpoints_missed_buffer[0] = '\0';
            min_recv_time = 0x7fffffffffffffff;
			// Loop over all data buffers received from FE computers
			for(ii=0;ii<nsys;ii++) {
		  		recv_time[ii] = -1;
                if(dataRdy[ii]) {
                    recv_time[ii] = dataRecvTime[ii];
                    if (recv_time[ii] < min_recv_time) {
                        min_recv_time = recv_time[ii];
                    }
                    if (do_verbose && nextCycle == 0) {
                        fprintf(stderr, "+++%s\n", sname[ii]);
                    }
		  			++cur_endpoint_ready_count;
					int myc = mxDataBlockSingle[ii].header.dcuTotalModels;
					// For each model, copy over data header information
					for(jj=0;jj<myc;jj++) {
						// Copy data header information
						ifoDataBlock->header.dcuheader[mytotaldcu].dcuId = mxDataBlockSingle[ii].header.dcuheader[jj].dcuId;
						ifoDataBlock->header.dcuheader[mytotaldcu].fileCrc = mxDataBlockSingle[ii].header.dcuheader[jj].fileCrc;
						ifoDataBlock->header.dcuheader[mytotaldcu].status = mxDataBlockSingle[ii].header.dcuheader[jj].status;
						ifoDataBlock->header.dcuheader[mytotaldcu].cycle = mxDataBlockSingle[ii].header.dcuheader[jj].cycle;
						ifoDataBlock->header.dcuheader[mytotaldcu].timeSec = mxDataBlockSingle[ii].header.dcuheader[jj].timeSec;
						ifoDataBlock->header.dcuheader[mytotaldcu].timeNSec = mxDataBlockSingle[ii].header.dcuheader[jj].timeNSec;
						ifoDataBlock->header.dcuheader[mytotaldcu].dataCrc = mxDataBlockSingle[ii].header.dcuheader[jj].dataCrc;
						ifoDataBlock->header.dcuheader[mytotaldcu].dataBlockSize = mxDataBlockSingle[ii].header.dcuheader[jj].dataBlockSize;
						ifoDataBlock->header.dcuheader[mytotaldcu].tpBlockSize = mxDataBlockSingle[ii].header.dcuheader[jj].tpBlockSize;
						ifoDataBlock->header.dcuheader[mytotaldcu].tpCount = mxDataBlockSingle[ii].header.dcuheader[jj].tpCount;
						for(kk=0;kk<DAQ_GDS_MAX_TP_NUM ;kk++)	
							ifoDataBlock->header.dcuheader[mytotaldcu].tpNum[kk] = mxDataBlockSingle[ii].header.dcuheader[jj].tpNum[kk];
						edbs[mytotaldcu] = mxDataBlockSingle[ii].header.dcuheader[jj].tpBlockSize + mxDataBlockSingle[ii].header.dcuheader[jj].dataBlockSize;
						// Get some diags
						if(ifoDataBlock->header.dcuheader[mytotaldcu].status != 0xbad)
							ets = mxDataBlockSingle[ii].header.dcuheader[jj].timeSec;
						estatus[mytotaldcu] = ifoDataBlock->header.dcuheader[mytotaldcu].status;
						edcuid[mytotaldcu] = ifoDataBlock->header.dcuheader[mytotaldcu].dcuId;
						// Increment total DCU count
						mytotaldcu ++;
					}
					// Get the size of the data to transfer
					int mydbs = mxDataBlockSingle[ii].header.fullDataBlockSize;
					// Get pointer to data in receive data block
					char *mbuffer = (char *)&mxDataBlockSingle[ii].dataBlock[0];
					if (mydbs > zbuffer_remaining)
                    {
					    fprintf(stderr, "Buffer overflow found.  Attempting to write %d bytes to zbuffer which has %d bytes remainging\n",
                                (int)mydbs, (int)zbuffer_remaining);
					    abort();
                    }
					// Copy data from receive buffer to shared memory
					memcpy(zbuffer,mbuffer,mydbs);
					// Increment shared memory data buffer pointer for next data set
					zbuffer += mydbs;
					// Calc total size of data block for this cycle
					dc_datablock_size += mydbs;
		  		} else {
		  			missed_nsys[ii] |= missed_flag;
                    if (do_verbose && nextCycle == 0) {
                        fprintf(stderr, "---%s\n", sname[ii]);
                    } 
		  		}
			}
			if (cur_endpoint_ready_count < endpoint_min_count) {
				endpoint_min_count = cur_endpoint_ready_count;
			}
			if (cur_endpoint_ready_count > endpoint_max_count) {
				endpoint_max_count = cur_endpoint_ready_count;
			}
			endpoint_mean_count += cur_endpoint_ready_count;
			// Write total data block size to shared memory header
			ifoDataBlock->header.fullDataBlockSize = dc_datablock_size;
			// Write total dcu count to shared memory header
			ifoDataBlock->header.dcuTotalModels = mytotaldcu;
			// Set multi_cycle head cycle to indicate data ready for this cycle
			ifo_header->curCycle = nextCycle;
			xmitBlockNum = nextCycle % IX_BLOCK_COUNT;

			// Calc IX message size
			sendLength = header_size + ifoDataBlock->header.fullDataBlockSize;
            if (nextCycle == 0) {
                memset(recv_buckets, 0, sizeof(recv_buckets));
            }
            for (ii = 0; ii < nsys; ++ii) {
                cur_ref_time = 0;
                recv_time[ii] -= min_recv_time;
                entry_binned = 0;
                for (jj = 0; jj < sizeof(recv_buckets)/sizeof(recv_buckets[0]); ++jj) {
                    if (recv_time[ii] < cur_ref_time) {
                        ++recv_buckets[jj];
                        entry_binned = 1;
                        break;
                    }
                    cur_ref_time += 5;
                }
                if (!entry_binned) {
                    ++recv_buckets[sizeof(recv_buckets)/sizeof(recv_buckets[0])-1];
                }
            }
            if (nextCycle == 0) {
				pv_dcu_count = mytotaldcu;
				pv_total_datablock_size = dc_datablock_size;
				mean_cycle_time = (n_cycle_time > 0 ? mean_cycle_time / n_cycle_time : 1 << 31);
				endpoint_mean_count = (n_cycle_time > 0 ? endpoint_mean_count/n_cycle_time :  1<<31);

				endpoints_missed_remaining = sizeof(endpoints_missed_buffer)-1;
				endpoints_missed_buffer[0] = '\0';
				for (ii = 0; ii < sizeof(missed_nsys)/sizeof(missed_nsys[0]) && endpoints_missed_remaining > 0; ++ii) {
					if (missed_nsys[ii]) {
						char tmp[80];
						int count = snprintf(tmp, sizeof(tmp), "%s(%x) ", sname[ii], missed_nsys[ii]);
						if (count < sizeof(tmp)) {
							strncat(endpoints_missed_buffer, tmp, count);
							endpoints_missed_remaining -= count;
						}
					}
					missed_nsys[ii] = 0;
				}
				send_pv_update(pv_debug_pipe, pv_prefix, pvs, sizeof(pvs)/sizeof(pvs[0]));

				if (do_verbose) {
					printf("\nData rdy for cycle = %d\t\tTime Interval = %ld msec\n", nextCycle, myptime);
					printf("Min/Max/Mean cylce time %ld/%ld/%ld msec over %ld cycles\n", min_cycle_time, max_cycle_time,
						   mean_cycle_time, n_cycle_time);
					printf("Total DCU = %d\t\t\tBlockSize = %d\n", mytotaldcu, dc_datablock_size);
					print_diags(mytotaldcu, nextCycle, sendLength, ifoDataBlock, edbs);
				}
				n_cycle_time = 0;
				min_cycle_time = 1 << 30;
				max_cycle_time = 0;
				mean_cycle_time = 0;

                endpoint_min_count = nsys;
                endpoint_max_count = 0;
                endpoint_mean_count = 0;

				missed_flag = 1;
			} else {
				missed_flag <<= 1;
			}
			if(xmitData) {
			    if (sendLength > IX_BLOCK_SIZE)
                {
			        fprintf(stderr, "Buffer overflow.  Sending %d bytes into a dolphin block that holds %d\n",
                            (int)sendLength, (int)IX_BLOCK_SIZE);
			        abort();
                }
				// WRITEDATA to Dolphin Network
	    		SCIMemCpy(sequence,nextData, remoteMap,xmitDataOffset[xmitBlockNum],sendLength,memcpyFlag,&error);
        		if (error != SCI_ERR_OK) {
					fprintf(stderr,"SCIMemCpy failed - Error code 0x%x\n",error);
		    		return error;
				}
				// Set data header information
        		xmitHeader[xmitBlockNum]->maxCycle = ifo_header->maxCycle;
        		// xmitHeader->cycleDataSize = ifo_header->cycleDataSize;
        		xmitHeader[xmitBlockNum]->cycleDataSize = sendLength;;
        		// xmitHeader->msgcrc = myCrc;
        		// Send cycle last as indication of data ready for receivers
        		xmitHeader[xmitBlockNum]->curCycle = ifo_header->curCycle;
        		// Have to flush the buffers to make data go onto Dolphin network
        		SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
			}

		}
		sprintf(dcstatus,"%ld ",ets);
		for(ii=0;ii<mytotaldcu;ii++) {
			sprintf(dcs,"%d %d %d ",edcuid[ii],estatus[ii],edbs[ii]);
			strcat(dcstatus,dcs);
		}


		// Increment cycle count
		nextCycle ++;
		nextCycle %= 16;
	}while (keepRunning);	// End of infinite loop

	// Stop Rcv Threads
	printf("stopping threads %d \n",nsys);
	stop_working_threads = 1;

	// Wait for threads to stop
	sleep(2);
	if(xmitData) {
		printf("closing out ix\n");
		// Cleanup the Dolphin connections
		error = dolphin_closeout();
	}
	printf("closing out zmq\n");
	// Cleanup the ZMQ connections
	for(ii=0;ii<nsys;ii++) {
		if (daq_subscriber[ii]) zmq_close(daq_subscriber[ii]);
		if (daq_context[ii]) zmq_ctx_destroy(daq_context[ii]);
	}
  
	exit(0);
}

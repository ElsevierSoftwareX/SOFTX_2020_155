//
///	@file zmq_rcv_ix_xmitcc
///	@brief  DAQ data concentrator code. Receives data via ZMQ and sends via Dolphin IX..
//

#include <iostream>

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
#include <algorithm>
#include <vector>

#include "zmq_transport.h"
#include "simple_pv.h"

#include <stdarg.h>

#include "recvr_utils.hh"

#define __CDECL

#define DO_HANDSHAKE 0

const int delay_length = 4;

template <typename C>
void shift_left_one_and_fill(C& cont, const typename C::value_type fill_value)
{
	std::copy(cont.begin() + 1, cont.end(), cont.begin());
	cont.back() = fill_value;
}

#define MIN_DELAY_MS 5
#define MAX_DELAY_MS 40

#define MAX_FE_COMPUTERS 64

#define MAX_RECEIVE_BUFFERS 16

extern "C" {
extern volatile void *findSharedMemorySize(const char *, int);
}

int do_verbose = 0;
int debug_zmq = 0;
bool debug_wait_timings = false;

struct thread_info {
    int index;
    char *src_iface;
    void *zmq_ctx;
};
struct thread_mon_info {
    int index;
    void *ctx;
};

typedef struct mask_t {
	int data[DCU_COUNT];
} mask_t;

/// \brief structure to hold when data has been received
typedef struct receive_map_t
{
    receive_map_entry_t data[MAX_RECEIVE_BUFFERS][MAX_FE_COMPUTERS];	/// indexed by cycle then machine
} receive_map_t;

pthread_spinlock_t receive_map_lock;
receive_map_t receive_map;

static inline void mask_clear(mask_t* mask)
{
	memset(mask->data, 0, sizeof(mask->data));
}

static inline void mask_set_entry(mask_t* mask, int index)
{
	const unsigned int MAX_INDEX = sizeof(mask->data)/sizeof(mask->data[0]);
	if (((unsigned int)index) < MAX_INDEX)
	{
		mask->data[index] = 1;
	}
}

static inline int mask_get_entry(mask_t* mask, int index)
{
    const unsigned int MAX_INDEX = sizeof(mask->data)/sizeof(mask->data[0]);
    return (((unsigned int)index) < MAX_INDEX ? mask->data[index] : 0);
}

static inline int mask_diffs(mask_t* input1, mask_t* input2, mask_t* dest)
{
	const int MAX_INDEX = sizeof(input1->data)/sizeof(input1->data[0]);
	int diffs = 0;
	int i = 0;
	for(; i < MAX_INDEX; ++i)
	{
		diffs += (dest->data[i] = (input1->data[i] != input2->data[i]));
	}
	return diffs;
}

static inline int mask_differs(mask_t* input1, mask_t* input2)
{
	const int MAX_INDEX = sizeof(input1->data)/sizeof(input1->data[0]);
	int i = 0;
	for (; i < MAX_INDEX; ++i)
	{
		if (input1->data[i] != input2->data[i])
		{
			return 1;
		}
	}
	return 0;
}

static inline void mask_dump(mask_t* mask, const char* description)
{
	printf("%s\n", description);
	const int MAX_INDEX = sizeof(mask->data)/sizeof(mask->data[0]);
	int i = 0;
	for (; i < MAX_INDEX; ++i)
	{
		if (mask->data[i])
		{
			printf("(%d) ", i);
		}
	}
	printf("\n");
}

static inline void mask_copy(mask_t* dest, const mask_t* src)
{
	memcpy(dest, src, sizeof(*dest));
}

static void dump_dcu_mask_diff(mask_t* prev, mask_t* cur, int dcu_endpoint_lookup[DCU_COUNT])
{
	const int MAX_INDEX = sizeof(cur->data)/sizeof(cur->data[0]);
	if (!mask_differs(prev, cur))
	{
		return;
	}
	printf("\nDCU state change:\n");
	int i = 0;
	for (i = 0; i < MAX_INDEX; ++i)
	{
		if (prev->data[i] < cur->data[i])
		{
			printf("(+%d)e%d ", i, dcu_endpoint_lookup[i]);
		}
		else if (prev->data[i] > cur->data[i])
		{
			printf("(-%d)e%d ", i, dcu_endpoint_lookup[i]);
		}
	}
}

static void dump_endpoint_mask_diff(mask_t* prev, mask_t* cur)
{
	const int MAX_INDEX = sizeof(cur->data)/sizeof(cur->data[0]);
	if (!mask_differs(prev, cur))
	{
		return;
	}
	printf("\nEndpoint state change:\n");
	int i = 0;
	for (i = 0; i < MAX_INDEX; ++i)
	{
		if (prev->data[i] < cur->data[i])
		{
			printf("(+%d) ", i);
		}
		else if (prev->data[i] > cur->data[i])
		{
			printf("(-%d) ", i);
		}
	}
	printf("\n");
}

static void receive_map_init()
{
    memset(&receive_map, 0, sizeof(receive_map));
    pthread_spin_init(&receive_map_lock, PTHREAD_PROCESS_PRIVATE);
}

static void receive_map_mark(int fe_index, long gps_sec, int64_t cycle, int64_t clock_val)
{
	int64_t gps_sec_and_cycle = 0;

	int cycle_index = (int)(cycle % MAX_RECEIVE_BUFFERS);
    gps_sec_and_cycle = calc_gps_sec_and_cycle(gps_sec, cycle);

    lock_guard<pthread_spinlock_t> lock(receive_map_lock);
    receive_map.data[cycle_index][fe_index].gps_sec_and_cycle = gps_sec_and_cycle;
    receive_map.data[cycle_index][fe_index].s_clock = clock_val;
}

static void receive_map_copy_to_local(receive_map_t *dest)
{
	if (!dest)
	{
		return;
	}
	lock_guard<pthread_spinlock_t> lock_(receive_map_lock);
	memcpy(dest, &receive_map, sizeof(receive_map));
}

static void dump_recv_mask(FILE *f, receive_map_t* local_map, int nsys, int64_t gps_sec, int block_index, mask_t* received_mask, int current_cycle, int64_t current_clock)
{
    int fe_index = 0, cycle_index = 0;
    if (!f || !local_map || nsys < 1)
    {
        return;
    }
   // receive_map_copy_to_local(&local_map);
    fprintf(f, "----------------------------------------------\n");
    fprintf(f, "-- Sending Gps %ld:%d\n", (long int)gps_sec, (int)block_index);
    fprintf(f, "-- Current receive cycle %d   system time %ld\n", current_cycle, (long int)current_clock);
    fprintf(f, "----------------------------------------------\n");

    block_index %= MAX_RECEIVE_BUFFERS;

    for (cycle_index = MAX_RECEIVE_BUFFERS - 1; cycle_index >= 0 ; --cycle_index)
    {
        int64_t min_recv = static_cast<int64_t>(1)<<62;
        int64_t max_recv = 0;

        fprintf(f, "%2d) ", cycle_index);
        for (fe_index = 0; fe_index < nsys; ++fe_index)
        {
            int64_t receive_time = local_map->data[cycle_index][fe_index].s_clock;
            fprintf(f, "|%s%ld:%02d (%ld) ",
                    (cycle_index != block_index ? " " :
                    (mask_get_entry(received_mask, fe_index) ? " " : "*")),
                    (long int) local_map->data[cycle_index][fe_index].gps_sec_and_cycle >> 4,
                    0x0f & (int) local_map->data[cycle_index][fe_index].gps_sec_and_cycle,
                    (long int) receive_time);
            min_recv = (min_recv < receive_time ? min_recv : receive_time);
            max_recv = (max_recv > receive_time ? max_recv : receive_time);
        }
        fprintf(f, "| (min=%ld, max=%ld delta=%ld)\n", (long int)min_recv, (long int)max_recv, (long int)(max_recv - min_recv));
    }

    fprintf(f, "----------------------------------------------\n\n");
    fflush(f);
}

static void dump_local_recv_mask(receive_map_t& local_map, int nsys, int64_t expected_sec, int cycle)
{
    using std::cout;

    cout << "----------------------------------------------\n";
    cout << "-- cycle = " << cycle << " expected sec = "
        << extract_gps(expected_sec) << ":" << extract_cycle(expected_sec) << "\n";
    cout << "----------------------------------------------\n";

    int cycle_index = cycle % MAX_RECEIVE_BUFFERS;

    for (cycle_index = MAX_RECEIVE_BUFFERS - 1; cycle_index >= 0; --cycle_index)
    {
        cout << cycle_index << ") ";
        for (int fe_index = 0; fe_index < nsys; ++fe_index)
        {
            cout << "| " << extract_gps(local_map.data[cycle_index][fe_index].gps_sec_and_cycle)
                << ":" << extract_cycle(local_map.data[cycle_index][fe_index].gps_sec_and_cycle)
                << " (" << local_map.data[cycle_index][fe_index].s_clock << ") ";
        }
        cout << "|\n";
    }
}



struct thread_info thread_index[DCU_COUNT];
void *daq_context[DCU_COUNT];
void *daq_subscriber[DCU_COUNT];
char *sname[DCU_COUNT];	// Names of FE computers serving DAQ data
char *local_iface[MAX_FE_COMPUTERS];
//daq_multi_dcu_data_t mxDataBlockSingle[MAX_FE_COMPUTERS][MAX_RECEIVE_BUFFERS];
debug_array_2d<daq_multi_dcu_data_t, MAX_FE_COMPUTERS, MAX_RECEIVE_BUFFERS> mxDataBlockSingle;
//pthread_spinlock_t mxDataBlockSingle_lock[MAX_FE_COMPUTERS][MAX_RECEIVE_BUFFERS];
debug_array_2d<pthread_spinlock_t, MAX_FE_COMPUTERS, MAX_RECEIVE_BUFFERS> mxDataBlockSingle_lock;
const int mc_header_size = sizeof(daq_multi_cycle_header_t);
int stop_working_threads = 0;
int start_acq = 0;
static volatile int keepRunning = 1;
//int thread_cycle[MAX_FE_COMPUTERS];
//int thread_timestamp[MAX_FE_COMPUTERS];
int rcv_errors = 0;

void
usage()
{
	fprintf(stderr, "Usage: zmq_recv [args] -s server names -m shared memory size\n");
	fprintf(stderr, "-l filename - log file name\n");
	fprintf(stderr, "-b buffername - name of the mbuf to write to\n");
	fprintf(stderr, "-s - server names separated by spaces eg \"x1lsc0 x1susex etc.\"\n");
	fprintf(stderr, "-v - verbose prints diag test data\n");
	fprintf(stderr, "-p - Debug pv prefix, requires -P as well\n");
	fprintf(stderr, "-P - Path to a named pipe to send PV debug information to\n");
	fprintf(stderr, "-L ifaces - local interfaces to listen on [used for load balancing], eg \"eth0 eth1\"\n");
    fprintf(stderr, "-z - output zmq debug information (state changes)\n");
    fprintf(stderr, "-X path - output history of received data to given path\n");
    fprintf(stderr, "-D - output debug information on the wait code\n");
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
    delete info;
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

//    void *zctx = 0;
    void *zsocket = 0;
    int rc = 0;
    pthread_t mon_th_id;
    int buffer_index = 0;


//    zctx = zmq_ctx_new();
    zsocket = zmq_socket(my_info->zmq_ctx, ZMQ_SUB);

//    if (debug_zmq) {
//        zmq_socket_monitor(zsocket, "inproc://monitor.req", ZMQ_EVENT_ALL);
//        mon_info = new thread_mon_info; // calloc(1, sizeof(struct thread_mon_info));
//        memset((void*)mon_info, 0, sizeof(*mon_info));
//        if (!mon_info) {
//            fprintf(stderr, "Unable to initialize monitoring thread for %s\n", sname[mt]);
//        } else {
//            mon_info->index = mt;
//            mon_info->ctx = zctx;
//            pthread_create(&mon_th_id, NULL, rcvr_thread_mon, mon_info);
//        }
//    }

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
	char *daqbuffer = (char *)&mxDataBlockSingle[mt][0];
	mxDataBlock = (daq_multi_dcu_data_t *)daqbuffer;
	do {
		unsigned int gps_sec = 0;
        buffer_index = zmq_recv_daq_multi_dcu_t_into_buffer((daq_multi_dcu_data_t*)daqbuffer, mxDataBlockSingle_lock[mt], zsocket, MAX_RECEIVE_BUFFERS, &gps_sec, &cycle);

		// Get the message DAQ cycle number
		//cycle = mxDataBlock[buffer_index].header.dcuheader[0].cycle;
		// Pass cycle and timestamp data back to main process
		int64_t cur_time = s_clock();
        //receive_map_mark(mt, mxDataBlock[buffer_index].header.dcuheader[0].timeSec, cycle, cur_time);
        receive_map_mark(mt, gps_sec, cycle, cur_time);
        //printf("#### thread %d wrote %ld:%d at %ld  ####\n", (int)mt, (long int)mxDataBlock[buffer_index].header.dcuheader[0].timeSec, (int)cycle, (long int)cur_time);
	// Run until told to stop by main thread
	} while(!stop_working_threads);
	printf("Stopping thread %d\n",mt);
	usleep(200000);

    zmq_close(zsocket);
//    zmq_ctx_destroy(zctx);
	return(0);

}

void get_data_ready(int* data_ready, receive_map_t* data_ready_map, int nsys, int64_t target_gps_and_cycle)
{
	int index = static_cast<int>(extract_cycle(target_gps_and_cycle)) % MAX_RECEIVE_BUFFERS;

	receive_map_copy_to_local(data_ready_map);

	for (int i = 0; i < nsys; ++i)
	{
		data_ready[i] = data_ready_map->data[index][i].gps_sec_and_cycle >= target_gps_and_cycle;
	}
}

/**
 * Wait for the first system to provide data in the requested cycle, return its gps time + cycle
 * @param nsys
 * @param nextCycle
 * @param prev_sec_and_cycle
 * @param rcv_errors
 * @return
 */
int64_t
wait_for_first_system(int nsys, const int nextCycle, const int64_t prev_sec_and_cycle)
{
	int ii = 0;
	int wait_count = 0;
	int next_cycle = nextCycle;
	int64_t expected_sec_and_cycle = prev_sec_and_cycle; // calc_gps_sec_and_cycle(extract_gps(*prev_sec_and_cycle + 1), next_cycle);
	int64_t result = 0;
	receive_map_t data_ready_map;

	using namespace std;

	if (prev_sec_and_cycle == 0)
	{
		expected_sec_and_cycle = 0;
	}
	if (debug_wait_timings)
	{
        cout << "\n----- wait_for_first_system nsys="<<nsys<<" nextCycle="<<nextCycle<<" prev_sec="<<prev_sec_and_cycle<<"\n";
        cout << "expected_sec_and_cycle = " << expected_sec_and_cycle << "\n";
    }

    bool keep_waiting = true;
	do {
		receive_map_copy_to_local(&data_ready_map);

		int cycle_index = next_cycle % MAX_RECEIVE_BUFFERS;

		for (ii = 0; ii < nsys; ++ii) {
			// FIXME: what to do if it is a different second?
			// For the first time through expected_sec_and_cycle == 0, so this grabs
			// the first entry.
			// Otherwise we handle the case of ignoring old data (ie a front end dropped out)
			// but what about something that is running fast, or where one FE is signifigantly
			// off on its timing?
			if (data_ready_map.data[cycle_index][ii].gps_sec_and_cycle >= expected_sec_and_cycle
			&& data_ready_map.data[cycle_index][ii].gps_sec_and_cycle != 0
			&& extract_cycle(data_ready_map.data[cycle_index][ii].gps_sec_and_cycle) == next_cycle)
			{
				result = data_ready_map.data[cycle_index][ii].gps_sec_and_cycle;
				keep_waiting = false;
                if (debug_wait_timings) {
                    cout << "receivied system " << ii << " searching for cycle " << nextCycle << " prev = ";
                    cout << extract_gps(prev_sec_and_cycle) << ":" << extract_cycle(prev_sec_and_cycle);
                    cout << " found ";
                    cout << extract_gps(data_ready_map.data[cycle_index][ii].gps_sec_and_cycle);
                    cout << ":" << extract_cycle(data_ready_map.data[cycle_index][ii].gps_sec_and_cycle);
                    cout << " (" << data_ready_map.data[cycle_index][ii].s_clock << ")";
                    cout << "\n";
                }
                break;
			}
		}
		if (keep_waiting) {
			usleep(1000);       /* 1ms */
			if (debug_wait_timings) {
				cout << "<sleep 1ms>";
				++wait_count;
				if (wait_count > 0 && wait_count % 200 == 0) {
					std::cout << "<--- dumping receive map info for overlong wait -->\n";
					dump_local_recv_mask(data_ready_map, nsys, expected_sec_and_cycle, next_cycle);
				}
			}
		}
	} while (keep_waiting);
    if (debug_wait_timings) {
        cout << "exiting wait loop waited " << wait_count << "cycles\n---------------\n";
        dump_local_recv_mask(data_ready_map, nsys, expected_sec_and_cycle, next_cycle);
        cout << "\n\n";
        cout.flush();
    }
	return result;
}


// *************************************************************************
// Main Process
// *************************************************************************
int
main(int argc, char **argv)
{
	pthread_t thread_id[MAX_FE_COMPUTERS];
	unsigned int nsys = 1; // The number of mapped shared memories (number of data sources)
	char *sysname;
	const char *buffer_name = "ifo";
	int c;
	int ii, jj, kk;					// Loop counter
	unsigned int niface = 0; // The number of local interfaces to split receives across.
    char *local_iface_names = 0;

	extern char *optarg;	// Needed to get arguments to program

	// PV/debug information
	char *pv_prefix = 0;
	simple_pv_handle pcas_server = NULL;

	// Declare shared memory data variables
	daq_multi_cycle_header_t *ifo_header;
	char *ifo;
	char *ifo_data;
	int cycle_data_size;
	daq_multi_dcu_data_t *ifoDataBlock;
	char *nextData = NULL;
	int max_data_size_mb = 100;
	int max_data_size = 0;

	/* set up defaults */
	sysname = NULL;

    receive_map_init();
    for (ii = 0; ii < MAX_FE_COMPUTERS; ++ii)
    {
        for (jj = 0; jj < MAX_RECEIVE_BUFFERS; ++jj)
        {
            pthread_spin_init(&mxDataBlockSingle_lock[ii][jj], PTHREAD_PROCESS_PRIVATE);
        }
    }

    FILE* receive_map_dump_file = 0;
    int receive_map_dump_counter = 0;

	// Get arguments sent to process
	while ((c = getopt(argc, argv, "b:hs:m:vp:DL:zX:")) != EOF) switch(c) {
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
    case 'b':
        buffer_name = optarg;
        break;
    case 'p':
            pv_prefix = optarg;
            break;
    case 'h':
    case 'L':
        local_iface_names = optarg;
        break;
    case 'D':
	    debug_wait_timings = true;
	    break;
    case 'X':
        if (strcmp(optarg, "--") == 0)
		{
        	receive_map_dump_file = stderr;
		}
		else
		{
			receive_map_dump_file = fopen(optarg, "wt");
		}
        break;
	default:
		usage();
		exit(1);
	}
	max_data_size = max_data_size_mb * 1024*1024;

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
            assert(niface < MAX_FE_COMPUTERS);
            local_iface[niface] = s;
            niface++;
        }
    }

	// Get pointers to local DAQ mbuf
    ifo = reinterpret_cast<char *>(const_cast<void*>(findSharedMemorySize(buffer_name,max_data_size_mb)));
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

	void *zmq_ctx = zmq_ctx_new();
	if (!zmq_ctx)
    {
	    fprintf(stderr, "Unable to create zmq context for receiving, aborting...\n");
	    exit(1);
    }
    {
        int io_threads = 4;
        zmq_ctx_set(zmq_ctx, ZMQ_IO_THREADS, io_threads);
    }

	// Make 0MQ socket connections
	for(ii=0;ii<nsys;ii++) {
		// Create a thread to receive data from each data server
		thread_index[ii].index = ii;
		thread_index[ii].src_iface = (niface ? local_iface[ii % niface]: 0);
		thread_index[ii].zmq_ctx = zmq_ctx;
		pthread_create(&thread_id[ii],NULL,rcvr_thread,(void *)&thread_index[ii]);
	}

	//

	int nextCycle = 0;
	int64_t prev_sec_and_cycle = 0;
	start_acq = 1;
	int64_t mytime = 0;
	int64_t mylasttime = 0;
	int64_t myptime = 0;
	int64_t n_cycle_time = 0;
	int mytotaldcu = 0;
	char *zbuffer;
	size_t zbuffer_remaining = 0;
	int dc_datablock_size = 0;
    int datablock_size_running = 0;
    int datablock_size_mb_s = 0;
	static const int header_size = sizeof(daq_multi_dcu_header_t);
	int edcuid[10];
	int estatus[10];
	int edbs[10];
	unsigned long ets = 0;
	int dataRdy[MAX_FE_COMPUTERS];
	int sendLength = 0;

	std::vector<int64_t> send_queue(delay_length, -1);

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
	int missed_nsys[MAX_FE_COMPUTERS];
    //int64_t recv_time[MAX_FE_COMPUTERS];
    //int64_t min_recv_time = 0;
    int64_t cur_ref_time = 0;
    int recv_buckets[(MAX_DELAY_MS/5)+2];
    int entry_binned = 0;
	daq_multi_dcu_data_t* mx_cur_datablock = 0;

    mask_t dcu_received_mask;
    mask_t dcu_received_prev_mask;
    mask_t endpoint_received_mask;
    mask_t endpoint_received_prev_mask;

    int dcu_endpoint_lookup[DCU_COUNT];
    memset(dcu_endpoint_lookup, 0, sizeof(dcu_endpoint_lookup));

    mask_clear(&dcu_received_mask);
    mask_clear(&dcu_received_prev_mask);
    mask_clear(&endpoint_received_mask);
    mask_clear(&endpoint_received_prev_mask);

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
                    (int)nsys,
                    -1,
                    (int)nsys,
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

					MAX_FE_COMPUTERS,
					0,
					MAX_FE_COMPUTERS-2,
					1,
			},
			{
					"ENDPOINT_MAX_COUNT",
                    SIMPLE_PV_INT,
					&endpoint_max_count,

					MAX_FE_COMPUTERS,
					0,
					MAX_FE_COMPUTERS-2,
					1,
			},
			{
					"ENDPOINT_MEAN_COUNT",
                    SIMPLE_PV_INT,
					&endpoint_mean_count,

					MAX_FE_COMPUTERS,
					0,
					MAX_FE_COMPUTERS-2,
					1,
			},
            {
                    "DATA_BLOCK_SIZE",
                    SIMPLE_PV_INT,
                    &dc_datablock_size,

                    DAQ_DCU_BLOCK_SIZE,
                    0,
                    (DAQ_DCU_BLOCK_SIZE*9)/10,
                    1*1024*1024,

            },
            {
                    "DATA_BLOCK_SIZE_MB_PER_S",
                    SIMPLE_PV_INT,
                    &datablock_size_mb_s,

                    DAQ_TRANSIT_MAX_DC_BYTE_SEC/(1024*1024),
                    0,
                    (DAQ_TRANSIT_MAX_DC_BYTE_SEC/(1024*1024))*9/10,
                    1,
            },
            {
                "ENDPOINTS_MISSED",
                        SIMPLE_PV_STRING,
                        endpoints_missed_buffer,

                        0, 0, 0, 0,
            },
	};
	if (pv_prefix)
	{
	    pcas_server = simple_pv_server_create(pv_prefix, pvs, sizeof(pvs)/ sizeof(pvs[0]));
            if ( pcas_server == NULL )
            {
                fprintf(stderr, "Unable to create epics server\n");
                exit(1);
            }
	}

	missed_flag = 1;
	memset(&missed_nsys[0], 0, sizeof(missed_nsys));
    memset(recv_buckets, 0, sizeof(recv_buckets));

    receive_map_t receive_map;
	do {
		// reset masks
		mask_copy(&dcu_received_prev_mask, &dcu_received_mask);
		mask_copy(&endpoint_received_prev_mask, &endpoint_received_mask);
		mask_clear(&dcu_received_mask);
		mask_clear(&endpoint_received_mask);

		send_queue.back() = wait_for_first_system(nsys, nextCycle, prev_sec_and_cycle);

		if (send_queue.front() >= 0) {
			int sending_cycle = static_cast<int>(extract_cycle(send_queue.front()));
			int sending_index = sending_cycle % MAX_RECEIVE_BUFFERS;

			get_data_ready(dataRdy, &receive_map, nsys, send_queue.front());

			//std::cout << "sending " << send_queue.front() << " " << extract_gps(send_queue.front()) << " " << extract_cycle(send_queue.front());
			//std::cout << " next cycle = " << nextCycle << "\n";
			//std::cout << "data ready =";
			//for (ii = 0; ii < nsys; ++ii)
			//{
			//	if (dataRdy[ii])
			//	{
			//		std::cout << " " << ii;
			//	}
			//}
			//std::cout << std::endl;
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
			if(do_verbose)printf("Data rdy for cycle = %d\t\t%ld\n",sending_cycle,myptime);

			// Reset total DCU counter
			mytotaldcu = 0;
			// Reset total DC data size counter
			dc_datablock_size = 0;
			// Get pointer to next data block in shared memory
			nextData = (char *)ifo_data;
        	nextData += cycle_data_size * sending_cycle;
        	ifoDataBlock = (daq_multi_dcu_data_t *)nextData;
			zbuffer = (char *)nextData + header_size;
			zbuffer_remaining = cycle_data_size - header_size;

			cur_endpoint_ready_count = 0;
			endpoints_missed_remaining = sizeof(endpoints_missed_buffer);
			endpoints_missed_buffer[0] = '\0';
            //min_recv_time = 0x7fffffffffffffff;
			// Loop over all data buffers received from FE computers
			for(ii=0;ii<nsys;ii++) {
		  		//recv_time[ii] = -1;
                if(dataRdy[ii]) {
                	mask_set_entry(&endpoint_received_mask, ii);

                    //recv_time[ii] = dataRecvTime[ii];
                    //if (recv_time[ii] < min_recv_time) {
                    //    min_recv_time = recv_time[ii];
                    //}
                    if (do_verbose && sending_cycle == 0) {
                        fprintf(stderr, "+++%s\n", sname[ii]);
                    }
		  			++cur_endpoint_ready_count;
                    mx_cur_datablock = &mxDataBlockSingle[ii][sending_index];

                    lock_guard<pthread_spinlock_t> lock_(mxDataBlockSingle_lock[ii][sending_index]);
                    {
                        int myc = mx_cur_datablock->header.dcuTotalModels;
                        // For each model, copy over data header information
                        for (jj = 0; jj < myc; jj++) {
                            // Copy data header information
                            ifoDataBlock->header.dcuheader[mytotaldcu].dcuId = mx_cur_datablock->header.dcuheader[jj].dcuId;
                            ifoDataBlock->header.dcuheader[mytotaldcu].fileCrc = mx_cur_datablock->header.dcuheader[jj].fileCrc;
                            ifoDataBlock->header.dcuheader[mytotaldcu].status = mx_cur_datablock->header.dcuheader[jj].status;
                            ifoDataBlock->header.dcuheader[mytotaldcu].cycle = mx_cur_datablock->header.dcuheader[jj].cycle;
                            ifoDataBlock->header.dcuheader[mytotaldcu].timeSec = mx_cur_datablock->header.dcuheader[jj].timeSec;
                            ifoDataBlock->header.dcuheader[mytotaldcu].timeNSec = mx_cur_datablock->header.dcuheader[jj].timeNSec;
                            ifoDataBlock->header.dcuheader[mytotaldcu].dataCrc = mx_cur_datablock->header.dcuheader[jj].dataCrc;
                            ifoDataBlock->header.dcuheader[mytotaldcu].dataBlockSize = mx_cur_datablock->header.dcuheader[jj].dataBlockSize;
                            ifoDataBlock->header.dcuheader[mytotaldcu].tpBlockSize = mx_cur_datablock->header.dcuheader[jj].tpBlockSize;
                            ifoDataBlock->header.dcuheader[mytotaldcu].tpCount = mx_cur_datablock->header.dcuheader[jj].tpCount;

                            mask_set_entry(&dcu_received_mask, ifoDataBlock->header.dcuheader[mytotaldcu].dcuId);
                            dcu_endpoint_lookup[ifoDataBlock->header.dcuheader[mytotaldcu].dcuId] = ii;

                            for (kk = 0; kk < DAQ_GDS_MAX_TP_NUM; kk++)
                                ifoDataBlock->header.dcuheader[mytotaldcu].tpNum[kk] = mx_cur_datablock->header.dcuheader[jj].tpNum[kk];
                            edbs[mytotaldcu] = mx_cur_datablock->header.dcuheader[jj].tpBlockSize +
                                               mx_cur_datablock->header.dcuheader[jj].dataBlockSize;
                            // Get some diags
                            if (ifoDataBlock->header.dcuheader[mytotaldcu].status != 0xbad)
                                ets = mx_cur_datablock->header.dcuheader[jj].timeSec;
                            estatus[mytotaldcu] = ifoDataBlock->header.dcuheader[mytotaldcu].status;
                            edcuid[mytotaldcu] = ifoDataBlock->header.dcuheader[mytotaldcu].dcuId;
                            // Increment total DCU count
                            mytotaldcu++;
                        }
                        // Get the size of the data to transfer
                        int mydbs = mx_cur_datablock->header.fullDataBlockSize;
                        // Get pointer to data in receive data block
                        char *mbuffer = (char *) &mx_cur_datablock->dataBlock[0];
                        if (mydbs > zbuffer_remaining) {
                            fprintf(stderr,
                                    "Buffer overflow found.  Attempting to write %d bytes to zbuffer which has %d bytes remainging\n",
                                    (int) mydbs, (int) zbuffer_remaining);
                            abort();
                        }
                        // Copy data from receive buffer to shared memory
                        memcpy(zbuffer, mbuffer, mydbs);
                        // Increment shared memory data buffer pointer for next data set
                        zbuffer += mydbs;
                        // Calc total size of data block for this cycle
                        dc_datablock_size += mydbs;
                    }
                } else {
		  			missed_nsys[ii] |= missed_flag;
                    if (do_verbose && sending_cycle == 0) {
                        fprintf(stderr, "---%s\n", sname[ii]);
                    }
                    if (receive_map_dump_file)
                    {
                        receive_map_dump_counter = 3;
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
			ifo_header->curCycle = sending_cycle;

			// Calc IX message size
			sendLength = header_size + ifoDataBlock->header.fullDataBlockSize;
            if (sending_cycle == 0) {
                memset(recv_buckets, 0, sizeof(recv_buckets));
            }
            /*for (ii = 0; ii < nsys; ++ii) {
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
            }*/
            datablock_size_running += dc_datablock_size;
            if (sending_cycle == 0) {
                datablock_size_mb_s = datablock_size_running / (1024*1024);
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
				simple_pv_server_update(pcas_server);

				if (do_verbose) {
					printf("\nData rdy for cycle = %d\t\tTime Interval = %ld msec\n", sending_cycle, myptime);
					printf("Min/Max/Mean cylce time %ld/%ld/%ld msec over %ld cycles\n", (long int)min_cycle_time, (long int)max_cycle_time,
                                               (long int)mean_cycle_time, (long int)n_cycle_time);
					printf("Total DCU = %d\t\t\tBlockSize = %d\n", mytotaldcu, dc_datablock_size);
					print_diags(mytotaldcu, sending_cycle, sendLength, ifoDataBlock, edbs);
				}
				n_cycle_time = 0;
				min_cycle_time = 1 << 30;
				max_cycle_time = 0;
				mean_cycle_time = 0;

                endpoint_min_count = nsys;
                endpoint_max_count = 0;
                endpoint_mean_count = 0;

				missed_flag = 1;
                datablock_size_running = 0;
			} else {
				missed_flag <<= 1;
			}

            if (receive_map_dump_counter)
            {
                dump_recv_mask(receive_map_dump_file, &receive_map, nsys, ifoDataBlock->header.dcuheader[0].timeSec, sending_cycle, &endpoint_received_mask, nextCycle, s_clock());
                --receive_map_dump_counter;
            }

			dump_dcu_mask_diff(&dcu_received_prev_mask, &dcu_received_mask, dcu_endpoint_lookup);
			dump_endpoint_mask_diff(&endpoint_received_prev_mask, &endpoint_received_mask);
			//std::cout << "Total DCUS " << mytotaldcu << "\n";
		}
		prev_sec_and_cycle = send_queue.back();
		shift_left_one_and_fill(send_queue, -1);

		// Increment cycle count
		nextCycle ++;
		nextCycle %= 16;
	}while (keepRunning);	// End of infinite loop

	// Stop Rcv Threads
	printf("stopping threads %d \n",nsys);
	stop_working_threads = 1;

	// Wait for threads to stop
	sleep(2);

	printf("closing out zmq\n");
	// Cleanup the ZMQ connections
	for(ii=0;ii<nsys;ii++) {
		if (daq_subscriber[ii]) zmq_close(daq_subscriber[ii]);
		if (daq_context[ii]) zmq_ctx_destroy(daq_context[ii]);
	}

	for (ii = 0; ii < MAX_FE_COMPUTERS; ++ii)
    {
	    for (jj = 0; jj < MAX_RECEIVE_BUFFERS; ++jj)
        {
	        pthread_spin_destroy(&mxDataBlockSingle_lock[ii][jj]);
        }
    }
    zmq_ctx_destroy(zmq_ctx);
    simple_pv_server_destroy( &pcas_server );
    exit(0);
}

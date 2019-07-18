// Myrinet MX DQ data receiver
//#define MX_THREAD_SAFE 1
//#include "config.h"
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
#include <iostream>
#include "../include/daqmap.h"
//#include "debug.h"
//#include "daqd.hh"
//#include "mx_rcvr.hh"
#include "gps.hh"

const int DEFAULT_THREAD_SAFE_VALUE = true;
const int DEFAULT_CLOCK_OFFSET_VALUE = 0;
const int DEFAULT_CYCLE_DELAY_VALUE = 0;
const int DEFAULT_TRAIL_CYCLE_MS_VALUE = 20;

/***
 * A simple atomic counter that should be implemented lock free.
 * This uses compiler builtins that clang & gcc recognize.
 * This is suitable for sharing between threads.
 */
template <typename T>
class atomic_counter {
public:
    typedef T value_type;

    atomic_counter(): val_(0) {}

    void increment()
    {
        __sync_add_and_fetch(&val_, 1);
    }
    value_type get()
    {
        return __sync_add_and_fetch(&val_, 0);
    }
    void clear()
    {
        __sync_and_and_fetch(&val_, 0);
    }
private:
    atomic_counter(const atomic_counter& other);
    atomic_counter& operator=(const atomic_counter& other);

    T val_;
};


template <typename T>
class simple_guard {
public:
    explicit simple_guard(T& lock): lock_(lock) { lock_.lock(); }
    ~simple_guard() { lock_.unlock(); }
private:
    simple_guard(const simple_guard& other);
    simple_guard operator=(const simple_guard& other);
    T& lock_;
};

template <typename T>
class noop_guard {
public:
    explicit noop_guard(T& t) {}
};

class base_lock {
public:
    base_lock(): l_()
    {
        pthread_spin_init(&l_, PTHREAD_PROCESS_PRIVATE);
    }
    ~base_lock()
    {
        pthread_spin_destroy(&l_);
    }
    void lock()
    {
        pthread_spin_lock(&l_);
    }
    void unlock()
    {
        pthread_spin_unlock(&l_);
    }
private:
    base_lock(const base_lock& other);
    base_lock operator=(const base_lock& other);
    pthread_spinlock_t l_;
};

/// Define max boards, endpoints for mx_rcvr
#define MX_MAX_BOARDS 4
#define MX_MAX_ENDPOINTS 32

//struct rmIpcStr gmDaqIpc[DCU_COUNT];
//struct cdsDaqNetGdsTpNum gdsTpNumSpace[2][DCU_COUNT];
//struct cdsDaqNetGdsTpNum * gdsTpNum[2][DCU_COUNT];
void *directed_receive_buffer[DCU_COUNT];
int _log_level = 10;

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

struct MXdata {
    MXdata(): data(), lock_() { memset((void*)&data, 0, sizeof(data)); }

    daqMXdata data;

    void lock() { lock_.lock(); }
    void unlock() { lock_.unlock(); }
private:
    base_lock lock_;
};

typedef simple_guard<MXdata> MXLockGuard;
typedef noop_guard<MXdata> MXNoopGuard;
MXdata received_data[DCU_COUNT];
atomic_counter<long> mx_msg_counter;

static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;

template <typename LockGuardT>
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
    mx_endpoint_t ep[MX_MAX_BOARDS*MX_MAX_ENDPOINTS];

    // Set thread parameters
    char my_thr_label[16];
    snprintf(my_thr_label,16,"dqmx%03x",neid);
    char my_thr_name[40];
    snprintf(my_thr_name,40,"mx receiver %03x",neid);
    //daqd_c::set_thread_priority(my_thr_name, my_thr_label, MX_THREAD_PRIORITY,0);

    uint32_t board_num = (neid >> 8);
    uint32_t ep_num = neid & 0xff;

    mx_return_t ret = mx_open_endpoint(board_num, ep_num, FILTER, NULL, 0, &ep[neid]);
    if (ret != MX_SUCCESS) {
        fprintf(stderr, "Failed to open board %d endpoint %d error %s\n", board_num, ep_num, mx_strerror(ret));
        exit(1);
    }

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
                fprintf(stderr, "mx_cancel() eid=%03x failed with status %s\n", neid, mx_strerror(ret));
                exit(1); // Not clear what to do in this case; this indicates shortage of memory or resources
            }
            if (result == 0) { // Request incomplete
                goto again; // Restart incomplete request
            } else { // Request is complete
                if (stat.code != MX_STATUS_SUCCESS) { // Request completed, but bad code
                    fprintf(stderr, "mx_wait failed in rcvr eid=%03x, reqn=%d; wait did not complete; status code is %s\n",
                            neid, count, mx_strstatus(stat.code));
                    //mx_return_t ret = mx_cancel(ep[neid], &req[cur_req], &result);
                    mx_endpoint_addr_t ep_addr;
                    ret = mx_get_endpoint_addr(ep[neid], &ep_addr);
                    if (ret != MX_SUCCESS) {
                        fprintf(stderr, "mx_get_endpoint_addr() eid=%03x failed with status %s\n", neid, mx_strerror(ret));
                    } else {
                        ret = mx_disconnect(ep[neid], ep_addr);
                        if (ret != MX_SUCCESS) {
                            fprintf(stderr, "mx_disconnect() eid=%03x failed with status %s\n", neid, mx_strerror(ret));
                        } else {
                            fprintf(stderr, "disconnected from the sender on endpoint %03x\n", neid);
                        }
                    }
                    myErrorStat = 1;
                }
            }
            // Fall through if the request is complete and the code is success
            //

            copySize = stat.xfer_length;
            // seg.segment_ptr = buffer[cur_req];
            seg.segment_ptr = &buffer[cur_req * len];
            seg.segment_length = len;

            if (!myErrorStat) {
                //printf("received one\n");
                struct daqMXdata *dataPtr = (struct daqMXdata *) seg.segment_ptr;
                int dcu_id = dataPtr->mxIpcData.dcuId;

                if (dcu_id < 0 || dcu_id > (DCU_COUNT-1)) {
                    mx_irecv(ep[neid], &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, &req[cur_req]);
                    continue; // Bad DCU ID
                }
//                if (daqd.dcuSize[0][dcu_id] == 0) {
//                    mx_irecv(ep[neid], &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, &req[cur_req]);
//                    continue; // Unconfigured DCU
//                }

//                int cycle = dataPtr->mxIpcData.cycle;
//                int len = dataPtr->mxIpcData.dataBlockSize;
//
                {
                    LockGuardT g_(received_data[dcu_id]);
                    received_data[dcu_id].data = *dataPtr;
                }
                mx_msg_counter.increment();
//                if (dcu_id == 7)
//                {
//                    int cycle = dataPtr->mxIpcData.cycle;
//                    fprintf(stderr, "rcv >>> id(%d) %d bp: %d %d:%d (%d %d bytes)\n",
//                            (int)dcu_id,
//                            (int)(dataPtr->mxIpcData.cycle),
//                            (int)(dataPtr->mxIpcData.bp[cycle].cycle),
//                            (int)(dataPtr->mxIpcData.bp[cycle].timeSec),
//                            (int)(dataPtr->mxIpcData.bp[cycle].timeNSec),
//                            (int)copySize,
//                            (int)len
//                    );
//                }

//                char *dataDest = (char *)((char *)(directed_receive_buffer[dcu_id]) + buf_size * cycle);
//
//                // Move the block data into the buffer
//                memcpy (dataDest, dataSource, len);
//
//                // Assign IPC data
//                gmDaqIpc[dcu_id].crc = dataPtr->mxIpcData.crc;
//                gmDaqIpc[dcu_id].dcuId = dataPtr->mxIpcData.dcuId;
//                gmDaqIpc[dcu_id].bp[cycle].timeSec = dataPtr->mxIpcData.bp[cycle].timeSec;
//                gmDaqIpc[dcu_id].bp[cycle].crc = dataPtr->mxIpcData.bp[cycle].crc;
//                gmDaqIpc[dcu_id].bp[cycle].cycle = dataPtr->mxIpcData.bp[cycle].cycle;
//                gmDaqIpc[dcu_id].dataBlockSize = dataPtr->mxIpcData.dataBlockSize;
//
//                fprintf(stderr, "dcu %d gps %d %d %d\n", dcu_id, dataPtr->mxIpcData.bp[cycle].timeSec, cycle, dataPtr->mxIpcData.bp[cycle].timeNSec);
//
//                // Assign test points table
//                //*gdsTpNum[0][dcu_id] = dataPtr->mxTpTable;
//
//                gmDaqIpc[dcu_id].cycle = cycle;
//#ifndef USE_MAIN
//#ifndef  USE_SYMMETRICOM
//                if (daqd.controller_dcu == dcu_id)  {
//                    controller_cycle = cycle;
//                    DEBUG(6, printf("Timing dcu=%d cycle=%d\n", dcu_id, controller_cycle));
//                }
//#endif
//#endif
//                daqd.producer1.rcvr_stats[dcu_id].tick();
            }
            //daqd.producer1.rcvr_stats[neid].tick();
            mx_irecv(ep[neid], &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, &req[cur_req]);
        }
    } while(1);
    gettimeofday(&end_time, NULL);
    fprintf(stderr, "mx_wait failed in rcvr after do loop\n");
    free(buffer);
    //exit(1);
}

void *
receiver_thread_safe(void *args)
{
    receiver_mx<MXLockGuard>(*reinterpret_cast<int*>(args));
    return NULL;
}

void *
receiver_UNSAFE(void *args)
{
    receiver_mx<MXNoopGuard>(*reinterpret_cast<int*>(args));
    return NULL;
}

int mx_ep_opened = 0;

struct mx_info {
    uint32_t max_endpoints;
    uint32_t nics_available;
};

/// Initialize MX library.
/// Returns the maximum number of end-points supported in the system.
mx_info
open_mx(void) {
    mx_info results;
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

    static char omx_errors_env[] = "OMX_ERRORS_ARE_FATAL=0";
    static char mx_errors_env[] = "MX_ERRORS_ARE_FATAL=0";

    if (!mx_ep_opened) {
        printf("%ld\n", sizeof(struct daqMXdata));

        // So that openmx is not aborting on connection loss
        putenv(omx_errors_env);
        putenv(mx_errors_env);

        mx_init();

        /* set up defaults */
        sysname = NULL;
        filter = FILTER;
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
    /// make sure these don't exceed array allocations
        if (max_endpoints > MX_MAX_ENDPOINTS) {
            fprintf(stderr, "ERROR: max end-points of %d exceeds array limit of %d\n", max_endpoints, MX_MAX_ENDPOINTS);
            exit(1);
        }
        if (nics_available > MX_MAX_BOARDS) {
            fprintf(stderr, "ERROR: available nics of %d exceeds array limit of %d\n", nics_available, MX_MAX_BOARDS);
            exit(1);
        }

        mx_ep_opened = 1;
    }
    results.nics_available = nics_available;
    results.max_endpoints = max_endpoints;
    return results;
}

void short_sleep(unsigned long sleep_ms = 10)
{
    struct timespec wait = {0, 0};
    wait.tv_nsec = 1000000UL * sleep_ms;
    nanosleep(&wait, NULL);
}

void wait_for_mx_messages()
{
    while (mx_msg_counter.get() == 0)
    {
        short_sleep();
    }
}

GPS::gps_time
wait_for_end_of_cycle(GPS::gps_clock& clock, int cycle, int cycle_delay) {
    const unsigned int c = 1000000000 / 16;

    int target_cycle = (cycle + cycle_delay + 1) % 16;

    long lower_nano = target_cycle * c;
    long upper_nano = lower_nano + c;

    //fprintf(stderr, "wfeoc %d %d - waiting for %d %ld %ld\n", cycle, cycle_delay, target_cycle, lower_nano, upper_nano);

    GPS::gps_time now = clock.now();
    while (now.nanosec < lower_nano || now.nanosec > upper_nano)
    {
        short_sleep();
        now = clock.now();
    }
    return now;
}

void close_mx() {
    mx_finalize();
}

template <typename LockGuardT>
void
concentrate_loop(GPS::gps_clock& clock, int cycle_delay, int trail_cycle_ms)
{
    int cur_cycle = 0;
    bool timing_glitch[DCU_COUNT];

    for (int i = 0; i < DCU_COUNT; ++i)
    {
        timing_glitch[i] = true;
    }

    std::cout << "Waiting for MX messages to start ...\n";
    wait_for_mx_messages();
    std::cout << "MX messages have been received, will concentrate data at the start of the next second\n";

    for (int cur_cycle = 0;; cur_cycle = (cur_cycle + 1) % 16)
    {
        GPS::gps_time now = wait_for_end_of_cycle(clock, cur_cycle, cycle_delay);
        short_sleep(trail_cycle_ms);
        long expected_gps = now.sec;
        if (cur_cycle >= (15 - cycle_delay))
        {
            --expected_gps;
        }
        bool need_nl = false;
        for (int cur_dcu = 0; cur_dcu < DCU_COUNT; ++cur_dcu)
        {
            unsigned int timeSec = 0;
            unsigned int timeNSec = 0;
            unsigned int cycle =0 ;
            {
                LockGuardT g_(received_data[cur_dcu]);
                cycle = received_data[cur_dcu].data.mxIpcData.cycle;
                timeSec = received_data[cur_dcu].data.mxIpcData.bp[cur_cycle].timeSec;
                timeNSec = received_data[cur_dcu].data.mxIpcData.bp[cur_cycle].timeNSec;
            }
//            if (cur_dcu == 7)
//            {
//                std::cout << "now (" << now.sec << ":" << now.nanosec << ") cycle: " << cur_cycle;
//                std::cout << " received cycle - " << cycle << " " << timeSec << ":" << timeNSec << " exp: " << expected_gps << std::endl;
//            }
            if (expected_gps == timeSec)
            {
                if (timing_glitch[cur_dcu])
                {
                    std::cout << " +(" << cur_dcu << ")";
                    need_nl = true;
                }
                timing_glitch[cur_dcu] = false;
            }
            else
            {
                if (!timing_glitch[cur_dcu])
                {
                    std::cout << " -(" << cur_dcu << ")";
                    need_nl = true;
                }
                timing_glitch[cur_dcu] = true;
            }
        }
        if (need_nl)
        {
            std::cout << "\n";
        }
    }
}

struct options_t
{
    options_t(): thread_safe(DEFAULT_THREAD_SAFE_VALUE),
    clock_offset(DEFAULT_CLOCK_OFFSET_VALUE),
    cycle_delay(DEFAULT_CYCLE_DELAY_VALUE),
    trail_cycle_ms(DEFAULT_TRAIL_CYCLE_MS_VALUE),
    abort(false) {}

    bool thread_safe;
    int clock_offset;
    int cycle_delay;
    int trail_cycle_ms;
    bool abort;
};

void show_help(const char* prog)
{
    std::cout << "Usage:" << prog << " options\n\nWhere options are:\n";
    std::cout << "\t-h - Show this help\n";
    std::cout << "\t-S - Use the safe thread code [Default]\n";
    std::cout << "\t-U - Use the UNSAFE thread code (match the daqd code)\n";
    std::cout << "\t-G <offset> - Offset to apply to the gps clock [Default=" << DEFAULT_CLOCK_OFFSET_VALUE << "]\n";
    std::cout << "\t-c <cyle delay> - Cycle delay value [Default=" << DEFAULT_CYCLE_DELAY_VALUE << "]\n";
    std::cout << "\t-t <ms> - Wait for the given ms after each cycle to start concentrating the data [Default=" << DEFAULT_TRAIL_CYCLE_MS_VALUE << "]\n";
    std::cout << std::endl;
}

options_t parse_options(int argc, char *const *argv) {
    options_t opts;
    int arg = 0;
    while ((arg = getopt(argc, argv, "SUG:c:t:h")) != EOF)
    {
        switch (arg) {
            case 'S':
                opts.thread_safe = 1;
                break;
            case 'U':
                opts.thread_safe = 0;
                break;
            case 'G':
                opts.clock_offset = atoi(optarg);
                break;
            case 'c':
                opts.cycle_delay = atoi(optarg);
                break;
            case 't':
                opts.trail_cycle_ms = atoi(optarg);
                break;
            case 'h':
            default:
                opts.abort = true;
        }
    }
    if (opts.abort)
    {
        show_help(argv[0]);
    }
    return opts;
}

int main(int argc, char* argv[]) {
    options_t opts = parse_options(argc, argv);

    if (opts.abort)
    {
        exit(1);
    }
    GPS::gps_clock clock(opts.clock_offset);
    if (!clock.ok())
    {
        fprintf(stderr, "This program requires a LIGO gps time device to be present.");
        exit(1);
    }


	// Allocate receive buffers for each configured DCU
	for (int i = 0; i < DCU_COUNT; i++) {
	    //if (0 == daqd.dcuSize[0][i]) continue;

  		directed_receive_buffer[i] = malloc(2*DAQ_DCU_BLOCK_SIZE*DAQ_NUM_DATA_BLOCKS);
  		if (directed_receive_buffer[i] == 0) {
      			fprintf (stderr, "[MX recv] Couldn't allocate recv buffer\n");
      			exit(1);
    		}
	}
	mx_info info = open_mx();

    int bp_aray[MX_MAX_BOARDS][MX_MAX_ENDPOINTS];
    pthread_t gm_tid[MX_MAX_BOARDS][MX_MAX_ENDPOINTS];

    for (int bnum = 0; bnum < info.nics_available; bnum++) { // Start
        for (int j = 0; j < info.max_endpoints; j++) {
            int bp = j;
            bp = bp + (bnum * 256);
            /* calculate address within array */
            bp_aray[bnum][j] = bp;
            //void *bpPtr = (int *)(bp_aray + bnum) + j;
            void *bpPtr = (void *)(&bp_aray[bnum][j]);
            int my_err_no = 0;

            std::cout << "setup for board " << bnum << " ep " << j << " *bpPtr = " << *(int*)bpPtr << " bpPtr = " << bpPtr << "\n";

            my_err_no =
                    pthread_create(&gm_tid[bnum][j], NULL, (opts.thread_safe ? receiver_thread_safe : receiver_UNSAFE), bpPtr);
            if (my_err_no)
            {
                char errmsgbuf[80];
                strerror_r(my_err_no, errmsgbuf, sizeof(errmsgbuf));
                fprintf(stderr, "pthread_create() err=%s", errmsgbuf);
                exit(1);
            }
        }
    }
    if (opts.thread_safe)
    {
        concentrate_loop<MXLockGuard>(clock, opts.cycle_delay, opts.trail_cycle_ms);
    }
    else
    {
        concentrate_loop<MXNoopGuard>(clock, opts.cycle_delay, opts.trail_cycle_ms);
    }
	return 0;
}

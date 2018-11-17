// Myrinet MX DQ data receiver

#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>

#include "mx_extensions.h"
#include "myriexpress.h"

#include "../include/daqmap.h"
#include "daq_core.h"
#include "recvr_utils.hh"

const int MXR_MAX_DCU = 128;
const int MAX_RECEIVE_BUFFERS = 16;     // number of 16Hz cycles we are buffering

/**
 * Diagnostic, return the system time in ms
 */
static int64_t
s_clock (void)
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/**
 * daqMXdata, the format the data arrives in
 */
struct daqMXdata {
    struct rmIpcStr mxIpcData;
    cdsDaqNetGdsTpNum mxTpTable;
    char mxDataBlock[DAQ_DCU_BLOCK_SIZE];
};

/**
 * single_dcu_block the format we keep the data in.
 * We also maintain a lock that should be used when accessing
 * this data.
 */
struct single_dcu_block {
    daq_msg_header_t header;
    char data[DAQ_DCU_BLOCK_SIZE];
    spin_lock lock_;

    single_dcu_block(): header(), data(), lock_()
    {
    }

    void lock()
    {
        lock_.lock();
    }
    void unlock()
    {
        lock_.unlock();
    }
private:
    single_dcu_block(const single_dcu_block& other);
    single_dcu_block& operator=(const single_dcu_block& other);
};

debug_array_2d<single_dcu_block, MXR_MAX_DCU, MAX_RECEIVE_BUFFERS> mxr_data;
debug_array_2d<receive_map_entry_t, MAX_RECEIVE_BUFFERS, MXR_MAX_DCU> receive_map;
spin_lock receive_map_locks[MAX_RECEIVE_BUFFERS];

atomic_flag threads_should_stop;
atomic_flag exit_main_loop;

int cycle_to_buffer_index(int cycle)
{
    return cycle % MAX_RECEIVE_BUFFERS;
}

void mark_dcu_received(int dcuid, int64_t gps_sec, int cycle)
{
    if (dcuid < 0 || dcuid >= MXR_MAX_DCU)
    {
        return;
    }
    receive_map_entry_t new_entry;
    new_entry.s_clock = s_clock();
    new_entry.gps_sec_and_cycle = calc_gps_sec_and_cycle(gps_sec, cycle);
    int index = cycle_to_buffer_index(cycle);
    {
        lock_guard<spin_lock> l_(receive_map_locks[index]);
        receive_map[index][dcuid] = new_entry;
    }
}

void receive_map_copy_row(int cycle, receive_map_entry_t dest[MXR_MAX_DCU])
{
    int index = cycle_to_buffer_index(cycle);
    {
        lock_guard<spin_lock> l_(receive_map_locks[index]);
        std::copy(&(receive_map[index][0]), &(receive_map[index][MXR_MAX_DCU-1]), dest);
    }
}

/// Define max boards, endpoints for mx_rcvr
#define MX_MAX_BOARDS 4
#define MX_MAX_ENDPOINTS 32

#define FILTER 0x12345
#define MATCH_VAL 0xabcdef
#define DFLT_EID 1
#define DFLT_LEN 8192
#define DFLT_END 128
#define MAX_LEN (1024 * 1024 * 1024)
#define DFLT_ITER 1000
#define NUM_RREQ 256
#define NUM_SREQ 4 /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/

#define MATCH_VAL_MAIN (1 << 31)
#define MATCH_VAL_THREAD 1

/**
 * A simple class used to ensure that mx_irecv is called again.  This is
 * designed for the re-registration of the wait buffer.  No error checking is done.
 * When the object goes out of scope it will call mx_irecv.
 */
class setup_mx_irecv {
public:
    setup_mx_irecv(mx_endpoint_t& ep, mx_segment_t* seg, mx_request_t* req):
        ep_(ep), seg_(seg), req_(req)
    {}
    ~setup_mx_irecv()
    {
        mx_irecv(ep_, seg_, 1, MATCH_VAL_MAIN, MX_MATCH_MASK_NONE, 0, req_);
    }
private:
    setup_mx_irecv(const setup_mx_irecv& other);
    setup_mx_irecv& operator=(const setup_mx_irecv& other);

    mx_endpoint_t &ep_;
    mx_segment_t* seg_;
    mx_request_t* req_;
};

static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;

void receiver_mx(int neid) {
    mx_status_t stat;
    mx_segment_t seg;
    uint32_t result;
    struct timeval start_time, end_time;
    std::vector<daqMXdata> buffer(NUM_RREQ);            // mx receive buffers
    int myErrorStat = 0;
    int copySize;
    // float *testData;
    uint32_t match_val = MATCH_VAL_MAIN;
    mx_request_t req[NUM_RREQ];
    mx_endpoint_t ep[MX_MAX_BOARDS * MX_MAX_ENDPOINTS]; // does this need to be an array?  we only use one entry!

    uint32_t board_num = (neid >> 8);
    uint32_t ep_num = neid & 0xff;

    const int daqMXdata_len = sizeof(daqMXdata);

    mx_return_t ret =
        mx_open_endpoint(board_num, ep_num, FILTER, NULL, 0, &ep[neid]);
    if (ret != MX_SUCCESS) {
        fprintf(stderr, "Failed to open board %d endpoint %d error %s\n",
                board_num, ep_num, mx_strerror(ret));
        exit(1);
    }

    /* pre-post our receives */
    for (int cur_req = 0; cur_req < NUM_RREQ; cur_req++) {
        seg.segment_ptr = &buffer[cur_req];
        seg.segment_length = sizeof(daqMXdata);
        mx_endpoint_t ep1 = ep[neid];
        mx_irecv(ep1, &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, &req[cur_req]);
    }

    mx_set_error_handler(MX_ERRORS_RETURN);
    gettimeofday(&start_time, NULL);

    do {
        for (int count = 0; count < NUM_RREQ; count++) {
            /* Wait for the receive to complete */
            int cur_req = count & (NUM_RREQ - 1);

        // 	mx_test_or_wait(blocking, ep, &sreq, MX_INFINITE, &stat,
        // &result);
        again:
            mx_return_t ret =
                mx_wait(ep[neid], &req[cur_req], MX_INFINITE, &stat, &result);
            myErrorStat = 0;
            if (ret != MX_SUCCESS) {
                fprintf(stderr, "mx_cancel() eid=%03x failed with status %s\n",
                        neid, mx_strerror(ret));
                exit(1); // Not clear what to do in this case; this indicates
                // shortage of memory or resources
            }
            if (result == 0) { // Request incomplete
                goto again;    // Restart incomplete request
            } else {           // Request is complete
                if (stat.code !=
                    MX_STATUS_SUCCESS) { // Request completed, but bad code
                    fprintf(stderr, "mx_wait failed in rcvr eid=%03x, reqn=%d; "
                                    "wait did not complete; status code is "
                                    "%s\n",
                            neid, count, mx_strstatus(stat.code));
                    // mx_return_t ret = mx_cancel(ep[neid], &req[cur_req],
                    // &result);
                    mx_endpoint_addr_t ep_addr;
                    ret = mx_get_endpoint_addr(ep[neid], &ep_addr);
                    if (ret != MX_SUCCESS) {
                        fprintf(stderr, "mx_get_endpoint_addr() eid=%03x "
                                        "failed with status %s\n",
                                neid, mx_strerror(ret));
                    } else {
                        ret = mx_disconnect(ep[neid], ep_addr);
                        if (ret != MX_SUCCESS) {
                            fprintf(stderr, "mx_disconnect() eid=%03x failed "
                                            "with status %s\n",
                                    neid, mx_strerror(ret));
                        } else {
                            fprintf(stderr, "disconnected from the sender on "
                                            "endpoint %03x\n",
                                    neid);
                        }
                    }
                    myErrorStat = 1;
                }
            }
            // Fall through if the request is complete and the code is sucess
            //

            copySize = stat.xfer_length;
            daqMXdata& cur_data = buffer[cur_req];
            seg.segment_ptr = &cur_data;
            seg.segment_length = daqMXdata_len;

            // this will always call mx_irecv to re-start this request at the end of the loop
            setup_mx_irecv register_with_mx(ep[neid], &seg, &req[cur_req]);

            if (!myErrorStat) {
                // printf("received one\n");
                int dcu_id = cur_data.mxIpcData.dcuId;

                // Skip bad inputs
                if (dcu_id < 0 || dcu_id > (MXR_MAX_DCU - 1))
                {
                    continue; // Bad DCU ID
                }
                // daqd should skip dcus it doesn't know
                // about, we cannot
                // if (daqd.dcuSize[0][dcu_id] == 0) {
                //     mx_irecv(ep[neid], &seg, 1, match_val,
                //     MX_MATCH_MASK_NONE,
                //              0, &req[cur_req]);
                //     continue; // Unconfigured DCU
                // }

                int64_t gps_sec = 0;
                int cycle = cur_data.mxIpcData.cycle;
                int len = cur_data.mxIpcData.dataBlockSize;

                single_dcu_block& cur_dest = mxr_data[dcu_id][cycle];
                {
                    lock_guard<single_dcu_block> l_(cur_dest);

                    char *dataSource = (char *)cur_data.mxDataBlock;
                    char *dataDest = cur_dest.data;

                    // Move the block data into the buffer
                    memcpy(dataDest, dataSource, /* len */ DAQ_DCU_BLOCK_SIZE );

                    cur_dest.header.dcuId = static_cast<unsigned int>(dcu_id);
                    cur_dest.header.fileCrc = cur_data.mxIpcData.crc;
                    cur_dest.header.status = 2;
                    cur_dest.header.cycle = static_cast<unsigned int>(cycle);
                    gps_sec = cur_dest.header.timeSec = cur_data.mxIpcData.bp[cycle].timeSec;
                    cur_dest.header.timeNSec = cur_data.mxIpcData.bp[cycle].timeNSec;
                    cur_dest.header.dataCrc = cur_data.mxIpcData.bp[cycle].crc;
                    cur_dest.header.dataBlockSize = cur_data.mxIpcData.dataBlockSize;
                    cur_dest.header.tpBlockSize = DAQ_DCU_BLOCK_SIZE - cur_data.mxIpcData.dataBlockSize;
                    cur_dest.header.tpCount = static_cast<unsigned int>(cur_data.mxTpTable.count);
                    std::copy(cur_data.mxTpTable.tpNum, cur_data.mxTpTable.tpNum + cur_dest.header.tpCount, cur_dest.header.tpNum);
                }
                // do this in a scope where the lock on the data segment is not held, so we are never holding two locks
                mark_dcu_received(dcu_id, gps_sec, cycle);
            }
            // daqd.producer1.rcvr_stats[neid].tick();
        }
    } while (!threads_should_stop.is_set());
    gettimeofday(&end_time, NULL);
    fprintf(stderr, "mx_wait failed in rcvr after do loop\n");
    // exit(1);
}

void *receiver_thread(void *args)
{
    receiver_mx(*reinterpret_cast<int*>(args));
    return NULL;
}


int mx_ep_opened = 0;

/// Initialize MX library.
/// Returns the maximum number of end-points supoprted in the system.
unsigned int open_mx(void) {
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

    if (mx_ep_opened)
        return max_endpoints | (nics_available << 8);

    printf("%ld\n", sizeof(struct daqMXdata));

    // So that openmx is not aborting on connection loss
    char omx_env_setting[] = "OMX_ERRORS_ARE_FATAL=0";
    char mx_env_setting[] = "MX_ERRORS_ARE_FATAL=0";
    putenv(omx_env_setting);
    putenv(mx_env_setting);

    mx_init();

    /* set up defaults */
    sysname = NULL;
    filter = FILTER;
    ret = mx_get_info(0, MX_MAX_NATIVE_ENDPOINTS, 0, 0, &max_endpoints,
                      sizeof(max_endpoints));
    if (ret != MX_SUCCESS) {
        fprintf(stderr, "Failed to do mx_get_info: %s\n", mx_strerror(ret));
        exit(1);
    }
    fprintf(stderr, "MX has %d maximum end-points configured\n", max_endpoints);
    ret = mx_get_info(0, MX_NIC_COUNT, 0, 0, &nics_available,
                      sizeof(nics_available));
    if (ret != MX_SUCCESS) {
        fprintf(stderr, "Failed to do mx_get_info: %s\n", mx_strerror(ret));
        exit(1);
    }
    fprintf(stderr, "%d MX NICs available\n", nics_available);
    /// make sure these don't exceed array allocations
    if (max_endpoints > MX_MAX_ENDPOINTS) {
        fprintf(stderr,
                "ERROR: max end-points of %d exceeds array limit of %d\n",
                max_endpoints, MX_MAX_ENDPOINTS);
        exit(1);
    }
    if (nics_available > MX_MAX_BOARDS) {
        fprintf(stderr,
                "ERROR: available nics of %d exceeds array limit of %d\n",
                nics_available, MX_MAX_BOARDS);
        exit(1);
    }

    mx_ep_opened = 1;
    return max_endpoints | (nics_available << 8);
}

void close_mx() { mx_finalize(); }

int64_t wait_for_first_system(const int next_cycle, const int64_t prev_sec_and_cycle)
{
    int wait_count = 0;
    int64_t expected_sec_and_cycle = prev_sec_and_cycle;
    int64_t result = 0;
    receive_map_entry_t cur_row[MXR_MAX_DCU];

    if (prev_sec_and_cycle == 0)
    {
        expected_sec_and_cycle = 0;
    }
    bool keep_waiting = true;
    do
    {
        receive_map_copy_row(next_cycle, cur_row);
        for (int ii = 0; ii < MXR_MAX_DCU; ++ii)
        {
            if (cur_row[ii].gps_sec_and_cycle >= expected_sec_and_cycle
                && cur_row[ii].gps_sec_and_cycle != 0
                && extract_cycle(cur_row[ii].gps_sec_and_cycle) == next_cycle)
            {
                result = cur_row[ii].gps_sec_and_cycle;
                keep_waiting = false;
                break;
            }
            if (keep_waiting)
            {
                usleep(1000);
                ++wait_count;
            }
        }
    } while (keep_waiting);

    return result;
}

void consentrate_data(const int64_t expected_sec_and_cycle, daq_multi_dcu_data_t* dest)
{
    receive_map_entry_t cur_row[MXR_MAX_DCU];

    int cur_cycle = static_cast<int>(extract_cycle(expected_sec_and_cycle));
    int cycle_index = cycle_to_buffer_index(cur_cycle);

    receive_map_copy_row(cur_cycle, cur_row);
    char* dest_data = dest->dataBlock;
    int remaining = DAQ_TRANSIT_FE_DATA_BLOCK_SIZE;
    for (int ii = 0; ii < MXR_MAX_DCU; ++ii)
    {
        if (cur_row[ii].gps_sec_and_cycle == expected_sec_and_cycle)
        {
            int data_size = 0;
            single_dcu_block& cur_block = mxr_data[ii][cycle_index];
            lock_guard<single_dcu_block> l_(cur_block);
            {
                dest->header.dcuheader[ii] = cur_block.header;
                data_size = cur_block.header.dataBlockSize + cur_block.header.tpBlockSize;
                if (data_size > remaining)
                {
                    data_size = remaining;
                }
                memcpy(dest_data, dest->dataBlock, data_size);
            }
            dest->header.fullDataBlockSize += data_size;
            dest_data += data_size;
            remaining -= data_size;
            dest->header.dcuTotalModels++;
        }
    }

}

// *************************************************************************
// Catch Control C to end cod in controlled manner
// *************************************************************************
void intHandler(int dummy) {
    exit_main_loop.set();
}

void sigpipeHandler(int dummy)
{}

int main(int argc, char *argv[]) {

    // set up to catch Control C
    signal(SIGINT,intHandler);
    // setup to ignore sig pipe
    signal(SIGPIPE, sigpipeHandler);

    unsigned int max_endpoints = open_mx();
    unsigned int nics_available = max_endpoints >> 8;
    max_endpoints &= 0xff;

    int bp_aray[MX_MAX_BOARDS][MX_MAX_ENDPOINTS];
    pthread_t gm_tid[MX_MAX_BOARDS][MX_MAX_ENDPOINTS];

    for (int bnum = 0; bnum < nics_available; bnum++) { // Start
        for (int j = 0; j < max_endpoints; j++) {
            int bp = j;
            bp = bp + (bnum * 256);
            /* calculate address within array */
            bp_aray[bnum][j] = bp;
            void *bpPtr = (int *)(bp_aray + bnum) + j;
            int my_err_no = 0;
            my_err_no =
                    pthread_create(&gm_tid[bnum][j], NULL, receiver_thread, bpPtr);
            if (my_err_no)
            {
                char errmsgbuf[80];
                strerror_r(my_err_no, errmsgbuf, sizeof(errmsgbuf));
                fprintf(stderr, "pthread_create() err=%s", errmsgbuf);
                exit(1);
            }
        }
    }
    int next_cycle = 0;
    int64_t prev_sec_and_cycle = 0;
    do
    {
        daq_multi_dcu_data_t concentrated_data;

        int64_t cur_sec_and_cycle = wait_for_first_system(next_cycle, prev_sec_and_cycle);

        if (prev_sec_and_cycle > 0)
        {
            consentrate_data(prev_sec_and_cycle, &concentrated_data);
            // send data out
        }
        prev_sec_and_cycle = cur_sec_and_cycle;
        ++next_cycle;
        next_cycle %= 16;
    } while (!exit_main_loop.is_set());

    printf("stopping threads\n");
    threads_should_stop.set();
    sleep(2);
    close_mx();
    return 0;
}

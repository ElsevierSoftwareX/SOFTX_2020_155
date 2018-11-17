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

#include <string>
#include <vector>
#include <iostream>

#include "mx_extensions.h"
#include "myriexpress.h"

#include "../include/daqmap.h"
#include "daq_core.h"
#include "recvr_utils.hh"

const int MXR_MAX_DCU = 128;
const int MAX_RECEIVE_BUFFERS = 16;     // number of 16Hz cycles we are buffering

const char* DEFAULT_BUFFER_NAME = "ifo";
const unsigned int DEFAULT_MAX_DATA_SIZE_MB = 100;

extern "C" {
extern volatile void *findSharedMemorySize(const char *, int);
}

typedef flag_mask<MXR_MAX_DCU> dcu_mask_t;

/**
 * Program options
 */
struct options_t {
    std::string buffer_name;
    unsigned int max_data_size_mb;
    bool abort;

    options_t():
    buffer_name(DEFAULT_BUFFER_NAME),
    max_data_size_mb(DEFAULT_MAX_DATA_SIZE_MB),
    abort(false)
    {}
};

struct local_mx_info {
    uint32_t nics_available;
    uint32_t max_endpoints;
    uint32_t max_requests;
};

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
#define NUM_RREQ 64
#define NUM_SREQ 4 /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/

#define MATCH_VAL_MAIN (1 << 31)
#define MATCH_VAL_THREAD 1

/* A simple class used to ensure that when an endpoint is opened it is also closed. */
class managed_endpoint {
public:
    managed_endpoint(uint32_t board_num, uint32_t endpoint_id, uint32_t filter): endpoint_()
    {
        mx_return_t ret = mx_open_endpoint(board_num, endpoint_id, filter, NULL, 0, &endpoint_);
        if (ret != MX_SUCCESS)
        {
            std::cerr << "Failed to open board " << board_num << " endpoint " << endpoint_id << " error " << mx_strerror(ret) << "\n";
            exit(1);
        }
    }
    ~managed_endpoint()
    {
        mx_close_endpoint(endpoint_);
    }
    mx_endpoint_t get()
    {
        return endpoint_;
    }
private:
    managed_endpoint(const managed_endpoint& other);
    managed_endpoint& operator=(const managed_endpoint& other);

    mx_endpoint_t endpoint_;
};

/**
 * A simple class used to ensure that mx_irecv is called again.  This is
 * designed for the re-registration of the wait buffer.  No error checking is done.
 * When the object goes out of scope it will call mx_irecv.
 */
class setup_mx_irecv {
public:
    setup_mx_irecv(mx_endpoint_t ep, mx_segment_t* seg, mx_request_t* req, int* ctx):
        ep_(ep), seg_(seg), req_(req), ctx_(ctx)
    {}
    ~setup_mx_irecv()
    {
        mx_return_t ret = mx_irecv(ep_, seg_, 1, MATCH_VAL_MAIN, MX_MATCH_MASK_NONE, ctx_, req_);
        std::cout << "mx_irecv returned " << (ret == MX_SUCCESS ? "SUCCESS" : mx_strerror(ret)) << "\n";
    }
private:
    setup_mx_irecv(const setup_mx_irecv& other);
    setup_mx_irecv& operator=(const setup_mx_irecv& other);

    mx_endpoint_t ep_;
    mx_segment_t* seg_;
    mx_request_t* req_;
    int *ctx_;
};

static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;

void receiver_mx(int neid) {
    mx_status_t stat;
    mx_segment_t seg;
    uint32_t result;
    std::vector<daqMXdata> buffer(NUM_RREQ);            // mx receive buffers
    int myErrorStat = 0;
    int copySize;
    // float *testData;
    uint32_t match_val = MATCH_VAL_MAIN;
    int context_ids[NUM_RREQ];                          // a series of integers to provide index to the requests
    mx_request_t req[NUM_RREQ];

    uint32_t board_num = (neid >> 8);
    uint32_t ep_num = neid & 0xff;

    int messages_received = 0;

    const int daqMXdata_len = sizeof(daqMXdata);

    std::cout << "about to open board " << board_num << " ep " << ep_num << " neid " << neid << "\n";
    managed_endpoint endpoint(board_num, ep_num, FILTER);

    /* pre-post our receives */
    for (int cur_req = 0; cur_req < NUM_RREQ; cur_req++) {
        seg.segment_ptr = &buffer[cur_req];
        seg.segment_length = sizeof(daqMXdata);

        context_ids[cur_req] = cur_req;

        mx_return_t ret = mx_irecv(endpoint.get(), &seg, 1, match_val, MX_MATCH_MASK_NONE,
                reinterpret_cast<void*>(&context_ids[cur_req]), &req[cur_req]);
        if (ret != MX_SUCCESS)
        {
            std::cerr << "mx_irecv return an error: " << mx_strerror(ret) << "\n";
        }
        if (board_num != 0) {
            //std::cout << ".(" << neid << " " << cur_req << ")" << std::flush;
        }
    }

    mx_set_error_handler(MX_ERRORS_RETURN);

    std::cout << neid << " registered buffers and beginning wait loop\n";

    while (!threads_should_stop.is_set())
    {
        mx_request_t current_request;
        result = 0;
        mx_return_t ret = mx_peek(endpoint.get(), 10, &current_request, &result);
        if (ret != MX_SUCCESS)
        {
            std::cerr << "mx_peek on " << neid << " failed: " << mx_strerror(ret) << "\n";
            exit(1);
        }
        // timeout, try again, checking to see if we should abort
        if (!result)
        {
            continue;
        }
        // get the context for the request, id the index into buffer ... (request number)
        int *current_context = NULL;
        mx_context(&current_request, reinterpret_cast<void**>(&current_context));

        if (current_request != req[*current_context])
        {
            std::cout << "The request is different" << std::endl;
        }

        ret = mx_test(endpoint.get(), &current_request, &stat, &result);
        if (ret != MX_SUCCESS)
        {
            std::cerr << "mx_test failed on " << neid << " which was unexpected: " << mx_strerror(ret) << "\n";
            exit(1);
        }
        if (result == 0)    // request not ready
        {
            continue;   // should never happen as we did a mx_peek
        }
        /* from this point on we MUST restart a read on this buffer */
        daqMXdata& cur_data = buffer[*current_context];
        seg.segment_ptr = &cur_data;
        seg.segment_length = daqMXdata_len;

        ++messages_received;
        std::cout << neid << " received a message, #" << messages_received << "  ctx# " << *current_context << "\n" << std::endl;

        // this will always call mx_irecv to re-start this request at the end of the loop
        setup_mx_irecv register_with_mx(endpoint.get(), &seg, &current_request, current_context);

        if (stat.code != MX_STATUS_SUCCESS)
        {
            // Request completed, but bad code
            fprintf(stderr, "mx_wait failed in rcvr eid=%03x; "
                            "wait did not complete; status code is "
                            "%s\n",
                    neid, mx_strstatus(stat.code));
            // mx_return_t ret = mx_cancel(ep[neid], &req[cur_req],
            // &result);
            mx_endpoint_addr_t ep_addr;
            ret = mx_get_endpoint_addr(endpoint.get(), &ep_addr);
            if (ret != MX_SUCCESS) {
                fprintf(stderr, "mx_get_endpoint_addr() eid=%03x "
                                "failed with status %s\n",
                        neid, mx_strerror(ret));
            } else {
                ret = mx_disconnect(endpoint.get(), ep_addr);
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
            continue;
        }

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

        // daqd.producer1.rcvr_stats[neid].tick();

    }
    std::cout << "mx_receiver " << neid << " completed\n";
    // exit(1);
}

void *receiver_thread(void *args)
{
    std::cout << "calling receiver_mx with " << *reinterpret_cast<int*>(args) << "\n";
    receiver_mx(*reinterpret_cast<int*>(args));
    return NULL;
}


/// Initialize MX library.
/// Returns the maximum number of end-points supoprted in the system.
local_mx_info open_mx() {
    static int mx_ep_opened = 0;
    static local_mx_info info;
    uint16_t my_eid;
    uint32_t board_id;
    uint32_t filter;
    char *sysname;
    int c;
    extern char *optarg;
    mx_return_t ret;
    int fd;

    if (mx_ep_opened)
        return info;

    printf("%ld\n", sizeof(struct daqMXdata));

    // So that openmx is not aborting on connection loss
    char omx_env_setting[] = "OMX_ERRORS_ARE_FATAL=1";
    char mx_env_setting[] = "MX_ERRORS_ARE_FATAL=1";
    putenv(omx_env_setting);
    putenv(mx_env_setting);

    ret = mx_init();
    if (ret != MX_SUCCESS)
    {
        fprintf(stderr, "mx_init failed: %s\n", mx_strerror(ret));
        exit(1);
    }

    /* set up defaults */
    sysname = NULL;
    filter = FILTER;
    ret = mx_get_info(0, MX_MAX_NATIVE_ENDPOINTS, 0, 0, &info.max_endpoints,
                      sizeof(info.max_endpoints));
    if (ret != MX_SUCCESS) {
        fprintf(stderr, "Failed to do mx_get_info: %s\n", mx_strerror(ret));
        exit(1);
    }
    fprintf(stderr, "MX has %d maximum end-points configured\n", info.max_endpoints);
    ret = mx_get_info(0, MX_NIC_COUNT, 0, 0, &info.nics_available,
                      sizeof(info.nics_available));
    if (ret != MX_SUCCESS) {
        fprintf(stderr, "Failed to do mx_get_info: %s\n", mx_strerror(ret));
        exit(1);
    }
    fprintf(stderr, "%d MX NICs available\n", info.nics_available);
    /// make sure these don't exceed array allocations
    if (info.max_endpoints > MX_MAX_ENDPOINTS) {
        fprintf(stderr,
                "ERROR: max end-points of %d exceeds array limit of %d\n",
                info.max_endpoints, MX_MAX_ENDPOINTS);
        exit(1);
    }
    if (info.nics_available > MX_MAX_BOARDS) {
        fprintf(stderr,
                "ERROR: available nics of %d exceeds array limit of %d\n",
                info.nics_available, MX_MAX_BOARDS);
        exit(1);
    }
    ret = mx_get_info(0, MX_NATIVE_REQUESTS, 0, 0, &info.max_requests,sizeof(info.max_requests));
    if (ret != MX_SUCCESS)
    {
        fprintf(stderr, "Failed to do mx_get_info: %s\n", mx_strerror(ret));
        exit(1);
    }
    fprintf(stderr, "MX supports %d native requests\n", info.max_requests);

    mx_ep_opened = 1;
    return info;
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
        }
        if (keep_waiting)
        {
            usleep(1000);
            ++wait_count;
        }
        if (exit_main_loop.is_set())
        {
            return 0;
        }
    } while (keep_waiting);

    return result;
}

void consentrate_data(const int64_t expected_sec_and_cycle, volatile daq_multi_dcu_data_t *dest, unsigned int dest_size,
                      dcu_mask_t &receive_mask)
{
    receive_map_entry_t cur_row[MXR_MAX_DCU];

    int cur_cycle = static_cast<int>(extract_cycle(expected_sec_and_cycle));
    int cycle_index = cycle_to_buffer_index(cur_cycle);

    receive_map_copy_row(cur_cycle, cur_row);
    volatile char* dest_data = dest->dataBlock;
    unsigned int remaining = dest_size - sizeof(daq_multi_dcu_header_t);
    for (int ii = 0; ii < MXR_MAX_DCU; ++ii)
    {
        if (cur_row[ii].gps_sec_and_cycle == expected_sec_and_cycle)
        {
            int data_size = 0;
            single_dcu_block& cur_block = mxr_data[ii][cycle_index];
            lock_guard<single_dcu_block> l_(cur_block);
            {
                //dest->header.dcuheader[ii] = cur_block.header;
                memcpy((void*)&(dest->header.dcuheader[ii]), &(cur_block.header), sizeof(cur_block.header));
                data_size = cur_block.header.dataBlockSize + cur_block.header.tpBlockSize;
                if (data_size > remaining)
                {
                    data_size = remaining;
                }
                //std::copy(cur_block.data, cur_block.data + data_size, dest_data);
                memcpy((char*)dest_data, cur_block.data, static_cast<size_t>(data_size));
            }
            dest->header.fullDataBlockSize += data_size;
            dest_data += data_size;
            remaining -= data_size;
            dest->header.dcuTotalModels++;
            receive_mask.set(ii);
        }
    }

}

void dcu_mask_difference_printer(const bool prev_flag, const bool cur_flag, unsigned int index)
{
    std::cout << (prev_flag ? "(-" : "(+") << index << ") ";
};

void dump_dcu_mask_diffs(const dcu_mask_t& prev, const dcu_mask_t& cur)
{
    if (prev == cur)
    {
        return;
    }

    std::cout << "\nDCU state change:\n";
    foreach_difference(prev, cur, dcu_mask_difference_printer);
    std::cout << "\n";
}

void show_help(char *prog)
{
    std::cout << "Usage " << prog << " options\n\nWhere options are:\n";
    std::cout << "\t-h - Show this help\n";
    std::cout << "\t-b <name> - Set the output mbuf name - defaults to " << DEFAULT_BUFFER_NAME << "\n";
    std::cout << "\t-m <size> - Set the output mbuf size in MB [20-100] - defaults to " << DEFAULT_MAX_DATA_SIZE_MB << "\n";
    std::cout << std::endl;
}

// *************************************************************************
// Catch Control C to end cod in controlled manner
// *************************************************************************
void intHandler(int dummy) {
    exit_main_loop.set();
}

void sigpipeHandler(int dummy)
{}


options_t parse_options(int argc, char *const *argv) {
    options_t opts;
    int arg = 0;
    while ((arg = getopt(argc, argv, "b:m:h")) != EOF)
    {
        switch (arg) {
            case 'b':
                opts.buffer_name = optarg;
                break;
            case 'm':
                opts.max_data_size_mb = static_cast<unsigned int>(atoi(optarg));
                if (opts.max_data_size_mb < 20)
                {
                    std::cerr << "Min data block size is 20 MB\n";
                    opts.abort = true;
                }
                else if (opts.max_data_size_mb > 100)
                {
                    std::cerr << "Max data block size is 100 MB\n";
                    opts.abort = true;
                }
                break;
            case 'h':
            default:
                show_help(argv[0]);
                opts.abort = true;
        }
    }
    return opts;
}

int main(int argc, char *argv[]) {

    int max_data_size = 0;
    volatile daq_multi_cycle_header_t* ifo_header = NULL;
    volatile char* ifo_data = NULL;

    options_t opts = parse_options(argc, argv);
    if (opts.abort)
    {
        exit(1);
    }
    max_data_size = opts.max_data_size_mb * 1024*1024;

    ifo_header = reinterpret_cast<volatile daq_multi_cycle_header_t *>(findSharedMemorySize(opts.buffer_name.c_str(), opts.max_data_size_mb));
    ifo_data = (volatile char*)ifo_header + sizeof(daq_multi_cycle_header_t);
    unsigned int cycle_data_size = (max_data_size - sizeof(daq_multi_cycle_header_t)) / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    cycle_data_size -= (cycle_data_size % 8);
    std::cout << "cycle data size = " << cycle_data_size << "\t" << opts.max_data_size_mb << "\n";
    ifo_header->cycleDataSize = cycle_data_size;
    ifo_header->maxCycle = DAQ_NUM_DATA_BLOCKS;
    ifo_header->curCycle = DAQ_NUM_DATA_BLOCKS + 1;

    // set up to catch Control C
    signal(SIGINT,intHandler);
    // setup to ignore sig pipe
    signal(SIGPIPE, sigpipeHandler);

    local_mx_info mx_info = open_mx();

    int bp_aray[MX_MAX_BOARDS][MX_MAX_ENDPOINTS];
    pthread_t gm_tid[MX_MAX_BOARDS][MX_MAX_ENDPOINTS];

    std::cout << "&bp_aray[0][0] = " << &bp_aray[0][0] << "\n";
    std::cout << "&bp_aray[1][0] = " << &bp_aray[1][0] << "\n";

    for (int bnum = 0; bnum < mx_info.nics_available; bnum++) { // Start
        for (int j = 0; j < mx_info.max_endpoints; j++) {
            int bp = j;
            bp = bp + (bnum * 256);
            /* calculate address within array */
            bp_aray[bnum][j] = bp;
            //void *bpPtr = (int *)(bp_aray + bnum) + j;
            void *bpPtr = (void *)(&bp_aray[bnum][j]);
            int my_err_no = 0;

            std::cout << "setup for board " << bnum << " ep " << j << " *bpPtr = " << *(int*)bpPtr << " bpPtr = " << bpPtr << "\n";

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
    dcu_mask_t receive_mask;
    dcu_mask_t prev_receive_mask;

    std::cout << "Threads created, entering main loop\n";

    do
    {
        int64_t cur_sec_and_cycle = wait_for_first_system(next_cycle, prev_sec_and_cycle);

        if (cur_sec_and_cycle != 0)
        {
            if (prev_sec_and_cycle > 0)
            {
                receive_mask.clear();

                int64_t sending_cycle = extract_cycle(prev_sec_and_cycle);
                volatile daq_multi_dcu_data_t *ifoDataBlock = reinterpret_cast<volatile daq_multi_dcu_data_t *>(
                        ifo_data + (cycle_data_size * sending_cycle));
                consentrate_data(prev_sec_and_cycle, ifoDataBlock, cycle_data_size, receive_mask);
                ifo_header->curCycle = static_cast<unsigned int>(sending_cycle);

                dump_dcu_mask_diffs(prev_receive_mask, receive_mask);
                prev_receive_mask = receive_mask;
            }
            prev_sec_and_cycle = cur_sec_and_cycle;
            ++next_cycle;
            next_cycle %= 16;
        }
    } while (!exit_main_loop.is_set());

    std::cout << "stopping threads\n";
    threads_should_stop.set();
    sleep(2);
    close_mx();
    return 0;
}

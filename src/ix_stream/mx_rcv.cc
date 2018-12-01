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

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "dolphin_common.hh"

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
    std::string receive_map_dump_fname;
    unsigned int max_data_size_mb;
    unsigned int dolphin_group;
    bool abort;
    bool xmit_data;

    options_t():
    buffer_name(DEFAULT_BUFFER_NAME),
    receive_map_dump_fname(),
    max_data_size_mb(DEFAULT_MAX_DATA_SIZE_MB),
    dolphin_group(-1),
    abort(false),
    xmit_data(false)
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

/**
 * Simple structure used to get a mapping of which dcus did not report
 * in the current cycle that did in the previous
 */
struct missing_map_t {
    missing_map_t(): missing_map(), differences(0)
    {
        std::fill(&missing_map[0], &missing_map[0] + MXR_MAX_DCU, false);
    }
    void operator()(const bool old_entry, const bool new_entry, int i)
    {
        missing_map[i] = old_entry;
        differences += (old_entry ? 1 : 0);
    }
    bool missing_map[MXR_MAX_DCU];
    int differences;
};

typedef strong_type<int, 0> buffer_index_t;
typedef strong_type<int, 1> dcu_index_t;
typedef debug_array_2d<receive_map_entry_t, MAX_RECEIVE_BUFFERS, MXR_MAX_DCU, buffer_index_t, dcu_index_t> receive_map_t;

debug_array_2d<single_dcu_block, MXR_MAX_DCU, MAX_RECEIVE_BUFFERS, dcu_index_t, buffer_index_t> mxr_data;
receive_map_t receive_map;
spin_lock receive_map_locks[MAX_RECEIVE_BUFFERS];

atomic_flag threads_should_stop;
atomic_flag exit_main_loop;

buffer_index_t cycle_to_buffer_index(int cycle)
{
    return buffer_index_t(cycle % MAX_RECEIVE_BUFFERS);
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
    buffer_index_t index = cycle_to_buffer_index(cycle);
    {
        //lock_guard<spin_lock> l_(receive_map_locks[get_value(index)]);
        lock_guard<spin_lock> l_(receive_map_locks[0]);
        receive_map[index][dcu_index_t(dcuid)] = new_entry;
    }
}

void receive_map_copy_row(int cycle, receive_map_entry_t dest[MXR_MAX_DCU])
{
    buffer_index_t index = cycle_to_buffer_index(cycle);
    {
        //lock_guard<spin_lock> l_(receive_map_locks[get_value(index)]);
        lock_guard<spin_lock> l_(receive_map_locks[0]);
        std::copy(&(receive_map[index][0]), &(receive_map[index][MXR_MAX_DCU-1]), dest);
    }
}

void receive_map_copy(receive_map_t& dest)
{
    lock_guard<spin_lock> l_(receive_map_locks[0]);
    dest = receive_map;
}

class scoreboard_manager {
public:
    scoreboard_manager(receive_map_t& scoreboard_snapshot, dcu_mask_t& mask):scoreboard_snapshot_(scoreboard_snapshot), mask_(mask) {}
    void get_scoreboard_row(int cycle, receive_map_entry_t dest[MXR_MAX_DCU])
    {
        buffer_index_t cycle_index = cycle_to_buffer_index(cycle);
        receive_map_copy(scoreboard_snapshot_);
        std::copy(&(scoreboard_snapshot_[cycle_index][0]), &(scoreboard_snapshot_[cycle_index][MXR_MAX_DCU - 1]), dest);
    }
    void mark_received(int dcu)
    {
        mask_.set(dcu);
    }
private:
    scoreboard_manager(const scoreboard_manager& other);
    scoreboard_manager& operator=(const scoreboard_manager& other);

    receive_map_t& scoreboard_snapshot_;
    dcu_mask_t& mask_;
};

void dump_scoreboard(std::ostream& os, receive_map_t& local_map, missing_map_t& missing_map, int64_t sec_and_cycle)
{
    // figure out which columns have entries
    int64_t column_has_data[MXR_MAX_DCU];
    std::fill(&column_has_data[0], &column_has_data[0] + MXR_MAX_DCU, 0);
    {
        for (int i = 0; i < MAX_RECEIVE_BUFFERS; ++i)
        {
            buffer_index_t index = cycle_to_buffer_index(i);
            for (int j = 0; j < MXR_MAX_DCU; ++j)
            {
                column_has_data[j] |= local_map[index][dcu_index_t(j)].gps_sec_and_cycle;
            }
        }
    }
    os << "-----------------------------\n";
    os << "- " << sec_and_cycle << " (" << extract_gps(sec_and_cycle);
    os << ":" << extract_cycle(sec_and_cycle) <<")\n";
    os << "-----------------------------\n";

    buffer_index_t cur_cycle = cycle_to_buffer_index(static_cast<int>(extract_cycle(sec_and_cycle)));

    for (int i = 0; i < MAX_RECEIVE_BUFFERS; ++i)
    {
        buffer_index_t cycle_index = cycle_to_buffer_index(i);
        os << i << ") ";
        for (int j = 0; j < MXR_MAX_DCU; ++j)
        {
            if (column_has_data[j] == 0)
            {
                continue;
            }
            receive_map_entry_t& cur_entry = local_map[cycle_index][dcu_index_t(j)];
            os << "| " << j << "] ";
            os << (cur_cycle.get() == cycle_index.get() && missing_map.missing_map[j] ? "*" : " ");
            os << extract_gps(cur_entry.gps_sec_and_cycle) << ":" << extract_cycle(cur_entry.gps_sec_and_cycle);
            os << " (" << cur_entry.s_clock << ") ";
        }
        os << "|\n";
    }
    os << "-----------------------------\n";
    os << std::endl;
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

static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;

/**
 * Handle a bad message from mx_wait.
 * @param neid
 * @param endpoint
 * @param stat
 */
void handle_bad_message(int neid, mx_endpoint_t endpoint, const mx_status_t& stat)
{
    mx_return_t ret = MX_SUCCESS;

    if (stat.code == MX_STATUS_SUCCESS)
    {
        return;
    }
    // Request completed, but bad code
    fprintf(stderr, "mx_wait failed in rcvr eid=%03x; "
                    "wait did not complete; status code is "
                    "%s\n",
            neid, mx_strstatus(stat.code));
    // mx_return_t ret = mx_cancel(ep[neid], &req[cur_req],
    // &result);
    mx_endpoint_addr_t ep_addr;
    ret = mx_get_endpoint_addr(endpoint, &ep_addr);
    if (ret != MX_SUCCESS) {
        fprintf(stderr, "mx_get_endpoint_addr() eid=%03x "
                        "failed with status %s\n",
                neid, mx_strerror(ret));
    } else {
        ret = mx_disconnect(endpoint, ep_addr);
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
}

/**
 * Copy and convert the daqMXdata structure from a message into
 * the global dcu data store.
 * @param cur_data
 */
void handle_received_message(const daqMXdata& cur_data)
{
    // printf("received one\n");
    int dcu_id = cur_data.mxIpcData.dcuId;

    // Skip bad inputs
    if (dcu_id < 0 || dcu_id > (MXR_MAX_DCU - 1))
    {
        return;
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
    int cycle = static_cast<int>(cur_data.mxIpcData.cycle);
    buffer_index_t cycle_index = cycle_to_buffer_index(cycle);
    int len = cur_data.mxIpcData.dataBlockSize;

    if (dcu_id == 7)
    {
        std::cout << "> " << cur_data.mxIpcData.bp[cycle].timeSec << ":" << cycle << "(" << cycle_index.get() << ") => ";
    }

    single_dcu_block& cur_dest = mxr_data[dcu_index_t(dcu_id)][cycle_index];
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
    if (dcu_id == 7)
    {
        std::cout << gps_sec << ":" << cycle << "   - crc - " << cur_data.mxIpcData.bp[cycle].crc << "\n";
    }
}

void receiver_mx(int neid) {
    mx_status_t stat;
    mx_segment_t seg;
    uint32_t result;
    std::vector<daqMXdata> buffer(NUM_RREQ);            // mx receive buffers
    int myErrorStat = 0;
    int copySize;
    // float *testData;
    mx_return_t ret = MX_SUCCESS;
    uint32_t match_val = MATCH_VAL_MAIN;
    mx_request_t req[NUM_RREQ];

    uint32_t board_num = (neid >> 8);
    uint32_t ep_num = neid & 0xff;

    int messages_received = 0;

    const int daqMXdata_len = sizeof(daqMXdata);

    std::cout << "about to open board " << board_num << " ep " << ep_num << " neid " << neid << "\n";
    managed_endpoint endpoint(board_num, ep_num, FILTER);

    /* pre-post our receives */
    for (int cur_req = 0; cur_req < NUM_RREQ; ++cur_req) {
        seg.segment_ptr = &buffer[cur_req];
        seg.segment_length = sizeof(daqMXdata);

        ret = mx_irecv(endpoint.get(), &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, &req[cur_req]);
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
        for(int cur_req = 0; cur_req < NUM_RREQ && !threads_should_stop.is_set(); ++cur_req) {
            result = 0;
            do
            {
                ret = mx_wait(endpoint.get(), &req[cur_req], 10, &stat, &result);
                if (ret != MX_SUCCESS) {
                    std::cerr << "mx_wait on " << neid << " failed: " << mx_strerror(ret) << "\n";
                    exit(1);
                }
            } while (!result);
            daqMXdata& cur_data = buffer[cur_req];
            seg.segment_ptr = &cur_data;
            seg.segment_length = daqMXdata_len;

            ++messages_received;
            //std::cout << neid << " received a message, #" << messages_received << "\n" << std::endl;
            //std::cout << "." << std::flush;
            if (stat.code == MX_STATUS_SUCCESS)
            {
                handle_received_message(cur_data);
            }
            else
            {
                handle_bad_message(neid, endpoint.get(), stat);
            }
            memset(&req[cur_req], 0, sizeof(req[cur_req]));
            mx_irecv(endpoint.get(), &seg, 1, match_val, MX_MATCH_MASK_NONE, 0, &req[cur_req]);
        }
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
                      scoreboard_manager& manager)
{
    receive_map_entry_t cur_row[MXR_MAX_DCU];

    int cur_cycle = static_cast<int>(extract_cycle(expected_sec_and_cycle));
    buffer_index_t cycle_index = cycle_to_buffer_index(cur_cycle);

    volatile char* dest_data = &(dest->dataBlock[0]);
    dest->header.fullDataBlockSize = 0;
    dest->header.dcuTotalModels = 0;
    unsigned int remaining = dest_size - sizeof(daq_multi_dcu_header_t);

    //receive_map_copy_row(cur_cycle, cur_row);
    //receive_map_copy( local_map );
    //std::copy(&(local_map[cycle_index][0]), &(local_map[cycle_index][MXR_MAX_DCU - 1]), cur_row);
    manager.get_scoreboard_row(cur_cycle, cur_row);

    int model_index = 0;

    for (int ii = 0; ii < MXR_MAX_DCU; ++ii)
    {
        if (cur_row[ii].gps_sec_and_cycle == expected_sec_and_cycle)
        {
            int data_size = 0;
            single_dcu_block& cur_block = mxr_data[dcu_index_t(ii)][cycle_index];
            lock_guard<single_dcu_block> l_(cur_block);
            {
                //dest->header.dcuheader[model_index] = cur_block.header;
                memcpy((void*)&(dest->header.dcuheader[model_index]), &(cur_block.header), sizeof(cur_block.header));
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
            ++model_index;
            manager.mark_received(ii);
        }
    }
    dest->header.dcuTotalModels = model_index;

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
    std::cout << "\t-g <num>  - Set the dolphin group number to transmit data on.\n";
    std::cout << "\t-X <path> - Debug dump of the receive timing map to the given path when a dcu is missed.\n";
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
    while ((arg = getopt(argc, argv, "b:g:hm:X:")) != EOF)
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
            case 'g':
                    opts.dolphin_group = static_cast<unsigned int>(atoi(optarg));
                    opts.xmit_data = true;
                break;
            case 'X':
                opts.receive_map_dump_fname = optarg;
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
    std::fstream scoreboard_dump_stream;
    scoped_ptr<simple_dolphin> dolphin;

    daq_multi_cycle_header_t *xmitHeader[IX_BLOCK_COUNT];
    static int xmitDataOffset[IX_BLOCK_COUNT];
    int xmitBlockNum = 0;

    options_t opts = parse_options(argc, argv);
    if (opts.abort)
    {
        exit(1);
    }
    max_data_size = opts.max_data_size_mb * 1024*1024;
    if (!opts.receive_map_dump_fname.empty())
    {
        scoreboard_dump_stream.open(opts.receive_map_dump_fname.c_str(), std::ios::out | std::ios::trunc);
        scoreboard_dump_stream << "Opening dump stream for output" << std::endl;
        std::cout << "Opening dump stream for output '" << opts.receive_map_dump_fname << "'" << std::endl;
        if (!scoreboard_dump_stream.is_open())
        {
            std::cerr << "Unable to open debug stream" << std::endl;
            exit(1);
        }
    }

    ifo_header = reinterpret_cast<volatile daq_multi_cycle_header_t *>(findSharedMemorySize(opts.buffer_name.c_str(), opts.max_data_size_mb));
    ifo_data = (volatile char*)ifo_header + sizeof(daq_multi_cycle_header_t);
    unsigned int cycle_data_size = (max_data_size - sizeof(daq_multi_cycle_header_t)) / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    cycle_data_size -= (cycle_data_size % 8);
    std::cout << "cycle data size = " << cycle_data_size << "\t" << opts.max_data_size_mb << "\n";
    ifo_header->cycleDataSize = cycle_data_size;
    ifo_header->maxCycle = DAQ_NUM_DATA_BLOCKS;
    ifo_header->curCycle = DAQ_NUM_DATA_BLOCKS + 1;

    if (opts.xmit_data)
    {
        segmentId = opts.dolphin_group;
        dolphin.reset(new simple_dolphin());

        char* cur_write_address = (char*)writeAddr;
        for (int ii = 0; ii < IX_BLOCK_COUNT; ++ii)
        {
            xmitHeader[ii] = (daq_multi_cycle_header_t*)cur_write_address;
            cur_write_address += IX_BLOCK_SIZE;
            xmitDataOffset[ii] = IX_BLOCK_SIZE * ii + sizeof(struct daq_multi_cycle_header_t);
        }
    }

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
    receive_map_t local_map;

    std::cout << "Threads created, entering main loop\n";
    int dump_scoreboard_for_cycles = 0;

    do
    {
        scoreboard_manager scoreboard(local_map, receive_mask);
        int64_t cur_sec_and_cycle = wait_for_first_system(next_cycle, prev_sec_and_cycle);

        if (cur_sec_and_cycle != 0)
        {
            if (prev_sec_and_cycle > 0)
            {
                receive_mask.clear();

                int64_t sending_cycle = extract_cycle(prev_sec_and_cycle);
                volatile daq_multi_dcu_data_t *ifoDataBlock = reinterpret_cast<volatile daq_multi_dcu_data_t *>(
                        ifo_data + (cycle_data_size * sending_cycle));
                consentrate_data(prev_sec_and_cycle, ifoDataBlock, cycle_data_size, scoreboard);
                ifo_header->curCycle = static_cast<unsigned int>(sending_cycle);

                if (opts.xmit_data)
                {
                    sci_error_t err = SCI_ERR_OK;
                    xmitBlockNum = ifo_header->curCycle % IX_BLOCK_COUNT;
                    SCIMemCpy(sequence, (void*)ifoDataBlock, remoteMap, xmitDataOffset[xmitBlockNum], cycle_data_size, memcpyFlag, &err);
                    xmitHeader[xmitBlockNum]->maxCycle = ifo_header->maxCycle;
                    xmitHeader[xmitBlockNum]->cycleDataSize = cycle_data_size;
                    xmitHeader[xmitBlockNum]->curCycle = ifo_header->curCycle;
                    SCIFlush(sequence, SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
                }

                missing_map_t missing_map;
                foreach_difference(prev_receive_mask, receive_mask, missing_map);
                if (missing_map.differences > 0)
                {
                    dump_scoreboard_for_cycles = 3;
                }

                dump_dcu_mask_diffs(prev_receive_mask, receive_mask);
                prev_receive_mask = receive_mask;

                if (dump_scoreboard_for_cycles > 0 && !opts.receive_map_dump_fname.empty())
                {
                    --dump_scoreboard_for_cycles;
                    dump_scoreboard(scoreboard_dump_stream, local_map, missing_map, prev_sec_and_cycle);
                    if (dump_scoreboard_for_cycles == 1)
                    {
                        exit_main_loop.set();
                    }
                }
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

//
///	@file zmq_recv.cc
///	@brief  Receive zmq stream data and concentrate it in a shared memory buffer
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
#include <assert.h>
#include <time.h>
#include "zmq_daq.h"
#include "../include/daqmap.h"

#include <sys/ioctl.h>
#include "../drv/gpstime/gpstime.h"

#include <zmq.hpp>

#include "zmq_dc_recv.h"

#include "drv/shmem.h"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <sstream>
#include <string>

unsigned int do_wait = 0; // Wait for this number of milliseconds before starting a cycle


//std::vector<zmq_dc::receiver_thread_info> thread_info;


static volatile bool keep_running = true;

struct gps_time {
    long sec;
    long nanosec;

    gps_time(): sec(0), nanosec(0) {}
    explicit gps_time(long s): sec(s), nanosec(0) {}
    gps_time(long s, long ns): sec(s), nanosec(ns) {}
    gps_time(const gps_time& other): sec(other.sec), nanosec(other.nanosec) {}

    gps_time operator-(const gps_time& other)
    {

        gps_time result(sec - other.sec, nanosec - other.nanosec);
        while (result.nanosec < 0) {
            result.nanosec += 1000000000;
            --result.sec;
        }
        return result;
    }
};

std::ostream& operator<<(std::ostream& os, const gps_time& gps)
{
    os << gps.sec << ":" << gps.nanosec;
    return os;
}


class gps_clock {
    int _offset;
    int _fd;
    bool _ok;

    static bool symm_ok(int fd) {
        if (fd < 0)
            return false;
        unsigned long req = 0;
        ioctl(fd, IOCTL_SYMMETRICOM_STATUS, &req);
        return req != 0;
    }
public:
    explicit gps_clock(int offset): _offset(offset), _fd(open("/dev/symmetricom",O_RDWR | O_SYNC)), _ok(gps_clock::symm_ok(_fd)) {}
    ~gps_clock() {
        if (_fd >= 0) close(_fd);
    }

    gps_time now() const
    {
        gps_time result;

        if (!_ok)
            return result;
        unsigned long t[3];
        ioctl(_fd, IOCTL_SYMMETRICOM_TIME, &t);
        result.sec = t[0] + _offset;
        result.nanosec = t[1]*1000 + t[2];
        return result;
    }
};


void
usage()
{
    std::cerr << "Usage: zmq_recv [args] -s server name" << std::endl;
    std::cerr << "-t - use the LIGO timing drivers to check time on each received block of data" << std::endl;
    std::cerr << "-g offset - offset to add to the gps, defaults to 0" << std::endl;
    std::cerr << "-s - server name eg x1lsc0, x1susex, etc." << std::endl;
    std::cerr << "-v - verbose prints cpu_meter test data" << std::endl;
    std::cerr << "-b name - name of the shared memory buffer to write to, defaults to ifo" << std::endl;
    std::cerr << "-m size (in mb) - size of the destination buffer in megabytes" << std::endl;
    std::cerr << "-h - help" << std::endl;
}

static int64_t
s_clock (void)
{
    struct timeval tv;
    gettimeofday (&tv, nullptr);
    return (int64_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void intHandler(int dummy) {
    keep_running = false;
}



/// Parse a space separated list of names into a vector of strings
/// \param sysname Space seperated null terminated ascii string
/// \return std::vector<std::string> of each of the entries in sysname
std::vector<std::string> parse_publisher_list(const char *sysname) {
    std::vector<std::string> sname;
    sname.emplace_back(strtok(const_cast<char*>(sysname), " "));
    for(;;) {
        char *s = strtok(nullptr, " ");
        if (s == nullptr) break;
        sname.emplace_back(std::string(s));
    }
    return sname;
}

std::string create_debug_message(zmq_dc::data_block& block_info) {
    std::string msg("");
    std::ostringstream os;

    daq_dc_data_t *block = block_info.full_data_block;
    int dcuids = block->header.dcuTotalModels;
    unsigned long ets = block->header.dcuheader[dcuids-1].timeSec;
    os << ets << " ";
    for (int i = 0; i < dcuids; ++i) {
        daq_msg_header_t* cur_header = block->header.dcuheader + i;
        os << cur_header->dcuId << " " << cur_header->status << " " << cur_header->dataBlockSize<< " ";
    }
    msg = os.str();
    return msg;
}

int
main(int argc, char **argv)
{
    pthread_t thread_id[DCU_COUNT];
    unsigned int nsys = 1; // The number of mapped shared memories (number of data sources)
    char *sysname = nullptr;
    int c;
    bool timing_check = false;
    int timing_offset = 0;
    size_t max_data_size = (DAQ_TRANSIT_DC_DATA_BLOCK_SIZE*DAQ_NUM_DATA_BLOCKS_PER_SECOND);
    std::string dest_buffer_name = "ifo";

    // Create DAQ message area in local memory
    daq_multi_dcu_data_t mxDataBlock;
    // Declare pointer to local memory message area
    std::cout << "size of mxdata = " << sizeof(mxDataBlock) << std::endl;


    /* set up defaults */
    sysname = nullptr;
    int ii;

    // Declare 0MQ message pointers
    int rc;
    bool do_verbose = false;

    while ((c = getopt(argc, argv, "m:tg:hs:b:v")) != EOF) switch(c) {
            case 'm':
                {
                    std::istringstream os(optarg);
                    os >> max_data_size;
                }
                if (max_data_size < 1) {
                    usage();
                    exit(1);
                }
                max_data_size *= 1024*1024;
                break;
            case 't':
                timing_check = true;
                break;
            case 'g':
                {
                    std::istringstream os(optarg);
                    os >> timing_offset;
                }
                break;
            case 's':
                sysname = optarg;
                break;
            case 'v':
                do_verbose = true;
                break;
            case 'w':
                do_wait = atoi(optarg);
                break;
            case 'b':
                dest_buffer_name = optarg;
                break;
            case 'h':
            default:
                usage();
                exit(1);
        }

    if (sysname == nullptr) { usage(); exit(1); }

    signal(SIGINT,intHandler);

    gps_clock clock(timing_offset);

    std::cout << "Server name: " << sysname << std::endl;

    std::vector<std::string> sname(parse_publisher_list(sysname));
    nsys = sname.size();
    // hard limits are to keep things inside of a signed 32bit integer type
    // used as a bitfield
    if (nsys < 1 || nsys >= std::min(DCU_COUNT,32)) {
        std::cerr << "Please specify a set of nodes to subscribe to.  You must provide between 1 and ";
        std::cerr << std::min(DCU_COUNT, 32) << " entries" << std::endl;
        exit(1);
    }

    std::cout << "nsys = " << nsys << "\n";
    for(ii=0;ii<sname.size();ii++) {
        std::cout << "sys " << ii << " =  " << sname[ii] << "\n";
    }

    zmq_dc::ZMQDCReceiver dc_receiver(sname);
    int dataRdy = dc_receiver.data_mask();

    // lookup the destination buffer
    volatile void *dest_buffer = shmem_open_segment(
            const_cast<char*>(dest_buffer_name.c_str()),
            max_data_size
    );
    if (!dest_buffer) {
        std::cerr << "Unable to open shmem buffer " << dest_buffer_name << " with a size of at least ";
        std::cerr << max_data_size << " bytes." << std::endl;
        exit(1);
    }

    int loop = 0;
    dc_receiver.verbose(do_verbose);
    dc_receiver.begin_acquiring();

    size_t cycle_data_size = (max_data_size - sizeof(daq_multi_cycle_header_t))/DAQ_NUM_DATA_BLOCKS_PER_SECOND;

    volatile daq_multi_cycle_data_t* multi_cycle_header = reinterpret_cast<volatile daq_multi_cycle_data_t*>(dest_buffer);
    multi_cycle_header->header.maxCycle = DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    multi_cycle_header->header.cycleDataSize = static_cast<unsigned int>(cycle_data_size);
    volatile char *data_block_start = &(multi_cycle_header->dataBlock[0]);

    //size_t msg_size = 0;
    //char dcstatus[12*4*DCU_COUNT + 1];
    //char dcs[12*4];

    std::atomic<unsigned int>* cycle_ptr = reinterpret_cast<std::atomic<unsigned int>*>(
            const_cast<unsigned int*>(&(multi_cycle_header->header.curCycle))
    );
    unsigned int prev_cylce = DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    do {
        zmq_dc::data_block results = dc_receiver.receive_data();
        unsigned int cur_cycle = results.full_data_block->header.dcuheader[0].cycle;

        // write the data out
        std::copy(reinterpret_cast<char*>(results.full_data_block),
                  reinterpret_cast<char*>(results.full_data_block) + results.send_length,
                  const_cast<char*>(
                          data_block_start + (cur_cycle*cycle_data_size)
                  ));
        *cycle_ptr = cur_cycle;

        //std::string debug_message = create_debug_message(results);
        //std::cout << results.full_data_block->header.dcuheader[0].cycle << ": tpcount " << results.full_data_block->header.dcuheader[0].tpCount << std::endl;

        if (timing_check) {
            gps_time now = clock.now();
            unsigned long nsec = results.full_data_block->header.dcuheader[0].timeNSec;
            nsec *= (1000000000/16);
            gps_time msg_time(
                    results.full_data_block->header.dcuheader[0].timeSec,
                    nsec);
            gps_time delta = now - msg_time;
            std::cout << "Now: " << now << " block: " << msg_time << " delta: " << delta << std::endl;
        }

    } while (keep_running);

    std::cout << "stopping threads " << nsys << std::endl;
    dc_receiver.stop_threads();

    // Wait for threads to stop
    sleep(2);
    std::cout << "closing down" << std::endl;

    return 0;
}

//
///	@file zmq_threads.cc
///	@brief  Translation of zmq_threads to C++
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

#include <zmq.hpp>

#include "zmq_dc_recv.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>


unsigned int do_wait = 0; // Wait for this number of milliseconds before starting a cycle


//std::vector<zmq_dc::receiver_thread_info> thread_info;


static volatile bool keep_running = true;


void
usage()
{
    std::cerr << "Usage: zmq_multi_rcvr [args] -s server name" << std::endl;
    std::cerr << "-l filename - log file name" << std::endl;
    std::cerr << "-s - server name eg x1lsc0, x1susex, etc." << std::endl;
    std::cerr << "-v - verbose prints cpu_meter test data" << std::endl;
    std::cerr << "-p ifacename - interface to do broadcast from, defaults to eth2" << std::endl;
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
    int dcuids = block->dcuTotalModels;
    unsigned long ets = block->zmqheader[dcuids-1].timeSec;
    os << ets << " ";
    for (int i = 0; i < dcuids; ++i) {
        daq_msg_header_t* cur_header = block->zmqheader + i;
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
    std::string pub_iface = "eth2";

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

    while ((c = getopt(argc, argv, "hd:s:p:l:Vvw:x")) != EOF) switch(c) {
            case 's':
                sysname = optarg;
                break;
            case 'v':
                do_verbose = true;
                break;
            case 'w':
                do_wait = atoi(optarg);
                break;
            case 'p':
                pub_iface = optarg;
                break;
            case 'h':
            default:
                usage();
                exit(1);
        }

    if (sysname == nullptr) { usage(); exit(1); }

    signal(SIGINT,intHandler);

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

    // Create 0MQ socket for DC data transmission
    zmq::context_t dc_context;
    zmq::socket_t dc_publisher(dc_context, ZMQ_PUB);

    {
        std::ostringstream os;
        os << "tcp://" << pub_iface << ":" << DAQ_DATA_PORT;
        std::string loc = os.str();
        dc_publisher.bind(loc);
    }

    zmq::context_t de_context;
    zmq::socket_t de_publisher(de_context, ZMQ_PUB);
    {
        std::ostringstream os;
        os << "tcp://" << pub_iface << ":" << 7777;
        std::string loc = os.str();
        de_publisher.bind(loc);
    }

    int loop = 0;
    dc_receiver.verbose(do_verbose);
    dc_receiver.begin_acquiring();

    size_t msg_size = 0;
    char dcstatus[12*4*DCU_COUNT + 1];
    char dcs[12*4];
    do {
        zmq_dc::data_block results = dc_receiver.receive_data();

        std::string debug_message = create_debug_message(results);

        // Xmit the DC data block
        msg_size = dc_publisher.send(reinterpret_cast<char *>(results.full_data_block), results.send_length,0);

        msg_size = de_publisher.send(debug_message.c_str(), debug_message.size(), 0);
    }while (keep_running);

    std::cout << "stopping threads " << nsys << std::endl;
    dc_receiver.stop_threads();

    // Wait for threads to stop
    sleep(2);
    std::cout << "closing down" << std::endl;

    return 0;
}

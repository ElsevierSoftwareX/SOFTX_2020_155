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

#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>


int do_verbose;
unsigned int do_wait = 0; // Wait for this number of milliseconds before starting a cycle

class receiver_thread_info {
public:
    receiver_thread_info(int index, zmq::socket_t&& socket):
            index_(index), socket_(std::move(socket)) {}
    receiver_thread_info(receiver_thread_info&& other): index_(other.index_),
                                                        socket_(std::move(other.socket_)) {}

    receiver_thread_info& operator=(receiver_thread_info&& other) {
        index_ = other.index_;
        socket_ = std::move(other.socket_);
    }
    int index() const { return index_; }
    zmq::socket_t &socket() { return socket_; }

private:
    int index_;
    zmq::socket_t socket_;

    receiver_thread_info();
    receiver_thread_info(const receiver_thread_info& other);
    receiver_thread_info operator=(const receiver_thread_info& other);
};
std::vector<receiver_thread_info> thread_info;

std::array<volatile unsigned int, 16> tstatus;
daq_dc_data_t mxDataBlockFull[16];
daq_multi_dcu_data_t mxDataBlockG[32][16];
int stop_working_threads = 0;
static volatile bool start_acq = false;
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

void *rcvr_thread(void *arg) {
    receiver_thread_info *mythread = reinterpret_cast<receiver_thread_info*>(arg);
    int mt = mythread->index();
    std::cout << "starting receive loop for myarg = " << mt << std::endl;
    int ii;
    int cycle = 0;
    int acquire = 0;
    daq_multi_dcu_data_t mxDataBlock;

    do {
        zmq::message_t message;
        // Get data when message size > 0
        mythread->socket().recv(&message, 0);
        assert(message.size() >= 0);
        // Get pointer to message data
        char *string = reinterpret_cast<char *>(message.data());
        char *daqbuffer = (char *)&mxDataBlock;
        // Copy data out of 0MQ message buffer to local memory buffer
        memcpy(daqbuffer,string,message.size());


        //printf("Received block of %d on %d\n", size, mt);
        for (ii = 0;ii<mxDataBlock.dcuTotalModels;ii++) {
            cycle = mxDataBlock.zmqheader[ii].cycle;
            // Copy data to global buffer
            char *localbuff = (char *)&mxDataBlockG[mt][cycle];
            memcpy(localbuff,daqbuffer,message.size());
        }
        // Always start on cycle 0 after told to start by main thread
        if(cycle == 0 && start_acq) {
            if (acquire != 1) {
                std::cout << "starting to acquire\n";
            }
            acquire = 1;
        }
        // Set the cycle data ready bit
        if(acquire)  {
            tstatus[cycle] |= (1 << mt);
        }
        // if (acquire && cycle == 0)
        //    std::cout << "thread " << mt << " received " << message.size() << " bytes" << std::endl;
        // Run until told to stop by main thread
    } while(!stop_working_threads);
    std::cout << "Stopping thread " << mt << std::endl;
    usleep(200000);
    return(0);

}

/// Create all the subscriber threads
/// \param context The ZMQ context object to use
/// \param thread_info An empty vector of receiver thread info
/// \param sname Vector of system names to connect to
/// \return A data ready mask
int create_subscriber_threads(zmq::context_t& context, std::vector<receiver_thread_info>& thread_info, std::vector<std::string>& sname) {
    // allocate once only
    thread_info.reserve(sname.size());
    // Make 0MQ socket connection
    for(int ii=0;ii<sname.size();ii++) {
        // Make 0MQ socket connection for rcvr threads
        zmq::socket_t subscriber(context, ZMQ_SUB);

        // Subscribe to all data from the server
        subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);

        // connect to the publisher
        {
            std::ostringstream os;
            os << "tcp://" << sname[ii] << ":" << DAQ_DATA_PORT;
            std::string conn_str = os.str();
            std::cout << "Sys connection " << ii << " = " << conn_str << "\n";
            subscriber.connect(conn_str);
        }

        thread_info.emplace_back(receiver_thread_info(ii, std::move(subscriber)));
    }
    int dataRdy = 0;
    // we don't actually do anything with this.
    // but might as well keep it stable and not just reference one hard coded value.
    std::vector<pthread_t> thread_id(sname.size());
    for (int ii = 0; ii < sname.size(); ii++) {
        pthread_create(&thread_id[ii],nullptr,rcvr_thread, reinterpret_cast<void*>(&thread_info[ii]));
        dataRdy |= (1 << ii);
    }
    return dataRdy;
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

    while ((c = getopt(argc, argv, "hd:s:p:l:Vvw:x")) != EOF) switch(c) {
            case 's':
                sysname = optarg;
                break;
            case 'v':
                do_verbose = 1;
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

    std::fill(tstatus.begin(), tstatus.end(), 0);

    std::cout << "nsys = " << nsys << "\n";
    for(ii=0;ii<sname.size();ii++) {
        std::cout << "sys " << ii << " =  " << sname[ii] << "\n";
    }

    zmq::context_t recv_context(nsys);
    int dataRdy = create_subscriber_threads(recv_context, thread_info, sname);

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
    start_acq = true;
    int64_t mytime = 0;
    int64_t mylasttime = 0;
    int64_t myptime = 0;
    int mytotaldcu = 0;
    char *zbuffer;
    int dc_datablock_size = 0;
    char buffer[DAQ_ZMQ_DC_DATA_BLOCK_SIZE];
    static const int header_size = DAQ_ZMQ_HEADER_SIZE;
    int sendLength = 0;
    int msg_size = 0;
    char dcstatus[12*4*DCU_COUNT + 1];
    char dcs[12*4];
    int edcuid[DCU_COUNT];
    int estatus[DCU_COUNT];
    int edbs[DCU_COUNT];
    unsigned long ets = 0;
    int timeout = 0;
    bool resync = true;
    do {
        if(resync) {
            loop = 0;
            resync = false;
            std::fill(tstatus.begin(), tstatus.end(), 0);
        }
        // Wait until received data from at least 1 FE
        timeout = 0;
        do {
            usleep(2000);
            timeout += 1;
        }while(tstatus[loop] == 0 && timeout < 50);
        // If timeout, not getting data from anyone.
        if(timeout >= 50) resync = true;
        if (resync) continue;

        // Wait until data received from everyone
        timeout = 0;
        do {
            usleep(1000);
            timeout += 1;
        }while(tstatus[loop] != dataRdy && timeout < 5);
        // If timeout, not getting data from everyone.
        // TODO: MARK MISSING FE DATA AS BAD

        // Clear thread rdy for this cycle
        tstatus[loop] = 0;

        // Timing diagnostics
        mytime = s_clock();
        myptime = mytime - mylasttime;
        mylasttime = mytime;
        // printf("Data rday for cycle = %d\t%ld\n",loop,myptime);
        // Reset total DCU counter
        mytotaldcu = 0;
        // Set pointer to start of DC data block
        zbuffer = (char *)&mxDataBlockFull[loop].zmqDataBlock[0];
        // Reset total DC data size counter
        dc_datablock_size = 0;
        // Loop over all data buffers received from FE computers
        for(ii=0; ii<nsys; ii++) {
            int cur_sys_dcu_count = mxDataBlockG[ii][loop].dcuTotalModels;
            // printf("\tModel %d = %d\n",ii,cur_sys_dcu_count);
            for(int jj=0; jj<cur_sys_dcu_count; jj++) {
                // Copy data header information
                mxDataBlockFull[loop].zmqheader[mytotaldcu].dcuId = mxDataBlockG[ii][loop].zmqheader[jj].dcuId;
                int cur_dcuid = edcuid[mytotaldcu] = mxDataBlockFull[loop].zmqheader[mytotaldcu].dcuId;
                mxDataBlockFull[loop].zmqheader[mytotaldcu].fileCrc = mxDataBlockG[ii][loop].zmqheader[jj].fileCrc;
                mxDataBlockFull[loop].zmqheader[mytotaldcu].status = mxDataBlockG[ii][loop].zmqheader[jj].status;
                estatus[mytotaldcu] = mxDataBlockFull[loop].zmqheader[mytotaldcu].status;
                if(mxDataBlockFull[loop].zmqheader[mytotaldcu].status == 0xbad)
                    std::cout << "Fault on dcuid " << mxDataBlockFull[loop].zmqheader[mytotaldcu].dcuId << "\n";
                else
                    ets = mxDataBlockG[ii][loop].zmqheader[jj].timeSec;
                mxDataBlockFull[loop].zmqheader[mytotaldcu].cycle = mxDataBlockG[ii][loop].zmqheader[jj].cycle;
                mxDataBlockFull[loop].zmqheader[mytotaldcu].timeSec = mxDataBlockG[ii][loop].zmqheader[jj].timeSec;
                mxDataBlockFull[loop].zmqheader[mytotaldcu].timeNSec = mxDataBlockG[ii][loop].zmqheader[jj].timeNSec;
                int mydbs = mxDataBlockG[ii][loop].zmqheader[jj].dataBlockSize;
                edbs[mytotaldcu] = mydbs;

                //if (loop == 0 && do_verbose)
                //    printf("\t\tdcuid = %d ; data size= %d\n", cur_dcuid, mydbs);

                mxDataBlockFull[loop].zmqheader[mytotaldcu].dataBlockSize = mydbs;
                char *mbuffer = (char *)&mxDataBlockG[ii][loop].zmqDataBlock[0];
                // Copy data to DC buffer
                memcpy(zbuffer,mbuffer,mydbs);
                // Increment DC data buffer pointer for next data set
                zbuffer += mydbs;
                dc_datablock_size += mydbs;
                mytotaldcu ++;
            }
        }

        // printf("\tTotal DCU = %d\tSize = %d\n",mytotaldcu,dc_datablock_size);
        mxDataBlockFull[loop].dcuTotalModels = mytotaldcu;
        sendLength = header_size + dc_datablock_size;

        if (loop == 0 && do_verbose) {
            printf("Recieved %d bytes from %d dcuids\n", dc_datablock_size, mytotaldcu);
        }

        zbuffer = (char *)&mxDataBlockFull[loop];
        // Copy DC data to 0MQ message block
        memcpy(buffer,zbuffer,sendLength);
        // Xmit the DC data block
        msg_size = dc_publisher.send(buffer,sendLength,0);

        sprintf(dcstatus,"%ld ",ets);
        for(ii=0;ii<mytotaldcu;ii++) {
            sprintf(dcs,"%d %d %d ",edcuid[ii],estatus[ii],edbs[ii]);
            strcat(dcstatus,dcs);
        }
        sendLength = sizeof(dcstatus);
        msg_size = de_publisher.send(dcstatus,sendLength,0);

        loop ++;
        loop %= 16;
    }while (keep_running);

    std::cout << "stopping threads " << nsys << std::endl;
    stop_working_threads = 1;

    // Wait for threads to stop
    sleep(2);
    std::cout << "closing down" << std::endl;

    return 0;
}
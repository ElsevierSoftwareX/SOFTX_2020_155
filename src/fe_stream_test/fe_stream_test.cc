//
// Simulate a FE model by producing known output and placing it in the appropriate shared memory block
//

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <strings.h>
#include <unistd.h>

#include "../include/daqmap.h"
#include "../zmq_stream/zmq_daq_core.h"

#include "fe_stream_generator.hh"

#include "gps.hh"

extern "C" {

#include "../drv/rfm.c"

#include "../drv/crc.c"

}


class ChNumDb
{
private:
    int max_;
public:
    ChNumDb(): max_(40000) {}

    int next(int channel_type) { max_++; return max_; }
};

std::string generate_ini_filename(const std::string &ini_dir, const std::string &system_name)
{
    std::ostringstream ss;
    ss << ini_dir << "/" << system_name << ".ini";
    return ss.str();
}

std::string generate_par_filename(const std::string &ini_dir, const std::string &system_name)
{
    std::ostringstream ss;
    ss << ini_dir << "/" << system_name << ".par";
    return ss.str();
}

void output_ini_files(const std::string& ini_dir, const std::string& system_name, std::vector<GeneratorPtr> channels, int dcuid)
{
    using namespace std;

    string fname_ini = generate_ini_filename(ini_dir, system_name);
    string fname_par = generate_par_filename(ini_dir, system_name);
    ofstream os_ini(fname_ini.c_str());
    ofstream os_par(fname_par.c_str());
    os_ini << "[default]\ngain=1.0\nacquire=1\ndcuid=" << dcuid << "\nifoid=0\n";
    os_ini << "datatype=4\ndatarate=2048\noffset=0\nslope=1.0\nunits=undef\n\n";

    vector<GeneratorPtr>::iterator cur = channels.begin();
    for(; cur != channels.end(); ++cur)
    {
        Generator* gen = (cur->get());
        (*cur)->output_ini_entry(os_ini);
    }
}

unsigned int calculate_ini_crc(const std::string& ini_dir, const std::string& system_name)
{
    std::string fname_ini = generate_ini_filename(ini_dir, system_name);
    std::ifstream is(fname_ini.c_str(), std::ios::binary);

    std::vector<char> buffer(64*1024);

    size_t file_size = 0;
    unsigned int file_crc = 0;
    while (is.read(&buffer[0], buffer.size()))
    {
        file_crc = crc_ptr(&buffer[0], buffer.size(), file_crc);
        file_size += buffer.size();
    }
    if (is.gcount() > 0)
    {
        file_crc = crc_ptr(&buffer[0], is.gcount(), file_crc);
        file_size += is.gcount();
    }
    file_crc = crc_len(file_size, file_crc);
    return file_crc;

}


void usage()
{
    using namespace std;
    cout << "Usage: fe_stream_test" << endl;
    cout << "-d dcuid" << endl;
    cout << "-s system name" << endl;
    cout << "-r data rate in MB/s of the simulation [1..256)" << endl;
    cout << "-i directory to put the ini and test point.par files in" << endl;
    cout << "-S allow the use of the system clock" << endl;
}

std::string get_shmem_sysname(const std::string &system_name) {
    std::ostringstream os;
    for (int i = 0; i < system_name.size(); ++i)
    {
        int ch = system_name[i];
        if (isupper(ch))
            os << (char)tolower(ch);
        else if (!(ch == ':' || ch == '-' || ch == '_'))
            os << (char)ch;
    }
    os << "_daq";
    return os.str();
}

int main(int argc, char *argv[]) {
    std::string system_name;
    std::string ini_dir;
    size_t data_rate=1;
    int dcuid;
    bool verbose = false;
    bool system_clock_ok = false;

    {
        int c;
        while ((c = getopt(argc, argv, "d:hs:r:i:vS")) != -1)
        {
            switch (c)
            {
                case 'd':
                    dcuid = std::atoi(optarg);
                    break;
                case 's':
                    system_name = optarg;
                    break;
                case 'r':
                    data_rate = std::atoi(optarg);
                    break;
                case 'i':
                    ini_dir = optarg;
                    break;
                case 'v':
                    verbose = true;
                    break;
                case 'S':
                    system_clock_ok = true;
                    break;
                default:
                    usage();
                    std::exit(1);
            }
        }
        if (system_name.empty() || ini_dir.empty() || data_rate < 1 || data_rate > 256)
        {
            usage();
            std::exit(1);
        }
        if (verbose)
        {
            std::cout << system_name << "@" << data_rate << "MB/s.  Ini/par files go into " << ini_dir << std::endl;
        }
    }
    size_t req_size_bytes = data_rate * 1024*1024;
    size_t channel_count = req_size_bytes / (4*1024);
    size_t block_size = (channel_count*4*1024)/16;
    if (verbose)
        std::cout << "Will create " << channel_count << " channels." << std::endl;

    std::vector<GeneratorPtr> generators;
    generators.reserve(channel_count);

    ChNumDb chDb;

    for (int i = 0; i < channel_count; ++i)
    {
        int chnum = chDb.next(4);

        std::ostringstream ss;
        ss << system_name << "-" << i;
        generators.push_back(GeneratorPtr(new Generators::GPSSecondWithOffset(SimChannel(ss.str(), 4, 1024, chnum), i%21)));
    }

    if (true)
    {
        std::cout << "Channels" << std::endl;
        for (int i = 0; i < generators.size(); ++i)
        {
            std::cout << "\t" << generators[i]->full_channel_name() << std::endl;
        }
    }

    volatile char *shmem = 0;

    std::string shmem_sysname = get_shmem_sysname(system_name);

    shmem = reinterpret_cast<volatile char*>(findSharedMemory(const_cast<char*>(shmem_sysname.c_str())));
    if (shmem == 0)
        throw std::runtime_error("Received null shmem pointer");

    volatile struct rmIpcStr* ipc = reinterpret_cast<volatile struct rmIpcStr*>(shmem + CDS_DAQ_NET_IPC_OFFSET);
    volatile char *data = shmem + CDS_DAQ_NET_DATA_OFFSET;
    volatile struct cdsDaqNetGdsTpNum* tpData = reinterpret_cast<volatile struct cdsDaqNetGdsTpNum*>(shmem + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);

    output_ini_files(ini_dir, system_name, generators, dcuid);
    unsigned int ini_crc = calculate_ini_crc(ini_dir, system_name);

    GPS::gps_clock clock(0);
    if (!clock.ok())
    {
        if (system_clock_ok)
            std::cout << "Using system clock not Symmetricom driver" << std::endl;
        else
            throw std::runtime_error("Clock not synced to GPS time.");
    }

    bool buffer_ready = false;

    ipc->cycle = -1;
    ipc->dcuId = dcuid;
    ipc->crc = ini_crc;
    ipc->command = 0;
    ipc->cmdAck = 0;
    ipc->request = 0;
    ipc->reqAck = 0;
    ipc->status = 0xbad;
    ipc->channelCount = generators.size();
    ipc->dataBlockSize = block_size;
    for (int i = 0; i < DAQ_NUM_DATA_BLOCKS; ++i)
    {
        ipc->bp[i].status = 0xbad;
        ipc->bp[i].timeSec = 0;
        ipc->bp[i].timeNSec = 0;
        ipc->bp[i].run = 0;
        ipc->bp[i].cycle = 0;
        ipc->bp[i].crc = block_size;
    }

    GPS::gps_time time_step = GPS::gps_time(0, 1000000000/16);
    GPS::gps_time transmit_time = clock.now();
    ++transmit_time.sec;
    transmit_time.nanosec = 0;

    int cycle = 0;

    while (true)
    {
        // The simulation writes 1/16s behind real time
        // So we wait until the cycle start, then compute
        // then write and wait for the next cycle.
        GPS::gps_time now = clock.now();
        while (now < transmit_time)
        {
            usleep(1);
            now = clock.now();
        }

        if (verbose && cycle == 0)
        {
            std::cout << ". " << shmem_sysname << " " << transmit_time << " " << now << std::endl;
        }

        volatile char *data_cur = data + cycle*block_size;
        volatile char *data_end = data_cur + block_size;

        for (int i = 0; i < generators.size(); ++i)
        {
            data_cur = generators[i]->generate(transmit_time.sec, transmit_time.nanosec, (char*)data_cur);
            if (data_cur > data_end)
                throw std::range_error("Generator exceeded its output boundary");
        }

        ipc->status = 0;
        ipc->bp[cycle].status = 2;
        ipc->bp[cycle].timeSec = transmit_time.sec;
        ipc->bp[cycle].timeNSec = transmit_time.nanosec;
        ipc->bp[cycle].cycle = cycle;

        ipc->cycle = cycle;

        cycle = (cycle + 1) % 16;
        transmit_time = transmit_time + time_step;

    }
    return 0;
}
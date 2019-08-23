//
// Simulate a FE model by producing known output and placing it in the appropriate shared memory block
//

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <strings.h>
#include <unistd.h>

#include "../include/daqmap.h"
#include "../include/daq_core.h"

#include "fe_stream_generator.hh"
#include "fe_generator_support.hh"

#include "gps.hh"

#include "str_split.hh"

extern "C" {

#include "../drv/rfm.c"

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
    cout << "-t test point list - a space separated set of test point numbers" << endl;
    cout << "-f - write a set of .ini/.par mark data as bad, do not generate data" << endl;
    cout << "-D delay - delay writting data for the specified number of ms" << endl;
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

int str_to_int(const std::string& input)
{
    int value = 0;
    std::istringstream os(input);
    os >> value;
    return value;
}

int main(int argc, char *argv[]) {
    std::string system_name;
    std::string ini_dir;
    std::vector<int> tp_table;
    size_t data_rate_bytes=1;
    int data_rate = 2048;
    int dcuid;
    bool verbose = false;
    bool system_clock_ok = false;
    bool failed_node = false;
    int delay_multiplier = 0;

    {
        int c;
        while ((c = getopt(argc, argv, "D:d:hs:r:i:vSt:f")) != -1)
        {
            switch (c)
            {
                case 'D':
                    delay_multiplier = std::atoi(optarg);
                    break;
                case 'd':
                    dcuid = std::atoi(optarg);
                    break;
                case 's':
                    system_name = optarg;
                    break;
                case 'r':
                    data_rate_bytes = std::atoi(optarg);
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
                case 't':
                    {
                        std::vector<std::string> tp_strings = split(optarg, " ", EXCLUDE_EMPTY_STRING);
                        tp_table.clear();
                        tp_table.reserve(tp_strings.size());
                        std::transform(tp_strings.begin(), tp_strings.end(), std::back_inserter(tp_table), str_to_int);
                    }
                    break;
                case 'f':
                    failed_node = true;
                    break;
                default:
                    usage();
                    std::exit(1);
            }
        }
        if (system_name.empty() || ini_dir.empty() || data_rate_bytes < 1 || data_rate_bytes > 256)
        {
            usage();
            std::exit(1);
        }
        if (verbose)
        {
            std::cout << system_name << "@" << data_rate_bytes << "MB/s.  Ini/par files go into " << ini_dir << std::endl;
        }
    }
    size_t req_size_bytes = data_rate_bytes * 1024*1024;
    size_t channel_count = req_size_bytes / (4*data_rate);
    size_t block_size = (channel_count*4*data_rate)/16;
    if (verbose)
        std::cout << "Will create " << channel_count << " channels." << std::endl;

    std::vector<GeneratorPtr> generators;
    generators.reserve(channel_count);
    std::vector<GeneratorPtr> tp_generators;
    tp_generators.reserve(tp_table.size());

    ChNumDb chDb;
    ChNumDb tpDb(0);

    for (int i = 0; i < channel_count; ++i)
    {
        int chnum = chDb.next(4);

        std::ostringstream ss;
        ss << system_name << "-" << i;
        generators.push_back(GeneratorPtr(new Generators::GPSSecondWithOffset<int>(SimChannel(ss.str(), 2, data_rate, chnum), (i + dcuid)%21)));
    }
    for (int i = 0; i < 32; ++i)
    {
        int chnum = tpDb.next(4);

        std::ostringstream ss;
        ss << system_name << "-TP" << i;
        // TP need truncated
        tp_generators.push_back(GeneratorPtr(new Generators::GPSMod100kSecWithOffset<float>(SimChannel(ss.str(), 4, data_rate, chnum, dcuid), (i + dcuid)%21)));
    }
    GeneratorPtr null_tp = GeneratorPtr(new Generators::StaticValue<float>(SimChannel("null_tp_value", 4, data_rate, 0x7fffffff), 0.0));

    if (true)
    {
        std::cout << "Channels" << std::endl;
        for (int i = 0; i < generators.size(); ++i)
        {
            std::cout << "\t" << generators[i]->full_channel_name() << std::endl;
        }
        std::cout << "Test Points" << std::endl;
        for (int i = 0; i < tp_generators.size(); ++i)
        {
            std::cout << "\t" << tp_generators[i]->full_channel_name() << std::endl;
        }
    }

    volatile char *shmem = 0;

    std::string shmem_sysname = get_shmem_sysname(system_name);

    shmem = reinterpret_cast<volatile char*>(findSharedMemory(const_cast<char*>(shmem_sysname.c_str())));
    if (shmem == 0)
        throw std::runtime_error("Received null shmem pointer");

    volatile struct rmIpcStr* ipc = reinterpret_cast<volatile struct rmIpcStr*>(shmem + CDS_DAQ_NET_IPC_OFFSET);
    volatile struct cdsDaqNetGdsTpNum* tpData = reinterpret_cast<volatile struct cdsDaqNetGdsTpNum*>(shmem + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
    volatile char *data = shmem + CDS_DAQ_NET_DATA_OFFSET;

    output_ini_files(ini_dir, system_name, generators, tp_generators, dcuid);
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
    tpData->count = tp_table.size();
    for (int i = 0; i < tp_table.size(); ++i)
    {
        tpData->tpNum[i] = tp_table[i];
    }

    GPS::gps_time time_step = GPS::gps_time(0, 1000000000/16);
    GPS::gps_time transmit_time = clock.now();
    ++transmit_time.sec;
    transmit_time.nanosec = 0;

    int cycle = 0;

    if (failed_node)
    {
        // stay around just to keep the mbuf open
//        while(true)
//        {
//            sleep(1);
//        }
        return 0;
    }

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
    	usleep(delay_multiplier * 1000);

        if (verbose && cycle == 0)
        {
            std::cout << ". " << shmem_sysname << " " << transmit_time << " " << now << std::endl;
        }

        // The block size is the maximal block size * 2 (to allow for TP data)
        volatile char *data_cur = data + cycle*(DAQ_DCU_BLOCK_SIZE * 2);
        volatile char *data_end = data_cur + (DAQ_DCU_BLOCK_SIZE * 2);

        for (int i = 0; i < generators.size(); ++i)
        {
            data_cur = generators[i]->generate(transmit_time.sec, transmit_time.nanosec, (char*)data_cur);
            if (data_cur > data_end)
                throw std::range_error("Generator exceeded its output boundary");
        }
        for (int i = 0; i < tp_table.size(); ++i)
        {
            int index = tp_table[i];
            if (index == 0)
            {
                data_cur = null_tp->generate(transmit_time.sec, transmit_time.nanosec, (char*)data_cur);
            }
            else
            {
                data_cur = tp_generators[index-1]->generate(transmit_time.sec, transmit_time.nanosec, (char*)data_cur);
                if (verbose && cycle == 0)
                    std::cout << "tp " << index << ") " << tp_generators[index-1]->full_channel_name() << std::endl;
            }
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

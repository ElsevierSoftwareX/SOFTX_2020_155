//
// Simulate a FE model by producing known output and placing it in the appropriate shared memory block
//

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "fe_stream_generator.hh"

extern "C" {

extern volatile void* findSharedMemory(char* sys_name);

}

class ChNumDb
{
private:
    int max_;
public:
    ChNumDb(): max_(40000) {}

    int next(int channel_type) { max_++; return max_; }
};

class SimChannel
{
private:
    std::string name_;
    int chtype_;
    int rate_;
    int chnum_;
public:
    SimChannel(): name_(""), chtype_(0), rate_(0), chnum_(0) {}
    SimChannel(std::string name, int chtype, int rate, int chnum):
            name_(name), chtype_(0), rate_(rate), chnum_(chnum) {}
    SimChannel(const SimChannel& other):
            name_(other.name_), chtype_(other.chtype_), rate_(other.rate_),
            chnum_(other.chnum_) {}

    SimChannel& operator=(const SimChannel& other)
    {
        if (this != &other)
        {
            name_ = other.name_;
            chtype_ = other.chtype_;
            rate_ = other.rate_;
            chnum_ = other.chnum_;
        }
        return *this;
    }

    const std::string& name() const { return name_; }
};

void usage()
{
    using namespace std;
    cout << "Usage: fe_stream_test" << endl;
    cout << "-s system name" << endl;
    cout << "-r data rate in MB/s of the simulation [1..256)" << endl;
    cout << "-i directory to put the ini and test point.par files in" << endl;
}

int main(int argc, char *argv[]) {
    std::string system_name;
    std::string ini_dir;
    size_t data_rate=1;
    bool verbose = false;

    {
        int c;
        while ((c = getopt(argc, argv, "hs:r:i:v")) != -1)
        {
            switch (c)
            {
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
    size_t channel_count = req_size_bytes / (4*8*1024);
    if (verbose)
        std::cout << "Will create " << channel_count << " channels." << std::endl;

    std::vector<SimChannel> channels;
    std::vector<GeneratorPtr> generators;
    channels.reserve(channel_count);
    generators.reserve(channel_count);

    ChNumDb chDb;

    for (int i = 0; i < channel_count; ++i)
    {
        int chnum = chDb.next(4);

        std::ostringstream ss;
        ss << system_name << "-CHANNEL_" << i;
        channels.push_back(SimChannel(ss.str(), 4, 8*1024, chnum));
        generators.push_back(create_generator("gps_sec", channels.back().name(), 4, 8*1024));
    }

    if (true)
    {
        std::cout << "Channels" << std::endl;
        for (int i = 0; i < channels.size(); ++i)
        {
            std::cout << "\t" << channels[i].name() << std::endl;
        }
    }
    return 0;
}
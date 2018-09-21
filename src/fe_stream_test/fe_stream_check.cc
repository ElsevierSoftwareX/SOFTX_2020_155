#include <arpa/inet.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <nds.hh>

#include "fe_stream_generator.hh"

struct Config {
    std::string hostname;
    unsigned short port;
    NDS::buffer::gps_second_type gps;
    std::vector<std::string> channels;
};

void usage()
{
    using namespace std;
    cerr << "Usage:\n";
    cerr << "\t<server> <port> 'live'|gps_time channellist" << std::endl;
    exit(1);
}

Config get_config(int argc, char *argv[])
{
    Config cfg;
    if (argc < 5)
        throw std::runtime_error("Insufficient arguments");
    cfg.hostname = argv[1];
    {
        std::istringstream is(argv[2]);
        is >> cfg.port;
    }
    if (std::strcmp("live", argv[3]) == 0) {
        cfg.gps = 0;
    } else {
        std::istringstream is(argv[3]);
        is >> cfg.gps;
    }
    for (int i = 4; i < argc; ++i)
    {
        cfg.channels.push_back(argv[i]);
    }
    return cfg;
}

int main(int argc, char *argv[])
{
    Config cfg;
    std::vector<GeneratorPtr> generators;
    try {
        cfg = get_config(argc, argv);
    } catch(...) {
        usage();
    }

    generators.reserve(cfg.channels.size());
    for (int i = 0; i < cfg.channels.size(); ++i)
        generators.push_back(create_generator(cfg.channels[i]));

    int status = 0;

    std::cout << "Starting check at " << cfg.gps << std::endl;

    std::tr1::shared_ptr<NDS::connection> conn(new NDS::connection(cfg.hostname, cfg.port));
    for (int i = 0; i < generators.size(); ++i)
    {
        Generator& cur_gen = *(generators[i].get());
        NDS::connection::channel_names_type chlist;
        chlist.push_back(generators[i]->full_channel_name());

        NDS::buffers_type bufs;
        if (cfg.gps == 0)
        {

        }
        else
        {
            bufs = conn->fetch(cfg.gps, cfg.gps+1, chlist);
        }
        NDS::buffer& sample_buf = *(bufs[0].get());
        if (sample_buf.samples_to_bytes(sample_buf.Samples()) != cur_gen.bytes_per_sec())
        {
            std::cerr << "Length mismatch on " << cur_gen.full_channel_name() << std::endl;
            status = 1;
            continue;
        }
        std::vector<char> ref_buf(cur_gen.bytes_per_sec());

        {
            char *out = &ref_buf[0];
            int gps_sec = bufs[0]->Start();
            int gps_nano = bufs[0]->StartNano();
            int step = 1000000000/16;
            for (int j = 0; j < 16; ++j)
            {
                out = cur_gen.generate(gps_sec, gps_nano, out);
                gps_nano += step;
            }
        }

        if (std::memcmp(&ref_buf[0], &sample_buf[0], ref_buf.size()) != 0)
        {
            std::cerr << "There is a difference in " << cur_gen.full_channel_name() << std::endl;
            status = 1;
            std::vector<int> bswap_buff(ref_buf.size()/4);
            int* cur = (int*)(&ref_buf[0]);
            for (int j = 0; j < bswap_buff.size(); ++j)
            {
                bswap_buff[j] = htonl(*cur);
                ++cur;
            }
            if (std::memcmp(&bswap_buff[0], &sample_buf[0], ref_buf.size()) == 0)
            {
                std::cerr << "Byte order difference!!!!" << std::endl;
            }
        }
    }
    return status;
}

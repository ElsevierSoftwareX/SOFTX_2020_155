//
// Created by jonathan.hanks on 8/2/19.
//
#include <algorithm>
#include <deque>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <unistd.h>

#include <drv/shmem.h>
#include "daqmap.h"

struct model_stats {
    unsigned int dcuid;
    unsigned int cycle;
    unsigned int gps_sec;
    unsigned int gps_nsec;
    unsigned int size;
};

struct model
{
    model(): name(""), mbuf_name(""), data(0) {}
    explicit model(const std::string& mod_name): name(mod_name),
      mbuf_name(mod_name + "_daq"), data(0) {}
    model(const model& other): name(other.name),
      mbuf_name(other.mbuf_name), data(other.data) {}

    model& operator=(const model& other)
    {
        name = other.name;
        mbuf_name = other.mbuf_name;
        data = other.data;
    }

    volatile char*
    open()
    {
        if (data)
        {
            return data;
        }
        if (mbuf_name.empty())
        {
            return 0;
        }
        data = (volatile char*)shmem_open_segment(mbuf_name.c_str(), 64*1024*1024);
        return data;
    }

    std::string name;
    std::string mbuf_name;
    volatile char* data;
};

struct config_t {
    config_t(): models(),
    all_samples(false),
    show_help(false),
    error_message("")
    {}

    void
    set_error(const char* msg)
    {
        show_help = true;
        error_message = msg;
    }

    void
    consistency_check()
    {
        if (models.empty())
        {
            set_error("You must specify at least one model");
        }
    }

    std::vector<std::string> models;
    bool all_samples;
    bool show_help;
    std::string error_message;
};

config_t
parse_args(int argc, char* argv[])
{
    config_t opts;

    std::deque<std::string> args;
    for (int i = 1; i < argc; ++i)
    {
        args.push_back(argv[i]);
    }
    while (!args.empty())
    {
        std::string arg = args.front();
        args.pop_front();
        if (arg == "-h" || arg == "--help")
        {
            opts.show_help = true;
            break;
        }
        else if (arg == "-s")
        {
            if (args.empty())
            {
                opts.set_error("You must specify a model name when using -s");
                break;
            }
            opts.models.push_back(args.front());
            args.pop_front();
        }
        else if (arg == "-a")
        {
            opts.all_samples = true;
        }
        else
        {
            opts.set_error("Unknown option");
        }
    }
    opts.consistency_check();
    return opts;
}

void
usage(const char* progname)
{
    std::cout << "Usage:\n\t" << progname << " [options]\n\n";
    std::cout << "Where options is one of:\n";
    std::cout << "-h,--help\t\tThis help\n";
    std::cout << "-s [model]\t\tSpecify a model\n";
    std::cout << "-a\t\tTry to sample all cycles\n";
    std::cout << "\nMore than one instance of -s may be used.\n";
}

model
to_model(const std::string& s)
{
    model m(s);
    m.open();
    return m;
}

model_stats
extract_stats(const model& m)
{
    model_stats stats;

    volatile rmIpcStr* ipc = (volatile rmIpcStr*)(m.data);

    stats.dcuid = ipc->dcuId;
    stats.cycle = ipc->cycle;
    if (stats.cycle >= 16)
    {
        stats.cycle = 0xff;
        stats.size = 0;
        stats.gps_sec = 0;
        stats.gps_nsec = 0;
    }
    else
    {
        stats.size = ipc->bp[stats.cycle].crc;
        stats.gps_sec = ipc->bp[stats.cycle].timeSec;
        stats.gps_nsec = ipc->bp[stats.cycle].timeNSec;
    }
    return stats;
}

void
wait_for_cycle(int target, std::vector<model>& models)
{
    while (true)
    {
        for (int i = 0; i < models.size(); ++i)
        {
            volatile rmIpcStr* ipc = (volatile rmIpcStr*)(models[i].data);
            if (ipc->cycle == target)
            {
                return;
            }
        }
        usleep(5000);
    }
}


int
main(int argc, char* argv[])
{
    config_t opts = parse_args(argc, argv);
    if (opts.show_help)
    {
        usage(argv[0]);
        if (!opts.error_message.empty())
        {
            std::cout << opts.error_message << "\n";
            return 1;
        }
    }

    std::vector<model> models;
    std::transform(opts.models.begin(), opts.models.end(), std::back_inserter(models), to_model);

    std::vector<model_stats> stats(models.size());

    int next_cycle = 0;
    while (true)
    {
        if (opts.all_samples)
        {
            wait_for_cycle(next_cycle, models);
            usleep(2000);
            next_cycle++;
            next_cycle %= 16;
        }
        else
        {
          sleep(1);
        }

        std::transform(models.begin(), models.end(), stats.begin(), extract_stats);
        for (int i = 0; i < models.size(); ++i)
        {
            model& m = models[i];
            model_stats& s = stats[i];
            std::cout << m.name << ": dcuid: " << s.dcuid << " cycle: " << s.cycle;
            std::cout << " size: " << s.size << " gps: " << s.gps_sec << ":" << s.gps_nsec << "\n";
        }
        std::cout << "\n";
    }
}
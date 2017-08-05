//
// Created by jonathan.hanks on 7/26/17.
//
#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
//#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <zmq.hpp>
#include "../zmq_daq.h"

#include "firehose_structs.hh"
#include "zmq_firehose_common.hh"

struct Config {
    std::string bind_point;
    std::string master_filename;
    int dcu_count;
    int chans_per_dcu;
    int max_generators;
    std::mt19937_64::result_type random_seed;
    size_t data_size;
    int rate_limit;
    int recovery_ms;
    int threads;
    int chunk_size;
    bool verbose;
    bool extra_verbose;
    bool zero_copy;

    Config(): bind_point{"tcp://*:5555"},
              master_filename{"master_file"},
              dcu_count{5}, chans_per_dcu {5},
              max_generators{50},
              random_seed{static_cast<std::mt19937_64::result_type >(std::time(nullptr))},
              data_size{1024*1024*64},
              rate_limit{0},
              recovery_ms{10000},
              threads{1},
              chunk_size{100*1024*1024},
              verbose{false},
              extra_verbose{false},
              zero_copy{false}
    {}
    Config(const Config& other) = default;
    Config(Config&& other) = default;
    Config& operator=(const Config& other) = default;
    Config& operator=(Config&& other) = default;
};

Config parse_args(int argc, char **argv) {
    Config cfg{};
    for (int i = 1; i < argc; ++i) {
        std::string arg{argv[i]};
        std::string next_arg;
        bool has_next = false;
        if (i +1 < argc) {
            next_arg = argv[i+1];
            has_next = true;
        }
        if (arg == "--bind") {
            if (has_next) {
                cfg.bind_point = next_arg;
                ++i;
            }
        } else if (arg == "--size") {
            if (has_next) {
                std::istringstream is{next_arg};
                is >> cfg.data_size;
                ++i;
            }
        } else if (arg == "--rate-limit") {
            if (has_next) {
                std::istringstream is{next_arg};
                is >> cfg.rate_limit;
                ++i;
            }
            std::cout << "rate limit = " << cfg.rate_limit << std::endl;
        } else if (arg == "--recovery-ms") {
            if (has_next) {
                std::istringstream is{next_arg};
                is >> cfg.recovery_ms;
                ++i;
            }
        } else if (arg == "--threads") {
            if (has_next) {
                std::istringstream is{next_arg};
                is >> cfg.threads;
                ++i;
            }
        } else if (arg == "-v" || arg == "--verbose") {
            cfg.verbose = true;
        } else if (arg == "-vv") {
            cfg.verbose = true;
            cfg.extra_verbose = true;
        } else if (arg == "-z" || arg == "--zero-copy") {
            cfg.zero_copy = true;
        } else if (arg == "--chunk") {
            if (has_next) {
                std::istringstream is{next_arg};
                is >> cfg.chunk_size;
                ++i;
            }
        } else {
            std::cerr << "Unknown argument " << arg << std::endl;
        }
    }
    return cfg;
};

void simple_send_loop(zmq::socket_t& publisher, const Config& config) {
    if (config.data_size < 1024) {
        std::cerr << "Data size is too small, please try at least 1KB" << std::endl;
        return;
    }
    const int segments = 16;
    const int max_gps_n = 1000 * 1000 * 1000;
    const int step = max_gps_n/segments;

    long gps=0;
    long gps_n=0;

    if (config.verbose) {
        std::cout << "Sending " << config.data_size * 16 << " bytes in 1/16s chunks" << std::endl;
    }

    // initialize a full seconds worth of buffers
    std::array<std::vector<char>, segments> buffers;
    {
        // just give each segment a different value
        char val = 0;
        for (auto& entry:buffers) {
            entry.resize(config.data_size, val);
            ++val;
        }
    }

    get_time(gps, gps_n);
    gps++;
    gps_n = 0;
    int cur_segment = 0;

    while(true) {
        wait_for(gps, gps_n);

        int parts = 0;
        size_t total_size = 0;

        if (config.zero_copy) {
            zmq::message_t msg(buffers[cur_segment].data(), config.data_size, nullptr, nullptr);
            long* tmp = reinterpret_cast<long*>(buffers[cur_segment].data());
            tmp[0] = gps;
            tmp[1] = gps_n;
            publisher.send(msg);
            parts = 1;
            total_size = config.data_size;
        } else {
            size_t cur = 0;
            const size_t chunk_size = config.chunk_size;
            while (cur < config.data_size) {
                size_t end = cur + chunk_size;
                bool last_message = false;
                if (end >= config.data_size) {
                    last_message = true;
                    end = config.data_size;
                }
                size_t message_size = end - cur;
                total_size += message_size;
                zmq::message_t msg(message_size);
                char *data = reinterpret_cast<char *>(msg.data());
                std::fill(data, data + message_size, static_cast<char>(cur_segment));
                if (cur == 0) {
                    long *tmp = reinterpret_cast<long *>(data);
                    tmp[0] = gps;
                    tmp[1] = gps_n;
                }
                publisher.send(msg, (last_message ? 0 : ZMQ_SNDMORE));
                if (config.extra_verbose)
                    std::cerr << "\t" << message_size << "-" << (last_message ? 0 : ZMQ_SNDMORE) << "\n";
                parts++;

                cur = end;
            }
        }

        ++cur_segment;
        if (cur_segment >= segments) {
            cur_segment = 0;
        }
        gps_n += step;
        if (gps_n >= max_gps_n) {
            ++gps;
            gps_n = 0;
        }

        {
            long cur_gps, cur_gps_n;
            get_time(cur_gps, cur_gps_n);
            if (cur_gps > gps || (cur_gps == gps && cur_gps_n >= gps_n)) {
                std::cerr << "Late wanted " << gps << ":" << gps_n << " got " << cur_gps << ":" << cur_gps_n << std::endl;
            } else if (config.verbose) {
                std::cout << "Sent " << total_size << " bytes at " << gps << ":" << gps_n << " by " << cur_gps << ":" << cur_gps_n << " in " << parts << " parts\n";
            }
        }
    }
}

int main(int argc, char **argv) {
    Config cfg = parse_args(argc, argv);

    std::cout << "Seeding with " << cfg.random_seed << std::endl;
    std::mt19937_64 rand_generator{cfg.random_seed};

//    for (int i = 0; i < cfg.dcu_count; ++i) {
//        std::cout << "Generating dcu " << i << std::endl;
//
//    }

//    std::cout << "Timing a lot of sin calls" << std::endl;
//    auto t0 = std::chrono::steady_clock::now();
//    for (auto i = 0; i < 100000; ++i) {
//        std::sin(static_cast<double>(1.5));
//    }
//    auto t1 = std::chrono::steady_clock::now();
//    std::chrono::duration<double> delta = t1-t0;
//
//    std::cout << "Did lots of sin calls in " << delta.count() << " units of time" << std::endl;

    zmq::context_t context(cfg.threads);
    zmq::socket_t publisher(context, ZMQ_PUB);

    {
        if (cfg.rate_limit > 0) {
            std::cout << "Setting rate limit to " << cfg.rate_limit << "Kb/s" << std::endl;
            publisher.setsockopt(ZMQ_RATE, &cfg.rate_limit, sizeof(cfg.rate_limit));
        }
        std::cout << "Setting recovery_ivl to " << cfg.recovery_ms << std::endl;
        publisher.setsockopt(ZMQ_RECOVERY_IVL, &cfg.recovery_ms, sizeof(cfg.recovery_ms));
        int rate = 0;
        size_t rate_size = sizeof(rate);
        publisher.getsockopt(ZMQ_RATE, &rate, &rate_size);
        std::cout << "Rate limit confirmed at " << rate << "Kb/s" << std::endl;
    }
    if (cfg.verbose)
        std::cout << "Binding to " << cfg.bind_point << std::endl;
    publisher.bind(cfg.bind_point.c_str());
    if (cfg.verbose)
        std::cout << "Starting send loop" << std::endl;
    simple_send_loop(publisher, cfg);
    return 1;
}

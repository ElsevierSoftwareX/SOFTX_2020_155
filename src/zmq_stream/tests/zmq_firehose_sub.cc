//
// Created by jonathan.hanks on 7/26/17.
//
#include <array>
#include <cmath>
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
    int rate_limit;
    int threads;
    bool verbose;
    bool extra_verbose;

    Config(): bind_point{"tcp://127.0.0.1:5555"},
              rate_limit{0},
              threads{1},
              verbose{false},
              extra_verbose{false}
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
        if (arg == "--connect") {
            if (has_next)
                cfg.bind_point = next_arg;
        }
        if (arg == "--threads") {
            if (has_next) {
                std::istringstream is{next_arg};
                is >> cfg.threads;
                ++i;
            }
        }
        if (arg == "-v" || arg == "--verbose") {
            cfg.verbose = true;
        }
        if (arg == "-vv") {
            cfg.verbose = true;
            cfg.extra_verbose = true;
        }
        if (arg == "--rate-limit") {
            if (has_next) {
                std::istringstream is{next_arg};
                is >> cfg.rate_limit;
                ++i;
            }
        }
    }
    return cfg;
};

void simple_recv_loop(zmq::socket_t& subscriber, const Config& config) {

    const int segments = 16;
    const int max_gps_n = 1000 * 1000 * 1000;
    const int step = max_gps_n/segments;

    long expected_gps=0;
    long expected_gps_n=0;

    while(true) {
        zmq::message_t msg(0);
        size_t total_size = 0;

        long gps = 0, gps_n = 0;
        bool error = false;
        bool first = true;

        int parts = 0;
        int more_parts = 0;
        size_t opt_len = sizeof(more_parts);
        do {
            subscriber.recv(&msg);
            ++parts;

            if (first) {
                long *tmp = reinterpret_cast<long *>(msg.data());
                gps = tmp[0];
                gps_n = tmp[1];
                first = false;
            }

            total_size += msg.size();
            opt_len = sizeof(more_parts);
            subscriber.getsockopt(ZMQ_RCVMORE, &more_parts, &opt_len);
            if (config.extra_verbose)
                std::cerr << "\t" << msg.size() << "-" << more_parts << "\n";
        } while (more_parts != 0);

        if (expected_gps != 0) {
            if (expected_gps != gps || expected_gps_n != gps_n) {
                std::cerr << "ERROR: expecting " << expected_gps << ":" << expected_gps_n << " got " << gps << ":"  << gps_n << " instead." << std::endl;
            }
        }
        expected_gps = gps;
        expected_gps_n = gps_n + step;
        if (expected_gps_n >= max_gps_n) {
            ++expected_gps;
            expected_gps_n = 0;
        }

        if (config.verbose) {
            std::cout << "Recieved " << total_size << " bytes for " << gps << ":" << gps_n << " in " << parts << " parts\n";
        }
    }
}

int main(int argc, char **argv) {
    Config cfg = parse_args(argc, argv);

    zmq::context_t context(cfg.threads);
    zmq::socket_t subscriber(context, ZMQ_SUB);

    {
        if (cfg.rate_limit > 0) {
            std::cout << "Setting rate limit to " << cfg.rate_limit << "Kb/s" << std::endl;
            subscriber.setsockopt(ZMQ_RATE, &cfg.rate_limit, sizeof(cfg.rate_limit));
        }
        subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);
        int rate = 0;
        size_t rate_size = sizeof(rate);
        subscriber.getsockopt(ZMQ_RATE, &rate, &rate_size);
        std::cout << "Rate limit confirmed at " << rate << "Kb/s" << std::endl;
    }

    std::cout << "connecting to " << cfg.bind_point << std::endl;
    subscriber.connect(cfg.bind_point.c_str());

    std::cout << "Beginning receive loop " << std::endl;
    simple_recv_loop(subscriber, cfg);
    return 1;
}

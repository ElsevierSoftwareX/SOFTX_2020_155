//
// Created by jonathan.hanks on 7/26/17.
//
#include <cmath>
#include <ctime>
#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include <zmq.hpp>
#include "../zmq_daq.h"

#include "firehose_structs.hh"

struct Config {
    std::string bind_point;
    std::string master_filename;
    int dcu_count;
    int chans_per_dcu;
    int max_generators;
    std::mt19937_64::result_type random_seed;

    Config(): bind_point{"tcp://*:55555"},
              master_filename{"master_file"},
              dcu_count{5}, chans_per_dcu {5},
              max_generators{50},
              random_seed{static_cast<std::mt19937_64::result_type >(std::time(nullptr))}
    {}
    Config(const Config& other) = default;
    Config(Config&& other) = default;
    Config& operator=(const Config& other) = default;
    Config& operator=(Config&& other) = default;
};

std::string get_publisher_address() {
    std::ostringstream os;
    os << "tcp://*:" << DAQ_DATA_PORT;
    return os.str();
}

Config parse_args(int argc, char **argv) {
    return Config{};
};

int main(int argc, char **argv) {
    Config cfg = parse_args(argc, argv);

    std::cout << "Seeding with " << cfg.random_seed << std::endl;
    std::mt19937_64 rand_generator{cfg.random_seed};

    for (int i = 0; i < cfg.dcu_count; ++i) {
        std::cout << "Generating dcu " << i << std::endl;

    }

    std::cout << "Timing a lot of sin calls" << std::endl;
    auto t0 = std::chrono::steady_clock::now();
    for (auto i = 0; i < 100000; ++i) {
        std::sin(static_cast<double>(1.5));
    }
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> delta = t1-t0;

    std::cout << "Did lots of sin calls in " << delta.count() << " units of time" << std::endl;

    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);

    publisher.bind(get_publisher_address().c_str());
    return 1;
}

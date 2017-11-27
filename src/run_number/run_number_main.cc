#include <iostream>
#include <string>
#include <vector>
#include <zmq.hpp>
#include "run_number.hh"
#include "run_number_structs.h"

struct config {
    std::string endpoint;
    bool verbose;
    bool test_crash;

    config(): endpoint("tcp://*:5556"), verbose(false), test_crash(false) {}
    config(const config& other): endpoint(other.endpoint), verbose(other.verbose), test_crash(other.test_crash) {}
    config& operator=(const config& other) {
        endpoint = other.endpoint;
        verbose = other.verbose;
        test_crash = other.test_crash;
        return *this;
    }
};

void send_zero_response(zmq::socket_t &s) {
    daqd_run_number_resp_v1_t resp;
    bzero(&resp, sizeof(resp));
    resp.version = 1;
}

bool parse_args(int argc, char *argv[], config& cfg) {
    std::string prog_name((argc >= 1 ? argv[0] : "unknown"));

    bool need_help = false;
    bool endpoint_assigned = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-v") {
            cfg.verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            need_help = true;
            break;
        } else if (arg == "--test-crash") {
            cfg.test_crash = true;
            break;
        } else {
            if (endpoint_assigned) {
                need_help = true;
                break;
            }
            cfg.endpoint = arg;
            endpoint_assigned = true;
        }
    }
    if (need_help) {
        std::cerr << "Usage:\n\t" << prog_name << " [options] [endpoint]\n\n";
        std::cerr << "Where options are:\n\t-v\tVerbose\n\t--test-crash\t";
        std::cerr << "A debug/test mode where the server does not complete a request.  DO NOT USE in production\n";
        std::cerr << "Endpoint defaults to '" << cfg.endpoint << "' if not specified" << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    config cfg;
    if (!parse_args(argc, argv, cfg)) {
        return 1;
    }

    daqdrn::file_backing_store db("run-number.db");
    daqdrn::run_number<daqdrn::file_backing_store> run_number_generator(db);

    zmq::context_t context(1);
    zmq::socket_t responder(context, ZMQ_REP);

    if (cfg.verbose)
        std::cout << "Binding to " << cfg.endpoint << std::endl;
    responder.bind(cfg.endpoint.c_str());

    while (true) {
        zmq::message_t request;

        responder.recv(&request);

        if (cfg.test_crash) {
            std::cerr << "crashing on purpose" << std::endl;
            std::exit(1);
        }

        if (request.size() != sizeof(::daqd_run_number_req_v1_t)) {
            if (cfg.verbose) {
                std::cout << "Received a bad request [invalid size]" << std::endl;
            }
            send_zero_response(responder);
            continue;
        }
        daqd_run_number_req_v1_t* req = reinterpret_cast<daqd_run_number_req_v1_t *>(request.data());
        if (req->version != 1 || req->hash_size < 0 || req->hash_size> sizeof(req->hash)) {
            if (cfg.verbose) {
                std::cout << "Received a bad request [invalid parameters]" << std::endl;
                send_zero_response(responder);
                continue;
            }
        }

        if (cfg.verbose) {
            std::string hash(req->hash, req->hash_size);
            std::cout << "Received a v" << req->version << " request with hash " << hash << " ";
        }

        daqd_run_number_resp_v1_t resp;
        resp.version = 1;
        resp.padding = 0;
        resp.number = run_number_generator.get_number(std::string(req->hash, req->hash_size));
        zmq::message_t response(sizeof(resp));
        memcpy(response.data(), &resp, sizeof(resp));
        responder.send(response);
        if (cfg.verbose) {
            std::cout << "returning run number = " << resp.number << std::endl;
        }
    }
    return 0;
}
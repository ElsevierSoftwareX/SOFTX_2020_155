#include <cassert>
#include <iostream>
#include <string>

#include <zmq.hpp>
#include "run_number_structs.h"

bool parse_args(int argc, char *argv[], std::string& target, std::string& hash) {
    assert(argc >= 1);
    std::string prog_name(argv[0]);

    bool need_help = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-h" || arg == "--help") need_help = true;
    }
    if (need_help || (argc != 3 && argc != 2)) {
        std::cerr << "Usage:\n\t" << prog_name << " [target] <hash>\n\n";
        std::cerr << "Where target defaults to '" << target << "' if not specified." << std::endl;
        return false;
    }
    if (argc == 2) {
        hash = argv[1];
    }
    if (argc == 3) {
        target = argv[1];
        hash = argv[2];
    }
    return true;
}

int main(int argc, char *argv[]) {
    std::string target = "tcp://localhost:5556";
    std::string hash = "";

    if (!parse_args(argc, argv, target, hash)) {
        return 1;
    }

    daqd_run_number_req_v1_t req;
    assert(hash.size() <= sizeof(req.hash));

    req.version = 1;
    req.hash_size = static_cast<short>(hash.size());
    memcpy(&req.hash[0], hash.data(), hash.size());

    zmq::context_t context(1);
    zmq::socket_t requestor(context, ZMQ_REQ);

    zmq::message_t request(sizeof(req));
    memcpy(request.data(), &req, sizeof(req));

    requestor.connect(target);
    requestor.send(request);

    zmq::message_t resp_msg;
    requestor.recv(&resp_msg);

    assert(resp_msg.size() == sizeof(daqd_run_number_resp_v1_t));
    daqd_run_number_resp_v1_t* resp = reinterpret_cast<daqd_run_number_resp_v1_t*>(resp_msg.data());
    assert(resp->version == 1);
    std::cout << "The new run number is " << resp->number << std::endl;

    return 0;
}
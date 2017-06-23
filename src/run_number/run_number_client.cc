#include <zmq.hpp>
#include "run_number_structs.h"
#include "run_number_client.hh"


namespace daqd_run_number {

    int get_run_number(const std::string& target, const std::string& hash)
    {
        daqd_run_number_req_v1_t req;
        if (hash.size() > sizeof(req.hash)) return 0;

        req.version = 1;
        req.hash_size = static_cast<short>(hash.size());
        memcpy(&req.hash[0], hash.data(), hash.size());

        zmq::context_t context(1);
        zmq::socket_t requestor(context, ZMQ_REQ);

        zmq::message_t request(sizeof(req));
        memcpy(request.data(), &req, sizeof(req));

        requestor.connect(target.c_str());
        requestor.send(request);

        zmq::message_t resp_msg;
        requestor.recv(&resp_msg);

        if (resp_msg.size() != sizeof(daqd_run_number_resp_v1_t)) return 0;
        daqd_run_number_resp_v1_t* resp = reinterpret_cast<daqd_run_number_resp_v1_t*>(resp_msg.data());
        if (resp->version != 1) return 0;
        return resp->number;
    }
}
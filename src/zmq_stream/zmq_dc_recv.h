#ifndef ZMQ_DC_RECV_H
#define ZMQ_DC_RECV_H

#ifdef __cplusplus
#include <string>
#include <vector>
#endif

#include "daq_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zmq_data_block {
    daq_dc_data_t *full_data_block;
    int send_length;
} zmq_data_block;

struct zmq_dc_receiver;
typedef struct zmq_dc_receiver* zmq_dc_receiver_p;

extern zmq_dc_receiver_p zmq_dc_receiver_create(char **sname);
extern void zmq_dc_receiver_destroy(zmq_dc_receiver_p rcv);

extern int zmq_dc_receiver_data_mask(const zmq_dc_receiver_p rcv);
extern void zmq_dc_receiver_stop_threads(zmq_dc_receiver_p rcv);
extern void zmq_dc_receiver_begin_acquiring(zmq_dc_receiver_p rcv);
extern void zmq_dc_receiver_receive_data(zmq_dc_receiver_p rcv, zmq_data_block* dest);

extern int zmq_dc_receiver_get_verbose(const zmq_dc_receiver_p rcv);
extern void zmq_dc_receiver_set_verbose(zmq_dc_receiver_p rcv, int verbose);

#ifdef __cplusplus
};
#endif

#ifdef __cplusplus

namespace zmq_dc {
    typedef zmq_data_block data_block;

    class ZMQDCReceiver {
        zmq_dc_receiver_p _rcv;
    public:
        explicit ZMQDCReceiver(char **sname): _rcv(zmq_dc_receiver_create(sname)) {}

        ~ZMQDCReceiver() { zmq_dc_receiver_destroy(_rcv); }

        data_block receive_data()
        {
            data_block db;
            zmq_dc_receiver_receive_data(_rcv, &db);
            return db;
        }

        int data_mask() const { return zmq_dc_receiver_data_mask(_rcv); }
        void begin_acquiring() { zmq_dc_receiver_begin_acquiring(_rcv); }
        void stop_threads() { zmq_dc_receiver_stop_threads(_rcv); }

        bool verbose() const { return zmq_dc_receiver_get_verbose(_rcv) != 0; }
        void verbose(bool val) { return zmq_dc_receiver_set_verbose(_rcv, val ? 1 : 0); }
    };
}

#endif


#endif /* ZMQ_DC_RECV_H */
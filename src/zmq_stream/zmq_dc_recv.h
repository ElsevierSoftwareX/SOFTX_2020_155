#ifndef ZMQ_DC_RECV_H
#define ZMQ_DC_RECV_H

#include <sys/time.h>

#include <algorithm>
#include <atomic>
#include <string>
#include <vector>

#include <zmq.hpp>

#include "zmq_daq_core.h"

namespace zmq_dc {
    class ZMQDCReceiver;

    class receiver_thread_info {
    public:
        receiver_thread_info(int index, const std::string& conn_str, ZMQDCReceiver* dc_receiver) :
                index_(index), _conn_str(conn_str), _dc_receiver(dc_receiver) {}

        receiver_thread_info(receiver_thread_info &&other) : index_(other.index_),
                                                             _conn_str(other._conn_str),
                                                             _dc_receiver(other._dc_receiver)
                                                              {}

        receiver_thread_info &operator=(receiver_thread_info &&other) {
            index_ = other.index_;
            _conn_str = std::move(other._conn_str);
            _dc_receiver = other._dc_receiver;
        }

        int index() const { return index_; }
        const std::string& conn_str() const { return _conn_str; }

        void run_thread();
    private:
        int index_;
        std::string _conn_str;
        ZMQDCReceiver* _dc_receiver;

        receiver_thread_info();

        receiver_thread_info(const receiver_thread_info &other);

        receiver_thread_info operator=(const receiver_thread_info &other);
    };

    struct data_block {
        daq_dc_data_t *full_data_block;
        int send_length;
    };

    /**
     * A class that encapsulates all workings of a ZMQ reciever for a series of ZMQ front ends
     */
    class ZMQDCReceiver {
        friend class receiver_thread_info;

        static const int header_size = sizeof(daq_multi_dcu_header_t);

        zmq::context_t _context;
        std::array<std::atomic<unsigned int>, 16> _tstatus;
        //std::atomic<unsigned int> _tstatus[16];
        std::vector<receiver_thread_info> _thread_info;
        int _data_mask;
        std::atomic<bool> _run_threads;
        std::atomic<bool> _start_acq;
        int _nsys;

        /* variables used for the receive data call */
        bool _resync;
        int _loop;
        int64_t _mylasttime;
        /* debug info */
        bool _verbose;

        /* data tables */
        // Use vectors to make sure they are dynamically allocated
        // if not we can easily blow a stack
        std::vector<daq_dc_data_t> _mxDataBlockFull;
        std::vector<std::vector<daq_multi_dcu_data_t>> _mxDataBlockG;

        void create_subscriber_threads(std::vector<std::string>& sname);

        void run_thread(receiver_thread_info& my_thread_info);

        bool should_threads_stop() const {
            return !_run_threads;
        }

        bool start_acquire() const {
            return _start_acq;
        }

        static int64_t
        s_clock ()
        {
            struct timeval tv;
            gettimeofday (&tv, nullptr);
            return (int64_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
        }
    public:
        explicit ZMQDCReceiver(std::vector<std::string> sname):
                _context(static_cast<int>(sname.size())), _tstatus(),
                _run_threads(true), _start_acq(false), _data_mask(0),
                _nsys(static_cast<int>(sname.size())),
                _resync(true), _loop(0),_mylasttime(0),
                _verbose(false),
                _mxDataBlockFull(16),
                _mxDataBlockG(32)
        {
            for (auto it = _mxDataBlockG.begin(); it != _mxDataBlockG.end(); ++it) {
                it->resize(16);
            }
            clear_status();
            create_subscriber_threads(sname);
        }

        void clear_status() {
            std::fill(_tstatus.begin(), _tstatus.end(), 0);
            //for (int i = 0; i < 16; ++i)
            //    _tstatus[i] = 0;
        }

        void clear_status(int segment) {
            _tstatus[segment] = 0;
        }

        unsigned int get_status(int segment) {
            return _tstatus[segment].load();
        }

        int data_mask() const { return _data_mask; }

        void stop_threads() {
            _run_threads = false;
        }

        void begin_acquiring() {
            _start_acq = true;
        }

        data_block receive_data();

        bool verbose() const { return _verbose; }
        void verbose(bool val) { _verbose = val; }
    };

    extern std::vector<std::string> parse_endpoint_list(const std::string& endpoints);
}

#endif /* ZMQ_DC_RECV_H */
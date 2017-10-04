#include <unistd.h>

#include <array>
#include <iostream>
#include <sstream>

#include "../include/daqmap.h"

#include "zmq_dc_recv.h"

namespace zmq_dc {

    //daq_dc_data_t mxDataBlockFull[16];
    //daq_multi_dcu_data_t mxDataBlockG[32][16];

    static void *rcvr_thread(void *arg) {
        receiver_thread_info *mythread = reinterpret_cast<receiver_thread_info *>(arg);
        mythread->run_thread();
        return 0;
    }

    void receiver_thread_info::run_thread() {
        _dc_receiver->run_thread(*this);
    }

    void ZMQDCReceiver::run_thread(receiver_thread_info& my_thread_info) {
        int mt = my_thread_info.index();
        int ii;
        int cycle = 0;
        int acquire = 0;
        daq_multi_dcu_data_t mxDataBlock;

        zmq::socket_t socket(_context, ZMQ_SUB);
        socket.setsockopt(ZMQ_SUBSCRIBE, nullptr, 0);
        std::cout << "reader thread " << mt << " connecting to " << my_thread_info.conn_str() << std::endl;
        socket.connect(my_thread_info.conn_str());

        std::cout << "thread " << mt << " entering main loop " << std::endl;
        do {
            zmq::message_t message;
            // Get data when message size > 0
            socket.recv(&message, 0);
            //std::cout << "." << std::endl;
            assert(message.size() >= 0);
            // Get pointer to message data
            char *string = reinterpret_cast<char *>(message.data());
            char *daqbuffer = (char *) &mxDataBlock;
            // Copy data out of 0MQ message buffer to local memory buffer
            memcpy(daqbuffer, string, message.size());


            //printf("Received block of %d on %d\n", size, mt);
            for (ii = 0; ii < mxDataBlock.dcuTotalModels; ii++) {
                cycle = mxDataBlock.zmqheader[ii].cycle;
                // Copy data to global buffer
                char *localbuff = (char *) &(_mxDataBlockG[mt][cycle]);
                memcpy(localbuff, daqbuffer, message.size());
            }
            // Always start on cycle 0 after told to start by main thread
            if (cycle == 0 && start_acquire()) {
                if (acquire != 1) {
                    std::cout << "thread " << mt << " starting to acquire\n";
                }
                acquire = 1;
            }
            // Set the cycle data ready bit
            if (acquire) {
                _tstatus[cycle] |= (1 << mt);
            }
            // if (acquire && cycle == 0)
            //    std::cout << "thread " << mt << " received " << message.size() << " bytes" << std::endl;
            // Run until told to stop by main thread
        } while (!should_threads_stop());
        std::cout << "Stopping thread " << mt << std::endl;
        usleep(200000);
    }

    void ZMQDCReceiver::create_subscriber_threads(std::vector<std::string> &sname)
    {
        // allocate once only
        _thread_info.reserve(sname.size());
        // Make 0MQ socket connection
        for(int ii=0;ii<sname.size();ii++) {
            // connect to the publisher
            std::ostringstream os;
            os << "tcp://" << sname[ii] << ":" << DAQ_DATA_PORT;
            std::string conn_str = os.str();
            std::cout << "Sys connection " << ii << " = " << conn_str << "\n";

            _thread_info.emplace_back(receiver_thread_info(ii, conn_str, this));
        }
        _data_mask = 0;
        // we don't actually do anything with this.
        // but might as well keep it stable and not just reference one hard coded value.
        std::vector<pthread_t> thread_id(sname.size());
        for (int ii = 0; ii < sname.size(); ii++) {
            pthread_create(&thread_id[ii],nullptr, rcvr_thread, reinterpret_cast<void*>(&_thread_info[ii]));
            _data_mask |= (1 << ii);
        }
    }

    data_block ZMQDCReceiver::receive_data() {
        data_block results;

        // Wait until received data from at least 1 FE
        int timeout = 0;

        do {
            if (_resync) {
                _loop = 0;
                _resync = false;
                clear_status();
            }

            do {
                usleep(2000);
                timeout += 1;
            } while (get_status(_loop) == 0 && timeout < 50);
            // If timeout, not getting data from anyone.
            if (timeout >= 50) _resync = true;
        } while (_resync);

        // Wait until data received from everyone
        timeout = 0;
        do {
            usleep(1000);
            timeout += 1;
        } while (get_status(_loop) != data_mask() && timeout < 5);
        // If timeout, not getting data from everyone.
        // TODO: MARK MISSING FE DATA AS BAD

        // Clear thread rdy for this cycle
        clear_status(_loop);

        // Timing diagnostics
        int64_t mytime = s_clock();
        int64_t myptime = mytime - _mylasttime;
        _mylasttime = mytime;
        // printf("Data rday for cycle = %d\t%ld\n",_loop,myptime);
        // Reset total DCU counter
        int mytotaldcu = 0;
        // Set pointer to start of DC data block
        char *zbuffer = reinterpret_cast<char *>(&_mxDataBlockFull[_loop].zmqDataBlock[0]);
        // Reset total DC data size counter
        int dc_datablock_size = 0;

        // Loop over all data buffers received from FE computers
        for (int ii = 0; ii < _nsys; ii++) {
            int cur_sys_dcu_count = _mxDataBlockG[ii][_loop].dcuTotalModels;
            // printf("\tModel %d = %d\n",ii,cur_sys_dcu_count);
            for (int jj = 0; jj < cur_sys_dcu_count; jj++) {
                // Copy data header information
                _mxDataBlockFull[_loop].zmqheader[mytotaldcu].dcuId = _mxDataBlockG[ii][_loop].zmqheader[jj].dcuId;
                int cur_dcuid = _mxDataBlockFull[_loop].zmqheader[mytotaldcu].dcuId;
                _mxDataBlockFull[_loop].zmqheader[mytotaldcu].fileCrc = _mxDataBlockG[ii][_loop].zmqheader[jj].fileCrc;
                _mxDataBlockFull[_loop].zmqheader[mytotaldcu].status = _mxDataBlockG[ii][_loop].zmqheader[jj].status;
                _mxDataBlockFull[_loop].zmqheader[mytotaldcu].status;
                if (_mxDataBlockFull[_loop].zmqheader[mytotaldcu].status == 0xbad)
                    std::cout << "Fault on dcuid " << _mxDataBlockFull[_loop].zmqheader[mytotaldcu].dcuId
                              << "\n";
                _mxDataBlockFull[_loop].zmqheader[mytotaldcu].cycle = _mxDataBlockG[ii][_loop].zmqheader[jj].cycle;
                _mxDataBlockFull[_loop].zmqheader[mytotaldcu].timeSec = _mxDataBlockG[ii][_loop].zmqheader[jj].timeSec;
                _mxDataBlockFull[_loop].zmqheader[mytotaldcu].timeNSec = _mxDataBlockG[ii][_loop].zmqheader[jj].timeNSec;
                int mydbs = _mxDataBlockG[ii][_loop].zmqheader[jj].dataBlockSize;

                //if (_loop == 0 && do_verbose)
                //    printf("\t\tdcuid = %d ; data size= %d\n", cur_dcuid, mydbs);

                _mxDataBlockFull[_loop].zmqheader[mytotaldcu].dataBlockSize = mydbs;
                char *mbuffer = (char *) &_mxDataBlockG[ii][_loop].zmqDataBlock[0];
                // Copy data to DC buffer
                memcpy(zbuffer, mbuffer, mydbs);
                // Increment DC data buffer pointer for next data set
                zbuffer += mydbs;
                dc_datablock_size += mydbs;
                mytotaldcu++;
            }
        }

        // printf("\tTotal DCU = %d\tSize = %d\n",mytotaldcu,dc_datablock_size);
        _mxDataBlockFull[_loop].dcuTotalModels = mytotaldcu;

        if (_loop == 0 && _verbose) {
            printf("Recieved %d bytes from %d dcuids\n", dc_datablock_size, mytotaldcu);
        }

        results.send_length = header_size + dc_datablock_size;
        results.full_data_block = &_mxDataBlockFull[_loop];

        ++_loop;
        _loop %= 16;
        return results;
    }

    std::vector<std::string> parse_endpoint_list(const std::string& endpoints)
    {
        std::vector<std::string> sname;
        const char *sysname = endpoints.c_str();
        char *save_ptr = nullptr;
        sname.emplace_back(strtok_r(const_cast<char*>(sysname), ",", &save_ptr));
        for(;;) {
            char *s = strtok_r(nullptr, ",", &save_ptr);
            if (s == nullptr) break;
            sname.emplace_back(std::string(s));
        }
        return sname;
    }
}
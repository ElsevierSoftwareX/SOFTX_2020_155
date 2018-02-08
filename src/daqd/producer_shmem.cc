
#define _XOPEN_SOURCE 600
#define _BSD_SOURCE 1
#define _DEFAULT_SOURCE 1

#include <config.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/timeb.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <array>
#include <cctype>      // old <ctype.h>
#include <sys/prctl.h>

#ifdef USE_SYMMETRICOM
#ifndef USE_IOP
//#include <bcuser.h>
#include <../drv/gpstime/gpstime.h>
#endif
#endif

using namespace std;

#include "circ.hh"
#include "daqd.hh"
#include "sing_list.hh"
#include "drv/cdsHardware.h"
#include <netdb.h>
#include "net_writer.hh"
#include "drv/cdsHardware.h"

#include <sys/ioctl.h>
#include "../drv/rfm.c"

#include "epics_pvs.hh"


#include "raii.hh"
#include "conv.hh"
#include "circ.h"
#include "drv/shmem.h"
#include "zmq_daq_core.h"

extern daqd_c daqd;
extern int shutdown_server();
extern unsigned int crctab[256];

extern long int altzone;

struct ToLower {
    char operator()(char c) const { return std::tolower(c); }
};

/* GM and shared memory communication area */

/* This may still be needed for test points */
struct rmIpcStr gmDaqIpc[DCU_COUNT];
/// DMA memory area pointers
int controller_cycle = 0;

/// Pointer to GDS TP tables
struct cdsDaqNetGdsTpNum *gdsTpNum[2][DCU_COUNT];



class ShMemReceiver {
    volatile daq_multi_cycle_data_t* _shmem;

    daq_dc_data_t _data;
    unsigned int _prev_cycle;

    void wait()
    {
        usleep(1000);
    }
public:
    ShMemReceiver() = delete;
    ShMemReceiver(ShMemReceiver& other) = delete;
    ShMemReceiver(const std::string &endpoint, size_t shmem_size) :
            _shmem(static_cast<volatile daq_multi_cycle_data_t*>(shmem_open_segment(endpoint.c_str(), shmem_size))),
            _prev_cycle(0xffffffff)
    {
    }

    daq_dc_data_t* receive_data()
    {
        std::atomic<unsigned int>* cycle_ptr = reinterpret_cast<std::atomic<unsigned int>*>(
                const_cast<unsigned int*>(&(_shmem->header.curCycle))
        );
        if (_prev_cycle == 0xffffffff)
        {
            _prev_cycle = *cycle_ptr;
        }
        unsigned int cur_cycle = *cycle_ptr;
        while (cur_cycle == _prev_cycle)
        {
            wait();
            cur_cycle = *cycle_ptr;
        }
        unsigned int cycle_stride = _shmem->header.cycleDataSize;
        // figure out offset to the right block
        // figure out how much data is actually there
        // memcpy into _data

        char* start = const_cast<char*>(&_shmem->dataBlock[0]);
        start += cur_cycle * cycle_stride;
        memcpy(&_data, start, cycle_stride);

        std::cerr << "shmem_recv - c " << cur_cycle << " p " << _prev_cycle << std::endl;

        _prev_cycle = cur_cycle;
        return &_data;
    }


};



/// The main data movement thread (the producer)
void *producer::frame_writer() {
    unsigned char *read_dest;
    circ_buffer_block_prop_t prop;

    unsigned long prev_gps, prev_frac;
    unsigned long gps, frac;

    // error message buffer
    char errmsgbuf[80];
    unsigned long stat_cycles = 0;
    stats stat_full, stat_recv, stat_crc, stat_transfer;

    // Set thread parameters
    daqd_c::set_thread_priority("Producer", "dqprod", PROD_THREAD_PRIORITY,
                                PROD_CPUAFFINITY);

    unsigned char *move_buf = 0;
    int vmic_pv_len = 0;
    raii::array_ptr<struct put_dpvec> _vmic_pv(
        new struct put_dpvec[MAX_CHANNELS]);
    struct put_dpvec *vmic_pv = _vmic_pv.get();

    // FIXME: move_buf could leak on errors (but we would probably die anyways.
    daqd.initialize_vmpic(&move_buf, &vmic_pv_len, vmic_pv);
    raii::array_ptr<unsigned char> _move_buf(move_buf);

    // Allocate local test point tables
    static struct cdsDaqNetGdsTpNum gds_tp_table[2][DCU_COUNT];

    for (int ifo = 0; ifo < daqd.data_feeds; ifo++) {
        for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
            if (daqd.dcuSize[ifo][j] == 0)
                continue; // skip unconfigured DCU nodes
            if (IS_MYRINET_DCU(j)) {
                gdsTpNum[ifo][j] = gds_tp_table[ifo] + j;

            } else {
                gdsTpNum[ifo][j] = 0;
            }
        }
    }

    for (int j = 0; j < DCU_COUNT; j++) {
        class stats s;
        rcvr_stats.push_back(s);
    }

    ShMemReceiver shmem_receiver(daqd.parameters().get("shmem_input", ""),
                                 daqd.parameters().get<size_t>("shmem_size", 21041152));


    sleep(1);

    stat_full.sample();

    // No waiting here if compiled as broadcasts receiver

    int cycle_delay = daqd.cycle_delay;
    // Wait until a second boundary
    /*{
        if ((daqd.dcu_status_check & 4) == 0) {

            if (daqd.symm_ok() == 0) {
                printf(
                    "The Symmetricom IRIG-B timing card is not synchronized\n");
                // exit(10);
            }

            unsigned long f;
            const unsigned int c = 1000000000 / 16;
            // Wait for the beginning of a second
            for (;;) {

                prev_gps = daqd.symm_gps(&f);
                gps = prev_gps;
                // if (f > 999493000) break;
                if (f < ((cycle_delay + 2) * c) && f > ((cycle_delay + 1) * c))
                    break; // Three cycles after a second

                struct timespec wait = {0, 10000000UL}; // 10 milliseconds
                nanosleep(&wait, NULL);
            }
            prev_gps = gps;
            prev_frac = c * cycle_delay;
            frac = c * (cycle_delay + 1);
            printf("Starting at gps %ld prev_gps %ld frac %ld f %ld\n", gps,
                   prev_gps, frac, f);
            controller_cycle = 1;
        }
    }*/

    // Wait until a second ends, so that the next data sould
    // come in on cycle 0
    // use the data as the clock here
    int sync_tries = 0;
    while (true) {
        const int max_sync_tries = 10 * DATA_BLOCKS;

        daq_dc_data_t* block = shmem_receiver.receive_data();
        ++sync_tries;
        if (block->header.dcuTotalModels == 0)
            continue;
        gps = block->header.dcuheader[0].timeSec;
        frac = block->header.dcuheader[0].timeNSec;
        std::cerr << block->header.dcuheader[0].timeNSec << std::endl;
        // as of 8 Nov 2017 zmq_multi_stream sends the gps nanoseconds as a cycle number
        if (frac == DATA_BLOCKS-1 || frac >= 937500000)
            break;
        if (sync_tries > max_sync_tries) {
            std::cerr << "Unable to sync up to front ends after " << sync_tries << " attempts" << std::endl;
            exit(1);
        }
    }
    prev_gps = gps;
    prev_frac = frac;


    PV::set_pv(PV::PV_UPTIME_SECONDS, 0);
    PV::set_pv(PV::PV_GPS, 0);

    int prev_controller_cycle = -1;
    int dcu_cycle = 0;
    int resync = 0;

    if (daqd.dcu_status_check & 4)
        resync = 1;

    for (unsigned long i = 0;; i++) { // timing
        tick();                       // measure statistics

        // DEBUG(6, printf("Timing %d gps=%d frac=%d\n", i, gps, frac));

        std::array<int, DCU_COUNT> dcu_to_zmq_lookup;
        std::array<char*,  DCU_COUNT> dcu_data_from_zmq;
        std::fill(dcu_to_zmq_lookup.begin(), dcu_to_zmq_lookup.end(), -1);
        std::fill(dcu_data_from_zmq.begin(), dcu_data_from_zmq.end(), nullptr);
        // retreive 1/16s of data from zmq
        daq_dc_data_t* data_block = shmem_receiver.receive_data();
        std::cout << "#" << std::endl;
        if (data_block->header.dcuTotalModels > 0) {
            gps = data_block->header.dcuheader[0].timeSec;
            frac = data_block->header.dcuheader[0].timeNSec;

            {
                for (int i = 0; i < data_block->header.dcuTotalModels; ++i)
                {
                    if (data_block->header.dcuheader[i].dataBlockSize == 0)
                        std::cerr << "block " << i << " has 0 bytes" << std::endl;
                }
            }

//            {
//                size_t offset = 0;
//                char *_data = &(data_block->dataBlock[0]);
//                for (int kk = 0; kk < data_block->header.dcuTotalModels; kk++)
//                {
//                    if (data_block->header.dcuheader[kk].dcuId != 21)
//                    {
//                        offset += data_block->header.dcuheader[kk].dataBlockSize;
//                        _data += data_block->header.dcuheader[kk].dataBlockSize;
//                        continue;
//                    }
//                    int errors = 0;
//                    int *int_data = reinterpret_cast<int*>(_data);
//                    for (int jj = 0; jj < 64; ++jj)
//                    {
//                        if (int_data[jj] != static_cast<int>(gps))
//                            ++errors;
//                    }
//                    if (errors > 0)
//                    {
//                        std::cerr << "@@@@@@@@@@@@ data is wrong err cnt " << errors << " offsets " << offset << std::endl;
//                    }
//                    break;
//                }
//            }

            bool new_sec = (i % 16) == 0;
            bool is_good = false;
            if (new_sec) {
                is_good = (gps == prev_gps + 1 && frac == 0);
                for (int i = 0; i < data_block->header.dcuTotalModels; ++i)
                {
                    int dcuid = data_block->header.dcuheader[i].dcuId;
                    gds_tp_table[0][dcuid].count = data_block->header.dcuheader[i].tpCount;
                    std::copy(&data_block->header.dcuheader[i].tpNum[0],
                              &data_block->header.dcuheader[i].tpNum[256],
                              &gds_tp_table[0][dcuid].tpNum[0]);
                }
            } else {
                const unsigned int step = 1000000000/16;
                is_good = (gps == prev_gps && ((frac == prev_frac + 1) || (frac == prev_frac + step)));
            }
            if (!is_good) {
                std::cerr << "###################################\n\n\nGlitch in receive\n"
                          << "prev " << prev_gps << ":" << prev_frac << "    cur " << gps << ":" << frac << std::endl;
            }
        }

        // map out the order of the dcuids in the zmq data, this could change
        // with each data block
        {
            int total_zmq_models = data_block->header.dcuTotalModels;
            char *cur_dcu_zmq_ptr = data_block->dataBlock;
            for (int cur_block = 0; cur_block < total_zmq_models; ++cur_block) {
                unsigned int cur_dcuid = data_block->header.dcuheader[cur_block].dcuId;
                dcu_to_zmq_lookup[cur_dcuid] = cur_block;
                dcu_data_from_zmq[cur_dcuid] = cur_dcu_zmq_ptr;
                cur_dcu_zmq_ptr += data_block->header.dcuheader[cur_block].dataBlockSize +
                                   data_block->header.dcuheader[cur_block].tpBlockSize;
            }
        }

        read_dest = move_buf;
        for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
            // printf("DCU %d is %d bytes long\n", j, daqd.dcuSize[0][j]);
            if (daqd.dcuSize[0][j] == 0 || dcu_to_zmq_lookup[j] < 0 || dcu_data_from_zmq[j] == nullptr)
                continue; // skip unconfigured DCU nodes
            std::cout << "dcu " << j << std::endl;
            long read_size = daqd.dcuDAQsize[0][j];
            if (IS_EPICS_DCU(j)) {

                memcpy((void *)read_dest,
                       (char *)(daqd.edcu1.channel_value + daqd.edcu1.fidx),
                       read_size);
                daqd.dcuStatus[0][j] = 0;

                read_dest += read_size;
            } else if (IS_MYRINET_DCU(j)) {
                std::cout << "is a myrinet dcu" << std::endl;
                dcu_cycle = i % DAQ_NUM_DATA_BLOCKS;

                // dcu_cycle = gmDaqIpc[j].cycle;
                // printf("cycl=%d ctrl=%d dcu=%d\n", gmDaqIpc[j].cycle,
                // controller_cycle, j);
                // Get the data from myrinet
                // Get the data from the buffers returned by the zmq receiver
                int zmq_index = dcu_to_zmq_lookup[j];
                daq_msg_header_t& cur_dcu = data_block->header.dcuheader[zmq_index];
                if (read_size != cur_dcu.dataBlockSize) {
                    std::cerr << "read_size = " << read_size << " cur dcu size " << cur_dcu.dataBlockSize << std::endl;
                }
                assert(read_size == cur_dcu.dataBlockSize);
                memcpy((void *)read_dest,
                       dcu_data_from_zmq[j],
                       cur_dcu.dataBlockSize);

//                std::cout << "is zmq dcu number " << zmq_index << std::endl;

//                if (j == 21)
//                {
//                    int block_time = static_cast<int>(data_block->header.dcuheader[zmq_index].timeSec);
//                    int data_error = 0;
//                    for (int k = 0; k < 64; ++k)
//                    {
//                        if (((int*)read_dest)[k] != block_time)
//                            data_error++;
//                    }
//                    if (data_error > 0) {
//                        std::cerr << "!!!!!!!!!!!! invalid data found in test " << data_error << " times at " << block_time << std::endl;
//                    }
//                }

                int cblk1 = (i + 1) % DAQ_NUM_DATA_BLOCKS;
                static const int ifo = 0; // For now

                // Calculate DCU status, if needed
                if (daqd.dcu_status_check & (1 << ifo)) {
                    std::cout << "need status check" << std::endl;
                    if (cblk1 % 16 == 0) {
                        /* DCU checking mask (Which DCUs to check for SYNC
                         * fault) */
                        unsigned int dcm = 0xfffffff0;

                        int lastStatus = dcuStatus[ifo][j];
                        dcuStatus[ifo][j] = DAQ_STATE_FAULT;

                        /* Check if DCU running at all */
                        if (1 /*dcm & (1 << j)*/) {
                            if (dcuStatCycle[ifo][j] == 0)
                                dcuStatus[ifo][j] = DAQ_STATE_SYNC_ERR;
                            else
                                dcuStatus[ifo][j] = DAQ_STATE_RUN;
                        }
                        // dcuCycleStatus shows how many matches of cycle number
                        // we got
                        DEBUG(4, cerr << "dcuid=" << j << " dcuCycleStatus="
                                      << dcuCycleStatus[ifo][j]
                                      << " dcuStatCycle="
                                      << dcuStatCycle[ifo][j] << endl);
                        std::cout << "dcuid=" << j << " dcuCycleStatus="
                             << dcuCycleStatus[ifo][j]
                             << " dcuStatCycle="
                             << dcuStatCycle[ifo][j] << endl;

                        /* Check if DCU running and in sync */
                        if ((dcuCycleStatus[ifo][j] > 3 || j < 5) &&
                            dcuStatCycle[ifo][j] > 4) {
                            dcuStatus[ifo][j] = DAQ_STATE_RUN;
                        }

                        if (/* (lastStatus == DAQ_STATE_RUN) && */ (
                            dcuStatus[ifo][j] != DAQ_STATE_RUN)) {
                            DEBUG(4, cerr << "Lost " << daqd.dcuName[j]
                                          << "(ifo " << ifo << "; dcu " << j
                                          << "); status "
                                          << dcuCycleStatus[ifo][j]
                                          << dcuStatCycle[ifo][j] << endl);
                            std::cout << "Lost " << daqd.dcuName[j]
                                 << "(ifo " << ifo << "; dcu " << j
                                 << "); status "
                                 << dcuCycleStatus[ifo][j]
                                 << dcuStatCycle[ifo][j] << endl;
                            cur_dcu.status = DAQ_STATE_FAULT;
                        }

                        if ((dcuStatus[ifo][j] ==
                             DAQ_STATE_RUN) /* && (lastStatus != DAQ_STATE_RUN) */) {
                            DEBUG(4, cerr << "New " << daqd.dcuName[j]
                                          << " (dcu " << j << ")" << endl);
                            cur_dcu.status = DAQ_STATE_RUN;
                        }

                        dcuCycleStatus[ifo][j] = 0;
                        dcuStatCycle[ifo][j] = 0;
                        cur_dcu.status = cur_dcu.status;
                    }

                    {
                        int intCycle = cur_dcu.cycle % DAQ_NUM_DATA_BLOCKS;
                        if (intCycle != dcuLastCycle[ifo][j])
                            dcuStatCycle[ifo][j]++;
                        dcuLastCycle[ifo][j] = intCycle;
                    }
                }

                // Update DCU status
                int newStatus = cur_dcu.status != DAQ_STATE_RUN ? 0xbad : 0;
                std::cout << "newStatus = " << (hex) << newStatus << " cur_dcu.status = " << (dec) << cur_dcu.status;
                std::cout << " gps = " << cur_dcu.timeSec << " gps_n = " << cur_dcu.timeNSec << std::endl;

                int newCrc = cur_dcu.fileCrc;

                // printf("%x\n", *((int *)read_dest));
                if (!IS_EXC_DCU(j)) {
                    if (newCrc != daqd.dcuConfigCRC[0][j]) {
                        newStatus |= 0x2000;
                        std::cout << "config crc mismatch" << std::endl;
                    }
                }
                if (newStatus != daqd.dcuStatus[0][j]) {
                    // system_log(1, "DCU %d IFO %d (%s) %s", j, 0,
                    // daqd.dcuName[j], newStatus? "fault": "running");
                    if (newStatus & 0x2000) {
                        // system_log(1, "DCU %d IFO %d (%s) reconfigured (crc
                        // 0x%x rfm 0x%x)", j, 0, daqd.dcuName[j],
                        // daqd.dcuConfigCRC[0][j], newCrc);
                    }
                }
                daqd.dcuStatus[0][j] = newStatus;

                daqd.dcuCycle[0][j] = cur_dcu.cycle;
                std::cout << "status and cycle are " << (hex) << newStatus << (dec) << " " << cur_dcu.cycle << std::endl;

                /* Check DCU data checksum */
                unsigned long crc = 0;
                unsigned long bytes = read_size;
                unsigned char *cp = (unsigned char *)read_dest;
                while (bytes--) {
                    crc = (crc << 8) ^ crctab[((crc >> 24) ^ *(cp++)) & 0xFF];
                }
                bytes = read_size;
                while (bytes > 0) {
                    crc = (crc << 8) ^ crctab[((crc >> 24) ^ bytes) & 0xFF];
                    bytes >>= 8;
                }
                crc = ~crc & 0xFFFFFFFF;
                int cblk = i % 16;
                // Reset CRC/second variable for this DCU
                if (cblk == 0) {
                    daqd.dcuCrcErrCntPerSecond[0][j] =
                        daqd.dcuCrcErrCntPerSecondRunning[0][j];
                    daqd.dcuCrcErrCntPerSecondRunning[0][j] = 0;
                }

                if (j >= DCU_ID_ADCU_1 && (!IS_TP_DCU(j)) &&
                    daqd.dcuStatus[0][j] == 0) {

                    unsigned int rfm_crc = gmDaqIpc[j].bp[cblk].crc;
                    unsigned int dcu_gps = gmDaqIpc[j].bp[cblk].timeSec;

                    // system_log(5, "dcu %d block %d cycle %d  gps %d symm
                    // %d\n", j, cblk, gmDaqIpc[j].bp[cblk].cycle,  dcu_gps,
                    // gps);
                    unsigned long mygps = gps;
                    //if (cblk > (15 - cycle_delay))
                    //    mygps--;

                    if (daqd.edcuFileStatus[j]) {
                        daqd.dcuStatus[0][j] |= 0x8000;
                        system_log(5, "EDCU .ini FILE CRC MISS dcu %d (%s)", j,
                                   daqd.dcuName[j]);
                    }
                    if (dcu_gps != mygps) {
                        daqd.dcuStatus[0][j] |= 0x4000;
                        system_log(5,
                                   "GPS MISS dcu %d (%s); dcu_gps=%d gps=%ld\n",
                                   j, daqd.dcuName[j], dcu_gps, mygps);
                    }

                    if (rfm_crc != crc) {
                        system_log(
                            5,
                            "MISS dcu %d (%s); crc[%d]=%x; computed crc=%lx\n",
                            j, daqd.dcuName[j], cblk, rfm_crc, crc);

                        /* Set DCU status to BAD, all data will be marked as BAD
                           because of the CRC mismatch */
                        daqd.dcuStatus[0][j] |= 0x1000;
                    } else {
                        system_log(6, " MATCH dcu %d (%s); crc[%d]=%x; "
                                      "computed crc=%lx\n",
                                   j, daqd.dcuName[j], cblk, rfm_crc, crc);
                    }
                    if (daqd.dcuStatus[0][j]) {
                        daqd.dcuCrcErrCnt[0][j]++;
                        daqd.dcuCrcErrCntPerSecondRunning[0][j]++;
                    }
                    // FIXME: is this right? Why 2* DAQ_DCU_BLOCK_SIZE?
                    read_dest += 2 * DAQ_DCU_BLOCK_SIZE;
                } else {
                    read_dest += cur_dcu.dataBlockSize;
                }

            }
        }

        int cblk = i % 16;

        // Assign per-DCU data we need to broadcast out
        //
        for (int ifo = 0; ifo < daqd.data_feeds; ifo++) {
            for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
                if (IS_TP_DCU(j))
                    continue; // Skip TP and EXC DCUs
                if (daqd.dcuSize[ifo][j] == 0)
                    continue; // Skip unconfigured DCUs
                prop.dcu_data[j + ifo * DCU_COUNT].cycle =
                    daqd.dcuCycle[ifo][j];
                volatile struct rmIpcStr *ipc = daqd.dcuIpc[ifo][j];

                // Do not support Myrinet DCUs on H2
                if (IS_MYRINET_DCU(j) && ifo == 0) {

                    prop.dcu_data[j].crc = gmDaqIpc[j].bp[cblk].crc;

                    // printf("dcu %d crc=0x%x\n", j, prop.dcu_data[j].crc);
                    // Remove 0x8000 status from propagating to the broadcast
                    // receivers
                    prop.dcu_data[j].status =
                        daqd.dcuStatus[0 /* IFO */][j] & ~0x8000;
                } else
                    // EDCU is "attached" to H1, not H2
                    if (j == DCU_ID_EDCU && ifo == 0) {
                    // See if the EDCU thread is running and assign status
                    if (0x0 == (prop.dcu_data[j].status =
                                    daqd.edcu1.running ? 0x0 : 0xbad)) {
                        // If running calculate the CRC

                        // memcpy(read_dest, (char *)(daqd.edcu1.channel_value +
                        // daqd.edcu1.fidx), daqd.dcuSize[ifo][j]);

                        unsigned int bytes = daqd.dcuSize[0][DCU_ID_EDCU];
                        unsigned char *cp =
                            move_buf; // The EDCU data is in front
                        unsigned long crc = 0;
                        while (bytes--) {
                            crc = (crc << 8) ^
                                  crctab[((crc >> 24) ^ *(cp++)) & 0xFF];
                        }
                        bytes = daqd.dcuDAQsize[0][DCU_ID_EDCU];
                        while (bytes > 0) {
                            crc = (crc << 8) ^
                                  crctab[((crc >> 24) ^ bytes) & 0xFF];
                            bytes >>= 8;
                        }
                        crc = ~crc & 0xFFFFFFFF;
                        prop.dcu_data[j].crc = crc;
                    }
                } else {
                    prop.dcu_data[j + ifo * DCU_COUNT].crc = ipc->bp[cblk].crc;
                    prop.dcu_data[j + ifo * DCU_COUNT].status =
                        daqd.dcuStatus[ifo][j];
                }
            }
        }

        // prop.gps = time(0) - 315964819 + 33;

        prop.gps = gps;
        //if (cblk > (15 - cycle_delay))
        //    prop.gps--;

        prop.gps_n = 1000000000 / 16 * (i % 16);
// printf("before put %d %d %d\n", prop.gps, prop.gps_n, frac);
        prop.leap_seconds = daqd.gps_leap_seconds(prop.gps);

        std::cout << "about to call put16th_dpscattered with " << vmic_pv_len << " entries. prop.gps = " << prop.gps << " prop.gps_n = " << prop.gps_n << "\n";
        //for (int ii = 0; ii < vmic_pv_len; ++ii)
        //    std::cout << " " << *vmic_pv[ii].src_status_addr;
        //std::cout << std::endl;


        stat_transfer.sample();
        int nbi = daqd.b1->put16th_dpscattered(vmic_pv, vmic_pv_len, &prop);
        stat_transfer.tick();

        {
            circ_buffer_block_t* block_p = daqd.b1->block_prop(nbi);
            std::cout << "block_p->prop.gps = " << block_p->prop.gps << " block_p->prop.gps_n = " << block_p->prop.gps_n << std::endl;
            //if (block_p->prop.gps != prop.gps) {
            //    std::cout << "\n\nblock_p->prop.gps (" << block_p->prop.gps << ") != prop.gps (" << prop.gps << ")\n" << std::endl;
            //}
            //assert(block_p->prop.gps == prop.gps);
        }


        std::cout << "put16th_dpscattered returned " << nbi << std::endl;
        std::cout << "drops: " << daqd.b1->drops() <<  " blocks: " << daqd.b1->blocks() << " puts: " << daqd.b1->num_puts()
                  << " consumers: " << daqd.b1->get_cons_num() << std::endl;

        //  printf("%d %d\n", prop.gps, prop.gps_n);
        // DEBUG1(cerr << "producer " << i << endl);

        PV::set_pv(PV::PV_CYCLE, i);
        PV::set_pv(PV::PV_GPS, prop.gps);
        // DEBUG1(cerr << "gps=" << PV::pv(PV::PV_GPS) << endl);
        if (i % 16 == 0) {
            // Count how many seconds we were acquiring data
            PV::pv(PV::PV_UPTIME_SECONDS)++;

            {
                extern unsigned long dmt_retransmit_count;
                extern unsigned long dmt_failed_retransmit_count;
                // Display DMT retransmit channels every second
                PV::set_pv(PV::PV_BCAST_RETR, dmt_retransmit_count);
                PV::set_pv(PV::PV_BCAST_FAILED_RETR,
                           dmt_failed_retransmit_count);
                dmt_retransmit_count = 0;
                dmt_failed_retransmit_count = 0;
            }
        }

        stat_full.tick();

        ++stat_cycles;
        if (stat_cycles >= 16) {
            PV::set_pv(PV::PV_PRDCR_TIME_FULL_MIN_MS,
                       conv::s_to_ms_int(stat_recv.getMin()));
            PV::set_pv(PV::PV_PRDCR_TIME_FULL_MAX_MS,
                       conv::s_to_ms_int(stat_recv.getMax()));
            PV::set_pv(PV::PV_PRDCR_TIME_FULL_MEAN_MS,
                       conv::s_to_ms_int(stat_recv.getMean()));

            PV::set_pv(PV::PV_PRDCR_TIME_RECV_MIN_MS,
                       conv::s_to_ms_int(stat_recv.getMin()));
            PV::set_pv(PV::PV_PRDCR_TIME_RECV_MAX_MS,
                       conv::s_to_ms_int(stat_recv.getMax()));
            PV::set_pv(PV::PV_PRDCR_TIME_RECV_MEAN_MS,
                       conv::s_to_ms_int(stat_recv.getMean()));

            PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MEAN_MS,
                       conv::s_to_ms_int(stat_crc.getMin()));
            PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MEAN_MS,
                       conv::s_to_ms_int(stat_crc.getMax()));
            PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MEAN_MS,
                       conv::s_to_ms_int(stat_crc.getMean()));

            PV::set_pv(PV::PV_PRDCR_CRC_TIME_XFER_MIN_MS,
                       conv::s_to_ms_int(stat_transfer.getMin()));
            PV::set_pv(PV::PV_PRDCR_CRC_TIME_XFER_MAX_MS,
                       conv::s_to_ms_int(stat_transfer.getMax()));
            PV::set_pv(PV::PV_PRDCR_CRC_TIME_XFER_MEAN_MS,
                       conv::s_to_ms_int(stat_transfer.getMean()));

            stat_full.clearStats();
            stat_crc.clearStats();
            stat_recv.clearStats();
            stat_transfer.clearStats();
            stat_cycles = 0;
        }

        stat_full.sample();

        // printf("gps=%d  prev_gps=%d bfrac=%d prev_frac=%d\n", gps, prev_gps,
        // frac, prev_frac);
        /*const int polls_per_sec = 320; // 320 polls gives 1 millisecond stddev
                                       // of cycle time (AEI Nov 2012)
        for (int ntries = 0;; ntries++) {
            struct timespec tspec = {
                0, 1000000000 / polls_per_sec}; // seconds, nanoseconds
            nanosleep(&tspec, NULL);

            gps = daqd.symm_gps(&frac);

            if (prev_frac == 937500000) {
                if (gps == prev_gps + 1) {
                    frac = 0;
                    break;
                } else {
                    if (gps > prev_gps + 1) {
                        fprintf(stderr, "GPS card time jumped from %ld (%ld) "
                                        "to %ld (%ld)\n",
                                prev_gps, prev_frac, gps, frac);
                        print(cout);
                        _exit(1);
                    } else if (gps < prev_gps) {
                        fprintf(stderr,
                                "GPS card time moved back from %ld to %ld\n",
                                prev_gps, gps);
                        print(cout);
                        _exit(1);
                    }
                }
            } else if (frac >= prev_frac + 62500000) {
                // Check if GPS seconds moved for some reason (because of delay)
                if (gps != prev_gps) {
                    fprintf(stderr, "WARNING: GPS time jumped from %ld (%ld) "
                                    "to %ld (%ld)\n",
                            prev_gps, prev_frac, gps, frac);
                    print(cout);
                    gps = prev_gps;
                }
                frac = prev_frac + 62500000;
                break;
            }

            if (ntries >= polls_per_sec) {

                fprintf(stderr, "Symmetricom GPS timeout\n");

                exit(1);
            }
        }*/
        // printf("gps=%d prev_gps=%d ifrac=%d prev_frac=%d\n", gps,  prev_gps,
        // frac, prev_frac);
        controller_cycle++;

        prev_controller_cycle = controller_cycle;

        prev_gps = gps;
        prev_frac = frac;
    }
}

/// A main loop for a producer that does a debug crc operation
/// in a seperate thread
void *producer::frame_writer_debug_crc() {
    // not implemented
    return (void *)NULL;
}

/// A main loop for a producer that does crc  and data transfer
/// in a seperate thread.
void *producer::frame_writer_crc() {
    // not implemented
    return (void *)NULL;
}

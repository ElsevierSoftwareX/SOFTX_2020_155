
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
#include <cctype>      // old <ctype.h>
#include <sys/prctl.h>

#ifdef USE_SYMMETRICOM
#ifndef USE_IOP
//#include <bcuser.h>
#include <../drv/symmetricom/symmetricom.h>
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

#include "zmq_dc_recv.h"

#if EPICS_EDCU == 1
#include "epics_pvs.hh"
#endif

#include "raii.hh"
#include "conv.hh"

extern daqd_c daqd;
extern int shutdown_server();
extern unsigned int crctab[256];

extern long int altzone;

struct ToLower {
    char operator()(char c) const { return std::tolower(c); }
};

/* GM and shared memory communication area */

struct rmIpcStr gmDaqIpc[DCU_COUNT];
/// DMA memory area pointers
void *directed_receive_buffer[DCU_COUNT];
int controller_cycle = 0;

/// Pointer to GDS TP tables
struct cdsDaqNetGdsTpNum *gdsTpNum[2][DCU_COUNT];



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

    // Allocate receive buffers for each configured DCU
    for (int i = 5; i < DCU_COUNT; i++) {
        if (0 == daqd.dcuSize[0][i])
            continue;

        directed_receive_buffer[i] =
            malloc(2 * DAQ_DCU_BLOCK_SIZE * DAQ_NUM_DATA_BLOCKS);
        if (directed_receive_buffer[i] == 0) {
            system_log(1, "[MX recv] Couldn't allocate recv buffer\n");
            exit(1);
        }
    }

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

    // FIXME: launch zmq_receiver thread here!
    std::vector<std::string> zmq_endpoints(zmq_dc::parse_endpoint_list(daqd.parameters().get("zmq_fecs", "")));
    zmq::context_t zcontext(1);
    zmq_dc::ZMQDCReceiver zmq_receiver(zcontext, zmq_endpoints);

    sleep(1);


    stat_full.sample();
// TODO make IP addresses configurable from daqdrc

    // No waiting here if compiled as broadcasts receiver

    int cycle_delay = daqd.cycle_delay;
    // Wait until a second boundary
    {
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
    }

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

        read_dest = move_buf;
        for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
            // printf("DCU %d is %d bytes long\n", j, daqd.dcuSize[0][j]);
            if (daqd.dcuSize[0][j] == 0)
                continue; // skip unconfigured DCU nodes
            long read_size = daqd.dcuDAQsize[0][j];
            if (IS_EPICS_DCU(j)) {

                memcpy((void *)read_dest,
                       (char *)(daqd.edcu1.channel_value + daqd.edcu1.fidx),
                       read_size);
                daqd.dcuStatus[0][j] = 0;

                read_dest += read_size;
            } else if (IS_MYRINET_DCU(j)) {
                dcu_cycle = i % DAQ_NUM_DATA_BLOCKS;

                // dcu_cycle = gmDaqIpc[j].cycle;
                // printf("cycl=%d ctrl=%d dcu=%d\n", gmDaqIpc[j].cycle,
                // controller_cycle, j);
                // Get the data from myrinet
                memcpy((void *)read_dest,
                       ((char *)directed_receive_buffer[j]) +
                           dcu_cycle * 2 * DAQ_DCU_BLOCK_SIZE,
                       2 * DAQ_DCU_BLOCK_SIZE);

                volatile struct rmIpcStr *ipc;

                ipc = &gmDaqIpc[j];

                int cblk1 = (i + 1) % DAQ_NUM_DATA_BLOCKS;
                static const int ifo = 0; // For now

                // Calculate DCU status, if needed
                if (daqd.dcu_status_check & (1 << ifo)) {
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
                            ipc->status = DAQ_STATE_FAULT;
                        }

                        if ((dcuStatus[ifo][j] ==
                             DAQ_STATE_RUN) /* && (lastStatus != DAQ_STATE_RUN) */) {
                            DEBUG(4, cerr << "New " << daqd.dcuName[j]
                                          << " (dcu " << j << ")" << endl);
                            ipc->status = DAQ_STATE_RUN;
                        }

                        dcuCycleStatus[ifo][j] = 0;
                        dcuStatCycle[ifo][j] = 0;
                        ipc->status = ipc->status;
                    }

                    {
                        int intCycle = ipc->cycle % DAQ_NUM_DATA_BLOCKS;
                        if (intCycle != dcuLastCycle[ifo][j])
                            dcuStatCycle[ifo][j]++;
                        dcuLastCycle[ifo][j] = intCycle;
                    }
                }

                // Update DCU status
                int newStatus = ipc->status != DAQ_STATE_RUN ? 0xbad : 0;

                int newCrc = gmDaqIpc[j].crc;

                // printf("%x\n", *((int *)read_dest));
                if (!IS_EXC_DCU(j)) {
                    if (newCrc != daqd.dcuConfigCRC[0][j])
                        newStatus |= 0x2000;
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

                daqd.dcuCycle[0][j] = gmDaqIpc[j].cycle;

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
                    if (cblk > (15 - cycle_delay))
                        mygps--;

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
                }

                read_dest += 2 * DAQ_DCU_BLOCK_SIZE;
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
        if (cblk > (15 - cycle_delay))
            prop.gps--;

        prop.gps_n = 1000000000 / 16 * (i % 16);
// printf("before put %d %d %d\n", prop.gps, prop.gps_n, frac);
        prop.leap_seconds = daqd.gps_leap_seconds(prop.gps);

        stat_transfer.sample();
        int nbi = daqd.b1->put16th_dpscattered(vmic_pv, vmic_pv_len, &prop);
        stat_transfer.tick();

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
        const int polls_per_sec = 320; // 320 polls gives 1 millisecond stddev
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
        }
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

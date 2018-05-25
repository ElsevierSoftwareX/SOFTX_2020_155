
#define _XOPEN_SOURCE 600
#define _BSD_SOURCE 1

#include <algorithm>
#include <arpa/inet.h>
#include <assert.h>
#include <cctype> // old <ctype.h>
#include <config.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef USE_SYMMETRICOM
#ifndef USE_IOP
//#include <bcuser.h>
#include <../drv/gpstime/gpstime.h>
#endif
#endif

using namespace std;

#include "circ.hh"
#include "daqd.hh"
#include "drv/cdsHardware.h"
#include "drv/cdsHardware.h"
#include "gm_rcvr.hh"
#include "net_writer.hh"
#include "sing_list.hh"
#include <netdb.h>

#include "../drv/rfm.c"
#include <sys/ioctl.h>

#if EPICS_EDCU == 1
#include "epics_pvs.hh"
#endif

#include "conv.hh"
#include "raii.hh"

extern daqd_c daqd;
extern int shutdown_server();
extern unsigned int crctab[256];

extern long int altzone;

struct ToLower {
    char operator()(char c) const { return std::tolower(c); }
};

/* GM and shared memory communication area */

int controller_cycle = 0;

///	@file daqmap.h
///	@brief File contains defs and structures used in DAQ and GDS/TP
/// routines. \n
///<		NOTE: This file used by daqd, as well as real-time and EPICS
/// code.

#define MAX_DEC 128
#define DEC_FILT_LENGTH 21
#define FB_H_INCLUDED

int cdsDaqNetInit(int);            /* Initialize DAQ network		*/
int cdsDaqNetClose(void);          /* Close CDS network connection	*/
int cdsDaqNetCheckCallback(void);  /* Check for messages on 	*/
int cdsDaqNetReconnect(int);       /* Make connects to FB.		*/
int cdsDaqNetCheckReconnect(void); /* Check FB net connected	*/
int cdsDaqNetDrop(void);
int cdsDaqNetDaqSend(int dcuId, int cycle, int subCycle, unsigned int fileCrc,
                     unsigned int blockCrc, int crcSize, int tpCount,
                     int tpNum[], int xferSize, char *dataBuffer);

/* Offset in the shared memory to the beginning of data buffer */
#define CDS_DAQ_NET_DATA_OFFSET 0x2000
/* Offset to GDS test point table (struct cdsDaqNetGdsTpNum, defined in
 * daqmap.h) */
#define CDS_DAQ_NET_GDS_TP_TABLE_OFFSET 0x1000
/* Offset to the IPC structure (struct rmIpcStr, defined in daqmap.h) */
#define CDS_DAQ_NET_IPC_OFFSET 0x0

extern int cdsNetStatus;
extern unsigned int cycle_gps_time;
extern unsigned int cycle_gps_ns;

/// Memory mapped addresses for the DCUs
volatile unsigned char *dcu_addr[DCU_COUNT];

/// Pointers to IPC areas for each DCU
struct rmIpcStr *shmemDaqIpc[DCU_COUNT];

/// Pointers into the shared memory for the cycle and time (coming from the IOP
/// (e.g. x00))
volatile int *ioMemDataCycle;
volatile int *ioMemDataGPS;
volatile IO_MEM_DATA *ioMemData;

/// Pointer to GDS TP tables
struct cdsDaqNetGdsTpNum *gdsTpNum[2][DCU_COUNT];

/// Data receiving thread
void *gm_receiver_thread(void *this_p) {

    int fd;
    // error message buffer
    char errmsgbuf[80];

    // Open and map all "Myrinet" DCUs
    for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
        if (daqd.dcuSize[0][j] == 0)
            continue; // skip unconfigured DCU nodes
        if (IS_MYRINET_DCU(j)) {
            std::string s(daqd.fullDcuName[j]);
            std::transform(s.begin(), s.end(), s.begin(), ToLower());
            s = s + "_daq";
            dcu_addr[j] =
                (volatile unsigned char *)findSharedMemory((char *)s.c_str());
            if (dcu_addr[j] == 0) {
                strerror_r(errno, errmsgbuf, sizeof(errmsgbuf));
                system_log(1, "Couldn't mmap `%s'; err = %s\n", s.c_str(),
                           errmsgbuf);
                exit(1);
            }
            system_log(1, "Opened %s\n", s.c_str());
            shmemDaqIpc[j] =
                (struct rmIpcStr *)(dcu_addr[j] + CDS_DAQ_NET_IPC_OFFSET);
            gdsTpNum[0][j] =
                (struct cdsDaqNetGdsTpNum *)(dcu_addr[j] +
                                             CDS_DAQ_NET_GDS_TP_TABLE_OFFSET);
        } else {
            gdsTpNum[0][j] = 0;
        }
    }

    // Open the IPC shared memory
    volatile void *ptr = findSharedMemory("ipc");
    if (ptr == 0) {
        system_log(1, "Couldn't open shared memory IPC area");
        exit(1);
    }
    system_log(1, "Opened shared memory ipc area\n");
    ioMemData = (volatile IO_MEM_DATA *)(((char *)ptr) + IO_MEM_DATA_OFFSET);

    CDS_HARDWARE cdsPciModules;

    // Find the first ADC card
    // Master will map ADC cards first, then DAC and finally DIO
    printf("Total PCI cards from the master: %d\n", ioMemData->totalCards);
    for (int ii = 0; ii < ioMemData->totalCards; ii++) {
        printf("Model %d = %d\n", ii, ioMemData->model[ii]);
        switch (ioMemData->model[ii]) {
        case GSC_16AI64SSA:
            printf("Found ADC at %d\n", ioMemData->ipc[ii]);
            cdsPciModules.adcType[0] = GSC_16AI64SSA;
            cdsPciModules.adcConfig[0] = ioMemData->ipc[ii];
            cdsPciModules.adcCount = 1;
            break;
        }
    }
    if (!cdsPciModules.adcCount) {
        printf("No ADC cards found - exiting\n");
        exit(1);
    }

    int ll = cdsPciModules.adcConfig[0];
    ioMemDataCycle = &ioMemData->iodata[ll][0].cycle;
    printf("ioMem Cycle from %d\n", ll);
    ioMemDataGPS = &ioMemData->gpsSecond;
}

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

    // FIXME: move_buf could leak on errors (but we would probably die
    // anyways.
    daqd.initialize_vmpic(&move_buf, &vmic_pv_len, vmic_pv);
    raii::array_ptr<unsigned char> _move_buf(move_buf);

    if (!daqd.no_myrinet) {


        {
            pthread_t gm_tid;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, daqd.thread_stack_size);
            pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
            int my_err_no;

            for (int j = 0; j < DCU_COUNT; j++) {
                class stats s;
                rcvr_stats.push_back(s);
            }

            // GM, USE_BROADCAST have a single thread
            if (my_err_no =
                    pthread_create(&gm_tid, &attr, gm_receiver_thread, 0)) {
                strerror_r(my_err_no, errmsgbuf, sizeof(errmsgbuf));
                pthread_attr_destroy(&attr);
                system_log(1, "pthread_create() err=%s", errmsgbuf);
                exit(1);
            } 
            pthread_attr_destroy(&attr);
        }

        sleep(1);
    }

    stat_full.sample();
    // TODO make IP addresses configurable from daqdrc

    // No waiting here if compiled as broadcasts receiver
    int cycle_delay = daqd.cycle_delay;
    // Wait until a second boundary
    {
        if ((daqd.dcu_status_check & 4) == 0) {
            if (daqd.symm_ok() == 0) {
                printf("The Symmetricom IRIG-B timing card is "
                       "not synchronized\n");
                // exit(10);
            }
            unsigned long f;
            const unsigned int c = 1000000000 / 16;
            // Wait for the beginning of a second
            for (;;) {
                prev_gps = daqd.symm_gps(&f);
                // prev_frac = 1000000000 - 1000000000/16;
                prev_frac = 0;
                // Starting at this time
                gps = prev_gps + 1;
                frac = 0;
                if (f > 990000000)
                    break;

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
            // printf("DCU %d is %d bytes long\n", j,
            // daqd.dcuSize[0][j]);
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
                // dcu_cycle = shmemDaqIpc[j]->cycle;
                // Get the data from myrinet
                unsigned char *read_src =
                    (unsigned char *)(dcu_addr[j] + CDS_DAQ_NET_DATA_OFFSET);
                memcpy((void *)read_dest,
                       read_src + dcu_cycle * 2 * DAQ_DCU_BLOCK_SIZE,
                       2 * DAQ_DCU_BLOCK_SIZE);

                volatile struct rmIpcStr *ipc;
                ipc = shmemDaqIpc[j];

                int cblk1 = (i + 1) % DAQ_NUM_DATA_BLOCKS;
                static const int ifo = 0; // For now

                // Calculate DCU status, if needed
                if (daqd.dcu_status_check & (1 << ifo)) {
                    if (cblk1 % 16 == 0) {
                        /* DCU checking mask (Which DCUs
                         * to check for SYNC fault) */
                        unsigned int dcm = 0xfffffff0;

                        int lastStatus = dcuStatus[ifo][j];
                        dcuStatus[ifo][j] = DAQ_STATE_FAULT;

                        /* Check if DCU running at all
                         */
                        if (1 /*dcm & (1 << j)*/) {
                            if (dcuStatCycle[ifo][j] == 0)
                                dcuStatus[ifo][j] = DAQ_STATE_SYNC_ERR;
                            else
                                dcuStatus[ifo][j] = DAQ_STATE_RUN;
                        }

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

                int newCrc = shmemDaqIpc[j]->crc;
                // printf("%x\n", *((int *)read_dest));
                if (!IS_EXC_DCU(j)) {
                    if (newCrc != daqd.dcuConfigCRC[0][j])
                        newStatus |= 0x2000;
                }
                if (newStatus != daqd.dcuStatus[0][j]) {
                    // system_log(1, "DCU %d IFO %d (%s)
                    // %s", j, 0, daqd.dcuName[j],
                    // newStatus? "fault": "running");
                    if (newStatus & 0x2000) {
                        // system_log(1, "DCU %d IFO %d
                        // (%s) reconfigured (crc 0x%x
                        // rfm 0x%x)", j, 0,
                        // daqd.dcuName[j],
                        // daqd.dcuConfigCRC[0][j],
                        // newCrc);
                    }
                }
                daqd.dcuStatus[0][j] = newStatus;
                daqd.dcuCycle[0][j] = shmemDaqIpc[j]->cycle;

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
                    unsigned int rfm_crc = shmemDaqIpc[j]->bp[cblk].crc;
                    unsigned int dcu_gps = shmemDaqIpc[j]->bp[cblk].timeSec;
                    shmemDaqIpc[j]->bp[cblk].crc = 0;

                    // system_log(5, "dcu %d block %d cycle
                    // %d  gps %d symm %d\n", j, cblk,
                    // gmDaqIpc[j].bp[cblk].cycle,  dcu_gps,
                    // gps);
                    unsigned long mygps = gps;
                    if (cblk > (15 - cycle_delay))
                        mygps--;
                    if (daqd.edcuFileStatus[j]) {
                        daqd.dcuStatus[0][j] |= 0x8000;
                        system_log(5, "EDCU .ini FILE "
                                      "CRC MISS dcu %d "
                                      "(%s)",
                                   j, daqd.dcuName[j]);
                    }
                    if (dcu_gps != mygps) {
                        daqd.dcuStatus[0][j] |= 0x4000;
                        system_log(5, "GPS MISS dcu %d (%s); "
                                      "dcu_gps=%d gps=%ld\n",
                                   j, daqd.dcuName[j], dcu_gps, mygps);
                    }

                    if (rfm_crc != crc) {
                        system_log(5, "MISS dcu %d (%s); "
                                      "crc[%d]=%x; "
                                      "computed crc=%lx\n",
                                   j, daqd.dcuName[j], cblk, rfm_crc, crc);

                        /* Set DCU status to BAD, all
                           data will be marked as BAD
                           because of the CRC mismatch
                           */
                        daqd.dcuStatus[0][j] |= 0x1000;
                    } else {
                        system_log(6, " MATCH dcu %d "
                                      "(%s); crc[%d]=%x; "
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

        // printf("gps=%d  prev_gps=%d bfrac=%d prev_frac=%d\n", gps,
        // prev_gps, frac, prev_frac);
        const int polls_per_sec = 320; // 320 polls gives 1 millisecond
                                       // stddev of cycle time (AEI Nov
                                       // 2012)
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
                        fprintf(stderr, "GPS card time "
                                        "jumped from "
                                        "%ld (%ld) to "
                                        "%ld (%ld)\n",
                                prev_gps, prev_frac, gps, frac);
                        print(cout);
                        _exit(1);
                    } else if (gps < prev_gps) {
                        fprintf(stderr, "GPS card time "
                                        "moved back "
                                        "from %ld to "
                                        "%ld\n",
                                prev_gps, gps);
                        print(cout);
                        _exit(1);
                    }
                }
            } else if (frac >= prev_frac + 62500000) {
                // Check if GPS seconds moved for some reason
                // (because of delay)
                if (gps != prev_gps) {
                    fprintf(stderr, "WARNING: GPS time "
                                    "jumped from %ld (%ld) "
                                    "to %ld (%ld)\n",
                            prev_gps, prev_frac, gps, frac);
                    print(cout);
                    gps = prev_gps;
                }
                frac = prev_frac + 62500000;
                break;
            }

            if (ntries >= polls_per_sec) {
                fprintf(stderr, "IOP timeout\n");

                exit(1);
            }
        }
        // printf("gps=%d prev_gps=%d ifrac=%d prev_frac=%d\n", gps,
        // prev_gps, frac, prev_frac);
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

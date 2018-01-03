
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


///	@file daqmap.h
///	@brief File contains defs and structures used in DAQ and GDS/TP
///routines. \n
///<		NOTE: This file used by daqd, as well as real-time and EPICS
///code.

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

/// Memory mapped addresses for the DCUs
volatile unsigned char *dcu_addr[DCU_COUNT];

/// Pointer to GDS TP tables
struct cdsDaqNetGdsTpNum *gdsTpNum[2][DCU_COUNT];

/// The main data movement thread (the producer)
void *producer::frame_writer() {
    unsigned char *read_dest;
    circ_buffer_block_prop_t prop;

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

    if (!daqd.no_myrinet) {

        // Allocate local test point tables
        static struct cdsDaqNetGdsTpNum gds_tp_table[2][DCU_COUNT];

        for (int ifo = 0; ifo < daqd.data_feeds; ifo++) {
            for (int j = DCU_ID_EDCU; j < DCU_COUNT; j++) {
                if (daqd.dcuSize[ifo][j] == 0)
                    continue; // skip unconfigured DCU nodes
                if (IS_MYRINET_DCU(j)) {
                    gdsTpNum[ifo][j] = gds_tp_table[ifo] + j;

                } else if (IS_TP_DCU(j)) {
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

        sleep(1);
    }

    stat_full.sample();
    // TODO make IP addresses configurable from daqdrc

    stat_recv.sample();
    diag::frameRecv *NDS = new diag::frameRecv(0);
    if (!NDS->open("225.0.0.1", "10.110.144.0",
                   net_writer_c::concentrator_broadcast_port)) {
        perror("Multicast receiver open failed.");
        exit(1);
    }
    stat_recv.tick();

    diag::frameRecv *NDS_TP = new diag::frameRecv(0);
    if (!NDS_TP->open("225.0.0.1", "10.110.144.0",
                      net_writer_c::concentrator_broadcast_port_tp)) {
        perror("Multicast receiver open failed 1.");
        exit(1);
    }

    char *bufptr = (char *)move_buf - BROADCAST_HEADER_SIZE;
    int buflen = daqd.block_size / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    buflen +=
        1024 * 100 + BROADCAST_HEADER_SIZE; // Extra overhead for the headers
    if (buflen < 64000)
        buflen = 64000;
    unsigned int seq, gps, gps_n;
    printf("Opened broadcaster receiver\n");
    gps_n = 1;

    static const int tpbuflen = 10 * 1024 * 1024;
    static char tpbuf[tpbuflen];
    char *tpbufptr = tpbuf;

    // Wait until start of a second
    while (gps_n) {
        int length = NDS->receive(bufptr, buflen, &seq, &gps, &gps_n);
        if (length < 0) {
            printf("Allocated buffer too small; required %d, size %d\n",
                   -length, buflen);
            exit(1);
        }
        printf("%d %d %d %d\n", length, seq, gps, gps_n);

        unsigned int tp_seq, tp_gps, tp_gps_n;
        int length_tp =
            NDS_TP->receive(tpbufptr, tpbuflen, &tp_seq, &tp_gps, &tp_gps_n);
        printf("%d %d %d %d\n", length_tp, tp_seq, tp_gps, tp_gps_n);
    }
    prop.gps = gps - 1;
    prop.gps_n = (1000000000 / 16) * 15;

// No waiting here if compiled as broadcasts receiver

    PV::set_pv(PV::PV_UPTIME_SECONDS, 0);
    PV::set_pv(PV::PV_GPS, 0);

    time_t zero_time = time(0); //  - 315964819 + 33;

    for (unsigned long i = 0;; i++) { // timing
        tick();                       // measure statistics

        if (((gps == prop.gps) && gps_n != prop.gps_n + 1000000000 / 16) ||
            ((gps == prop.gps + 1) &&
             (gps_n != 0 || prop.gps_n != (1000000000 / 16) * 15)) ||
            (gps > prop.gps + 1)) {
            fprintf(
                stderr,
                "Dropped broadcast block(s); gps now = %d, %d; was = %d, %d\n",
                gps, gps_n, (int)prop.gps, (int)prop.gps_n);
            exit(1);
        }

        // Update testpoints data in the main buffer
        daqd.gds.update_tp_data((unsigned int *)tpbufptr, (char *)move_buf);

        stat_crc.sample();
        // Parse received broadcast transmission header and
        // check config file CRCs and data CRCs, check DCU size and number
        // Assign DCU status and cycle.
        unsigned int *header =
            (unsigned int *)(((char *)move_buf) - BROADCAST_HEADER_SIZE);
        int ndcu = ntohl(*header++);
        // printf("ndcu = %d\n", ndcu);
        if (ndcu > 0 && ndcu <= MAX_BROADCAST_DCU_NUM) {
            int data_offs = 0; // Offset to the current DCU data
            for (int j = 0; j < ndcu; j++) {
                unsigned int dcu_number;
                unsigned int dcu_size;   // Data size for this DCU
                unsigned int config_crc; // Configuration file CRC
                unsigned int dcu_crc;    // Data CRC
                unsigned int status; // DCU status word bits (0-ok, 0xbad-out of
                                     // sync, 0x1000-trasm error
                                     // 0x2000 - configuration mismatch).
                unsigned int cycle;  // DCU cycle
                dcu_number = ntohl(*header++);
                dcu_size = ntohl(*header++);
                config_crc = ntohl(*header++);
                dcu_crc = ntohl(*header++);
                status = ntohl(*header++);
                cycle = ntohl(*header++);
                int ifo = 0;
                if (dcu_number > DCU_COUNT) {
                    ifo = 1;
                    dcu_number -= DCU_COUNT;
                }
                // printf("dcu=%d size=%d config_crc=0x%x crc=0x%x status=0x%x
                // cycle=%d\n",
                // dcu_number, dcu_size, config_crc, dcu_crc, status, cycle);
                if (daqd.dcuSize[ifo][dcu_number]) { // Don't do anything if
                                                     // this DCU is not
                                                     // configured
                    daqd.dcuStatus[ifo][dcu_number] = status;
                    daqd.dcuCycle[ifo][dcu_number] = cycle;
                    if (status ==
                        0) { // If the DCU status is OK from the concentrator
                        // Check for local configuration and data mismatch
                        if (config_crc != daqd.dcuConfigCRC[ifo][dcu_number]) {
                            // Detected local configuration mismach
                            daqd.dcuStatus[ifo][dcu_number] |= 0x2000;
                        }
                        unsigned char *cp =
                            move_buf + data_offs;      // Start of data
                        unsigned int bytes = dcu_size; // DCU data size
                        unsigned int crc = 0;
                        // Calculate DCU data CRC
                        while (bytes--) {
                            crc = (crc << 8) ^
                                  crctab[((crc >> 24) ^ *(cp++)) & 0xFF];
                        }
                        bytes = dcu_size;
                        while (bytes > 0) {
                            crc = (crc << 8) ^
                                  crctab[((crc >> 24) ^ bytes) & 0xFF];
                            bytes >>= 8;
                        }
                        crc = ~crc & 0xFFFFFFFF;
                        if (crc != dcu_crc) {
                            // Detected data corruption !!!
                            daqd.dcuStatus[ifo][dcu_number] |= 0x1000;
                            DEBUG1(printf(
                                "ifo=%d dcu=%d calc_crc=0x%x data_crc=0x%x\n",
                                ifo, dcu_number, crc, dcu_crc));
                        }
                    }
                }
                data_offs += dcu_size;
            }
        }
        stat_crc.tick();
        // :TODO: make sure all DCUs configuration matches; restart when the
        // mismatch detected

        prop.gps = gps;
        prop.gps_n = gps_n;

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

        stat_recv.sample();
        for (;;) {
            int old_seq = seq;
            int length = NDS->receive(bufptr, buflen, &seq, &gps, &gps_n);
            // DEBUG1(printf("%d %d %d %d\n", length, seq, gps, gps_n));
            // Strangely we receiver duplicate blocks on solaris for some reason
            // Looks like this happens when the data is lost...
            if (seq == old_seq) {
                printf("received duplicate NDS DAQ broadcast sequence %d; "
                       "prevpg = %d %d; gps=%d %d; length = %d\n",
                       seq, (int)prop.gps, (int)prop.gps_n, gps, gps_n, length);
            } else
                break;
        }
        stat_recv.tick();

        // TODO: check on the continuity of the sequence and GPS time here
        unsigned int tp_seq, tp_gps, tp_gps_n;
        do {
            int tp_length = NDS_TP->receive(tpbufptr, tpbuflen, &tp_seq,
                                            &tp_gps, &tp_gps_n);
            // DEBUG1(printf("TP %d %d %d %d\n", tp_length, tp_seq, tp_gps,
            // tp_gps_n));
            // gps = tp_gps; gps_n = tp_gps_n;
        } while (tp_gps < gps || (tp_gps == gps && tp_gps_n < gps_n));
        if (tp_gps != gps || tp_gps_n != gps_n) {
            fprintf(stderr, "Invalid broadcast received; seq=%d tp_seq=%d "
                            "gps=%d tp_gps=%d gps_n=%d tp_gps_n=%d\n",
                    seq, tp_seq, gps, tp_gps, gps_n, tp_gps_n);
            exit(1);
        }
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

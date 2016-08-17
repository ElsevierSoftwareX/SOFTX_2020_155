
#define _XOPEN_SOURCE 600
#define _BSD_SOURCE 1

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
#include <cctype> // old <ctype.h>
#include <sys/prctl.h>
#include <vector>
#include <stack>
#include <memory>

using namespace std;

#include "circ.hh"
#include "daqd.hh"
#include "sing_list.hh"
#include "drv/cdsHardware.h"
#include "gm_rcvr.hh"
#include <netdb.h>
#include "net_writer.hh"
#include "drv/cdsHardware.h"

#include <sys/ioctl.h>
#include "../drv/rfm.c"
#include "epics_pvs.hh"
#include "conv.hh"

#include "raii.hh"

extern daqd_c daqd;
extern int shutdown_server();
extern unsigned int crctab[256];

#if __GNUC__ >= 3
extern long int altzone;
#endif

struct ToLower {
    char operator()(char c) const { return std::tolower(c); }
};

namespace {

    struct producer_buf {
        struct put_dpvec *vmic_pv;
        int vmic_pv_len;
        unsigned char *move_buf;
        unsigned int gps;
        unsigned int gps_n;
    };

    class producer_work_queue {
        pthread_mutex_t _lock;
        pthread_cond_t _wait;

        typedef producer_buf* queue_entry;
        typedef std::stack<queue_entry, std::vector<queue_entry> > queue;

        std::vector<queue> _queues;

        producer_work_queue(const producer_work_queue &other);
        producer_work_queue operator=(const producer_work_queue &other);
    public:
        producer_work_queue(size_t num_queues): _queues(num_queues) {
            pthread_mutex_init(&_lock, NULL);
            pthread_cond_init(&_wait, NULL);
        }

        ~producer_work_queue() {
            pthread_mutex_destroy(&_lock);
        }

        void add_to_queue(size_t index, queue_entry entry) {
            if (!entry)
                return;
            raii::lock_guard<pthread_mutex_t> lock(_lock);
            _queues.at(index).push(entry);
            pthread_cond_broadcast(&_wait);
        }

        queue_entry get_from_queue(size_t index) {
            raii::lock_guard<pthread_mutex_t> lock(_lock);
            while (_queues.at(index).empty())
                pthread_cond_wait(&_wait, &_lock);
            queue &cur_queue = _queues.at(index);
            queue_entry val = cur_queue.top();
            cur_queue.pop();
            return val;
        }

        void dump_status(ostream &out) {
            raii::lock_guard<pthread_mutex_t> lock(_lock);
            int counter = 0;
            out << "work queue status" << endl;
            for (std::vector<queue>::iterator cur = _queues.begin(); cur != _queues.end(); ++cur) {
                out << "\tqueue " << counter++ << cur->size() << endl;
            }
        }
    };

    class dbl_buf_state {
        producer_buf _buffers[2];

        int _wait_count;
        pthread_mutex_t _lock;
        pthread_cond_t _wait;

        time_t _gps;
        time_t _gps_n;
    public:
        dbl_buf_state(): _wait_count(0), _gps(0), _gps_n(0) {
            pthread_mutex_init(&_lock, NULL);
            pthread_cond_init(&_wait, NULL);
        }
        ~dbl_buf_state() {
            pthread_mutex_destroy(&_lock);
        }

        void set_buffers(producer_buf buf1, producer_buf buf2) {
            _buffers[0] = buf1;
            _buffers[1] = buf2;
        }

        producer_buf *get_buffer(int index) {
            if (index < 0 || index >= 2) {
                cerr << "Attempting to retreive an invalid buffer from the double buffer" << endl;
                exit(1);
            }
            return &_buffers[index];
        }

        /// The double buffer has two working threads that must sync.
        /// when they are finished with work.
        /// This should be called by both threads at the synchronization point.
        void wait_crc(time_t &gps, time_t &gps_n) {
            raii::lock_guard<pthread_mutex_t> lock(_lock);
            // if no one is done prior to this call (ie only me)
            if (_wait_count == 0) {
                _wait_count = 1;
                while (_wait_count > 0)
                    pthread_cond_wait(&_wait, &_lock);
            } else {
                // the other thead is done, signal it and go on
                _wait_count = 0;
                pthread_cond_broadcast(&_wait);
            }
            gps = _gps;
            gps_n = _gps_n;
        }

        /// The double buffer has two working threads that must sync.
        /// when they are finished with work.
        /// This should be called by both threads at the synchronization point.
        void wait_recv(time_t gps, time_t gps_n) {
            raii::lock_guard<pthread_mutex_t> lock(_lock);
            _gps = gps;
            _gps_n = gps;
            // if no one is done prior to this call (ie only me)
            if (_wait_count == 0) {
                _wait_count = 1;
                while (_wait_count > 0)
                    pthread_cond_wait(&_wait, &_lock);
            } else {
                // the other thead is done, signal it and go on
                _wait_count = 0;
                pthread_cond_broadcast(&_wait);
            }
        }
    };

}


/* GM and shared memory communication area */
/// Point into shared memory for the controller DCU cycle
#define controller_cycle (shmemDaqIpc[daqd.controller_dcu]->cycle)

#define SHMEM_DAQ 1
#include "../../src/include/daqmap.h"
#include "../../src/include/drv/fb.h"

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

/// The main data movement thread (the producer)
void *producer::frame_writer() {
    unsigned char *read_dest;
    circ_buffer_block_prop_t prop;

    unsigned long stat_cycles = 0;
    stats stat_recv;

    DEBUG(5, cerr << "producer::frame_writer()" << endl);
    // Set thread parameters
    daqd_c::set_thread_priority("Producer", "dqprod", PROD_THREAD_PRIORITY,
                                PROD_CPUAFFINITY);

    // can probably get rid of this too
    if (!daqd.no_myrinet) {
        DEBUG(5, cerr << "no_myrinet" << endl);
        unsigned int max_endpoints = 1;
        static const unsigned int nics_available = 1;
        max_endpoints &= 0xff;

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
    }


    // this leaks for now, along with it's associated buffers
    producer_work_queue *work_queue = new producer_work_queue(2);

    {
        for (int i = 0; i < 3; ++i) {
            auto_ptr<producer_buf> _buf(new producer_buf);
            raii::array_ptr<struct put_dpvec> _buf_vmic_pv(new struct put_dpvec[MAX_CHANNELS]);
            _buf->move_buf = NULL;
            _buf->vmic_pv_len = 0;
            _buf->vmic_pv = _buf_vmic_pv.get();

            daqd.initialize_vmpic(&(_buf->move_buf), &(_buf->vmic_pv_len), _buf->vmic_pv);
            raii::array_ptr<unsigned char>  _mbuf(_buf->move_buf);

            work_queue->add_to_queue(0, _buf.get());
            _buf.release();
            _mbuf.release();
            _buf_vmic_pv.release();
        }
        DEBUG(5, work_queue->dump_status(cerr));
    }

    DEBUG(5, cerr << "created buffers and stocked work queue" << endl);
    _dbl_buf_hack = reinterpret_cast<void *>(work_queue);
    producer_buf *cur_buffer = work_queue->get_from_queue(0);
    DEBUG(5, cerr << "producer thread has buffer from queue, about to launch crc thread" << endl);

    // Start up the CRC and transfer thread
    {
        DEBUG(4, cerr << "starting producer crc thread" << endl);
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, daqd.thread_stack_size);
        pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

        raii::lock_guard<pthread_mutex_t> crc_sync(prod_crc_mutex);

        int err = pthread_create(&crc_tid, &attr,
                                 (void *(*)(void *))this->frame_writer_crc_static,
                                 (void *)this);
        if (err) {
            pthread_attr_destroy(&attr);
            system_log(1, "pthread_create() err=%d while creating producer crc thread", err);
            exit(1);
        }
        pthread_attr_destroy(&attr);

        pthread_cond_wait(&prod_crc_cond, &prod_crc_mutex);
        DEBUG(5, cerr << "producer threads synced" << endl);
    }

    // TODO make IP addresses configurable from daqdrc
    stat_recv.sample();
    diag::frameRecv *NDS = new diag::frameRecv(0);
    if (!NDS->open("225.0.0.1", "10.110.144.0",
                   net_writer_c::concentrator_broadcast_port)) {
        perror("Multicast receiver open failed.");
        exit(1);
    }
    stat_recv.tick();


    int buflen = daqd.block_size / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    buflen +=
        1024 * 100 + BROADCAST_HEADER_SIZE; // Extra overhead for the headers
    if (buflen < 64000)
        buflen = 64000;
    unsigned int seq, gps, gps_n;
    printf("Opened broadcaster receiver\n");
    gps_n = 1;

    char *bufptr = (char *)(cur_buffer->move_buf) - BROADCAST_HEADER_SIZE;
    // Wait until start of a second
    while (gps_n) {
        int length = NDS->receive(bufptr, buflen, &seq, &gps, &gps_n);
        if (length < 0) {
            printf("Allocated buffer too small; required %d, size %d\n",
                   -length, buflen);
            exit(1);
        }
        printf("%d %d %d %d\n", length, seq, gps, gps_n);
    }
    prop.gps = gps - 1;
    prop.gps_n = (1000000000 / 16) * 15;

    // No waiting here if compiled as broadcasts receiver

    time_t zero_time = time(0); //  - 315964819 + 33;

    int prev_controller_cycle = -1;
    int dcu_cycle = 0;
    int resync = 0;

    if (daqd.dcu_status_check & 4)
        resync = 1;

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


        ++stat_cycles;
        if (stat_cycles >= 16) {
            // for now this is the same as the recv time.
            PV::set_pv(PV::PV_PRDCR_TIME_FULL_MIN_MS, conv::s_to_ms_int(stat_recv.getMin()));
            PV::set_pv(PV::PV_PRDCR_TIME_FULL_MAX_MS, conv::s_to_ms_int(stat_recv.getMax()));
            PV::set_pv(PV::PV_PRDCR_TIME_FULL_MEAN_MS, conv::s_to_ms_int(stat_recv.getMean()));

            PV::set_pv(PV::PV_PRDCR_TIME_RECV_MIN_MS, conv::s_to_ms_int(stat_recv.getMin()));
            PV::set_pv(PV::PV_PRDCR_TIME_RECV_MAX_MS, conv::s_to_ms_int(stat_recv.getMax()));
            PV::set_pv(PV::PV_PRDCR_TIME_RECV_MEAN_MS, conv::s_to_ms_int(stat_recv.getMean()));

            DEBUG(5, {
                cout << "producer thread timings" << endl;
                cout << " recv min:" << stat_recv.getMin()
                     << " mean:" << stat_recv.getMean()
                     << " max:" << stat_recv.getMax() << endl;
            });
            stat_recv.clearStats();
            stat_cycles = 0;
        }

        prop.gps = gps;
        prop.gps_n = gps_n;

        // synchronize with the other thread and swap buffers
        cur_buffer->gps = gps;
        cur_buffer->gps_n = gps_n;
        work_queue->add_to_queue(1, cur_buffer);
        cur_buffer = work_queue->get_from_queue(0);

        stat_recv.sample();
        bufptr = (char *)(cur_buffer->move_buf) - BROADCAST_HEADER_SIZE;
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

        prev_controller_cycle = controller_cycle;
    }
}

void *producer::frame_writer_crc() {
    int stat_cycles = 0;

    circ_buffer_block_prop_t prop;
    stats stat_full, stat_crc, stat_transfer;

    PV::set_pv(PV::PV_UPTIME_SECONDS, 0);
    PV::set_pv(PV::PV_GPS, 0);

    // Set thread parameters
    daqd_c::set_thread_priority("Producer crc", "dqprodcrc",
                                PROD_CRC_THREAD_PRIORITY, PROD_CRC_CPUAFFINITY);
    pthread_mutex_lock(&prod_crc_mutex);
    pthread_cond_signal(&prod_crc_cond);
    pthread_mutex_unlock(&prod_crc_mutex);

    producer_work_queue *work_queue = reinterpret_cast<producer_work_queue *>(_dbl_buf_hack);

    for (unsigned long i = 0;; ++i) {
        unsigned int gps, gps_n;

        producer_buf *cur_buffer = work_queue->get_from_queue(1);
        gps = cur_buffer->gps;
        gps_n = cur_buffer->gps_n;
        unsigned char *move_buf = cur_buffer->move_buf;

        stat_full.sample();
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
                            move_buf + data_offs; // Start of data
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
        int nbi =
            daqd.b1->put16th_dpscattered(cur_buffer->vmic_pv, cur_buffer->vmic_pv_len, &prop);
        stat_transfer.tick();
        stat_full.tick();

	work_queue->add_to_queue(0, cur_buffer);
	cur_buffer = NULL;

        //  printf("%d %d\n", prop.gps, prop.gps_n);
        // DEBUG1(cerr << "producer " << i << endl);

        PV::set_pv(PV::PV_CYCLE, i);
        PV::set_pv(PV::PV_GPS, prop.gps);
        // DEBUG1(cerr << "gps=" << PV::pv(PV::PV_GPS) << endl);
        if (i % 16 == 0) {
            // Count how many seconds we were acquiring data
            PV::pv(PV::PV_UPTIME_SECONDS)++;
        }


        ++stat_cycles;
        if (stat_cycles >= 16) {
            PV::set_pv(PV::PV_PRDCR_CRC_TIME_FULL_MIN_MS, conv::s_to_ms_int(stat_full.getMin()));
            PV::set_pv(PV::PV_PRDCR_CRC_TIME_FULL_MAX_MS, conv::s_to_ms_int(stat_full.getMax()));
            PV::set_pv(PV::PV_PRDCR_CRC_TIME_FULL_MEAN_MS, conv::s_to_ms_int(stat_full.getMean()));

            PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MIN_MS, conv::s_to_ms_int(stat_crc.getMin()));
            PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MAX_MS, conv::s_to_ms_int(stat_crc.getMax()));
            PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MEAN_MS, conv::s_to_ms_int(stat_crc.getMean()));

            PV::set_pv(PV::PV_PRDCR_CRC_TIME_XFER_MIN_MS, conv::s_to_ms_int(stat_transfer.getMin()));
            PV::set_pv(PV::PV_PRDCR_CRC_TIME_XFER_MAX_MS, conv::s_to_ms_int(stat_transfer.getMax()));
            PV::set_pv(PV::PV_PRDCR_CRC_TIME_XFER_MEAN_MS, conv::s_to_ms_int(stat_transfer.getMean()));

            DEBUG(5, {
                cout << "producer crc thread timings" << endl;
                cout << " full min:" << stat_full.getMin()
                     << " mean:" << stat_full.getMean()
                     << " max:" << stat_full.getMax() << endl;
                cout << " crc  min:" << stat_crc.getMin()
                     << " mean:" << stat_crc.getMean()
                     << " max:" << stat_crc.getMax() << endl;
                cout << " xfer min:" << stat_transfer.getMin()
                     << " mean:" << stat_transfer.getMean()
                     << " max:" << stat_transfer.getMax() << endl;
            });
            stat_full.clearStats();
            stat_crc.clearStats();
            stat_transfer.clearStats();
            stat_cycles = 0;
        }
    }

    return NULL;
}

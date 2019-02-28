//
/// @file mx2ix.c
/// @brief  DAQ data concentrator code. Receives data via Open-MX and sends via Dolphin IX..
//

#include "myriexpress.h"
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "../drv/crc.c"
#include <time.h>
#include "../include/daqmap.h"
#include "../include/daq_core.h"
#include "dc_utils.h"

#include "sisci_types.h"
#include "sisci_api.h"
#include "sisci_error.h"
#include "sisci_demolib.h"
#include "testlib.h"

#include "simple_pv.h"

#define __CDECL

#define DO_HANDSHAKE 0


#include "./dolphin_common.c"

#define MIN_DELAY_MS 5
#define MAX_DELAY_MS 40

#define MAX_FE_COMPUTERS 32

#define MX_MUTEX_T pthread_mutex_t
#define MX_MUTEX_INIT(mutex_) pthread_mutex_init(mutex_, 0)
#define MX_MUTEX_LOCK(mutex_) pthread_mutex_lock(mutex_)
#define MX_MUTEX_UNLOCK(mutex_) pthread_mutex_unlock(mutex_)

#define MX_THREAD_T pthread_t
#define MX_THREAD_CREATE(thread_, start_routine_, arg_) \
pthread_create(thread_, 0, start_routine_, arg_)
#define MX_THREAD_JOIN(thread_) pthread_join(thread, 0)

MX_MUTEX_T stream_mutex;

#define FILTER     0x12345
#define MATCH_VAL  0xabcdef
#define DFLT_EID   1
#define DFLT_LEN   8192
#define DFLT_END   128
#define MAX_LEN    (1024*1024*1024) 
#define DFLT_ITER  1000
#define NUM_RREQ   16  /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/
#define NUM_SREQ   256  /* currently constrained by  MX_MCP_RDMA_HANDLES_CNT*/

#define DO_HANDSHAKE 0
#define MATCH_VAL_MAIN (1 << 31)
#define MATCH_VAL_THREAD 1
#define THREADS_PER_NIC     16
#define IX_STOP_SEC     5
static int xmitDataOffset[IX_BLOCK_COUNT];
daq_multi_cycle_header_t *xmitHeader[IX_BLOCK_COUNT];

extern void *findSharedMemorySize(char *,int);

int do_verbose = 0;

struct thread_info {
    int index;
    uint32_t match_val;
    uint32_t tpn;
    uint32_t bid;
};
struct thread_mon_info {
    int index;
    void *ctx;
};
struct thread_info thread_index[DCU_COUNT];
char *sname[DCU_COUNT]; // Names of FE computers serving DAQ data
char *local_iface[MAX_FE_COMPUTERS];
daq_multi_dcu_data_t mxDataBlockSingle[MAX_FE_COMPUTERS];
int64_t dataRecvTime[MAX_FE_COMPUTERS];
const int mc_header_size = sizeof(daq_multi_cycle_header_t);
int stop_working_threads = 0;
int start_acq = 0;
static volatile int keepRunning = 1;
int thread_cycle[MAX_FE_COMPUTERS];
int thread_timestamp[MAX_FE_COMPUTERS];
int rcv_errors = 0;
int dataRdy[MAX_FE_COMPUTERS];

void
usage()
{
    fprintf(stderr, "Usage: mx2ix [args] -s server names -m shared memory size -g IX channel \n");
    fprintf(stderr, "-l filename - log file name\n");
    fprintf(stderr, "-s - number of FE computers to connect (1-32): Default = 32\n");
    fprintf(stderr, "-v - verbose prints diag test data\n");
    fprintf(stderr, "-g - Dolphin IX channel to xmit on (0-3)\n");
    fprintf(stderr, "-p - Debug pv prefix, requires -P as well\n");
    fprintf(stderr, "-P - Path to a named pipe to send PV debug information to\n");
    fprintf(stderr, "-d - Max delay in milli seconds to wait for a FE to send data, defaults to 10\n");
    fprintf(stderr, "-t - Number of rcvr threads per NIC: default = 16\n");
    fprintf(stderr, "-n - Data Concentrator number (0 or 1) : default = 0\n");
    fprintf(stderr, "-h - help\n");
}

// *************************************************************************
// Timing Diagnostic Routine
// *************************************************************************
static int64_t
s_clock (void)
{
struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

// *************************************************************************
// Catch Control C to end cod in controlled manner
// *************************************************************************
void intHandler(int dummy) {
    keepRunning = 0;
}

void sigpipeHandler(int dummy)
{}

// **********************************************************************************************
void print_diags(int nsys, int lastCycle, int sendLength, daq_multi_dcu_data_t *ixDataBlock,int dbs[]) {
// **********************************************************************************************
    int ii = 0;
        // Print diags in verbose mode
        fprintf(stderr,"Receive errors = %d\n",rcv_errors);
        fprintf(stderr,"Time = %d\t size = %d\n",ixDataBlock->header.dcuheader[0].timeSec,sendLength);
        fprintf(stderr,"DCU ID\tCycle \t TimeSec\tTimeNSec\tDataSize\tTPCount\tTPSize\tXmitSize\n");
        for(ii=0;ii<nsys;ii++) {
            fprintf(stderr,"%d",ixDataBlock->header.dcuheader[ii].dcuId);
            fprintf(stderr,"\t%d",ixDataBlock->header.dcuheader[ii].cycle);
            fprintf(stderr,"\t%d",ixDataBlock->header.dcuheader[ii].timeSec);
            fprintf(stderr,"\t%d",ixDataBlock->header.dcuheader[ii].timeNSec);
            fprintf(stderr,"\t\t%d",ixDataBlock->header.dcuheader[ii].dataBlockSize);
            fprintf(stderr,"\t\t%d",ixDataBlock->header.dcuheader[ii].tpCount);
            fprintf(stderr,"\t%d",ixDataBlock->header.dcuheader[ii].tpBlockSize);
            fprintf(stderr,"\t%d",dbs[ii]);
            fprintf(stderr,"\n ");
        }
}


// *************************************************************************
// Thread for receiving DAQ data via Open-MX
// *************************************************************************
void *rcvr_thread(void *arg) {
    struct thread_info* my_info = (struct thread_info*)arg;
    int mt = my_info->index;
    uint32_t mv = my_info->match_val;
    uint32_t board_id = my_info->bid;
    uint32_t tpn = my_info->tpn;
    int cycle = 0;
    daq_multi_dcu_data_t *mxDataBlock;
    mx_return_t ret;
    int count, len, cur_req;
    mx_status_t stat;
    mx_request_t req[NUM_RREQ];
    mx_segment_t seg;
    uint32_t result;
    char *buffer;
    uint32_t filter;
    mx_endpoint_t ep;
    int myErrorStat = 0;

    filter = FILTER;
    int copySize;


    int dblock = board_id * tpn + mt;
    fprintf(stderr,"Starting receive loop for thread %d %d\n", board_id,mt);

    ret = mx_open_endpoint(board_id, mt, filter, NULL, 0, &ep);
    if (ret != MX_SUCCESS) {
        fprintf(stderr, "Failed to open endpoint %s\n", mx_strerror(ret));
        return(0);
    }

    fprintf(stderr,"waiting for someone to connect on CH %d\n",mt);
    len = 0xf00000;
    fprintf(stderr,"buffer length = %d\n",len);
    buffer = (char *)malloc(len);
    if (buffer == NULL) {
       fprintf(stderr, "Can't allocate buffers here\n");
       mx_close_endpoint(ep);
       return(0);
    }
    len /= NUM_RREQ;
    fprintf(stderr," length = %d\n",len);

    for (cur_req = 0; cur_req < NUM_RREQ; cur_req++) {
       seg.segment_ptr = &buffer[cur_req * len];
       seg.segment_length = len;
       mx_irecv(ep, &seg, 1, mv, MX_MATCH_MASK_NONE, 0,
       &req[cur_req]);
    }

    mx_set_error_handler(MX_ERRORS_RETURN);

    char *daqbuffer = (char *)&mxDataBlockSingle[dblock];
    do {
      for (count = 0; count < NUM_RREQ; count++) {
         cur_req = count & (16 - 1);
         mx_wait(ep, &req[cur_req],150, &stat, &result);
         myErrorStat = 0;
         if (!result) {
             myErrorStat = 1;
             count --;
         }
         if (stat.code != MX_STATUS_SUCCESS) {
            myErrorStat = 2;
         }
         if(!myErrorStat) {
         seg.segment_ptr = &buffer[cur_req * len];
        mxDataBlock = (daq_multi_dcu_data_t *)seg.segment_ptr;
        copySize = stat.xfer_length;
        memcpy(daqbuffer,seg.segment_ptr,copySize);
        // Get the message DAQ cycle number
        cycle = mxDataBlock->header.dcuheader[0].cycle;
        // Pass cycle and timestamp data back to main process
        thread_cycle[dblock] = cycle;
        thread_timestamp[dblock] = mxDataBlock->header.dcuheader[0].timeSec;
            dataRecvTime[dblock] = s_clock();
        mx_irecv(ep, &seg, 1, mv, MX_MATCH_MASK_NONE, 0, &req[cur_req]);
        }

    // Run until told to stop by main thread
      }
    } while(!stop_working_threads);
    fprintf(stderr,"Stopping thread %d\n",mt);
    usleep(200000);

    mx_close_endpoint(ep);
    return(0);

}

// *************************************************************************
// Main Process
// *************************************************************************
int
main(int argc, char **argv)
{
    pthread_t thread_id[MAX_FE_COMPUTERS];
    unsigned int nsys = MAX_FE_COMPUTERS; // The number of mapped shared memories (number of data sources)
    char *buffer_name = "ifo";
    int c;
    int ii;                 // Loop counter
    int delay_ms = 10;
    int delay_cycles = 0;

    extern char *optarg;    // Needed to get arguments to program

    // PV/debug information
    char *pv_prefix = 0;
    char *pv_debug_pipe_name = 0;
    int pv_debug_pipe = -1;

    // Declare shared memory data variables
    daq_multi_cycle_header_t *ifo_header;
    char *ifo;
    char *ifo_data;
    int cycle_data_size;
    daq_multi_dcu_data_t *ifoDataBlock;
    char *nextData;
    int max_data_size_mb = 100;
    int max_data_size = 0;
    char *mywriteaddr;
    // daq_multi_cycle_header_t *xmitHeader[IX_BLOCK_COUNT];
    // static int xmitDataOffset[IX_BLOCK_COUNT];
    int xmitBlockNum = 0;
    int dc_number = 1;
    

    /* set up defaults */
    int xmitData = 0;
    int tpn = THREADS_PER_NIC;



    // Get arguments sent to process
    while ((c = getopt(argc, argv, "b:hs:m:g:vp:P:d:l:t:n:")) != EOF) switch(c) {
    case 's':
        nsys = atoi(optarg);
        if(nsys > MAX_FE_COMPUTERS) {
            fprintf(stderr,"Max number of FE computers is 32\n");
            usage();
            exit(1);
        }
        break;
    case 'v':
        do_verbose = 1;
        break;
    case 'n':
        dc_number = atoi(optarg);
        dc_number ++;
        if(dc_number > 2) {
            fprintf(stderr,"DC number must be 0 or 1\n");
            usage();
            exit(1);
        }
        break;
    case 'm':
        max_data_size_mb = atoi(optarg);
        if (max_data_size_mb < 20){
            fprintf(stderr,"Min data block size is 20 MB\n");
            return -1;
        }
        if (max_data_size_mb > 100){
            fprintf(stderr,"Max data block size is 100 MB\n");
            return -1;
        }
        break;
    case 'g':
            segmentId = atoi(optarg);
            xmitData = 1;
            break;
    case 't':
            tpn = atoi(optarg);
            break;
    case 'b':
        buffer_name = optarg;
        break;
    case 'p':
        pv_prefix = optarg;
        break;
    case 'P':
        pv_debug_pipe_name = optarg;
        break;
    case 'd':
        delay_ms = atoi(optarg);
        if (delay_ms < MIN_DELAY_MS || delay_ms > MAX_DELAY_MS) {
            fprintf(stderr,"The delay factor must be between 5ms and 40ms\n");
            return -1;
        }
        break;
    case 'l':
        if (0 == freopen(optarg, "w", stdout)) {
            perror ("freopen");
            exit (1);
        }
        setvbuf(stdout, NULL, _IOLBF, 0);
        stderr = stdout;
        break;
    case 'h':
    default:
        usage();
        exit(1);
    }
    max_data_size = max_data_size_mb * 1024*1024;
    delay_cycles = delay_ms * 10;

    mx_init();
    // MX_MUTEX_INIT(&stream_mutex);

    // set up to catch Control C
    signal(SIGINT,intHandler);
    // setup to ignore sig pipe
    signal(SIGPIPE, sigpipeHandler);

    fprintf(stderr,"Num of sys = %d\n",nsys);

    // Get pointers to local DAQ mbuf
    ifo = (char *)findSharedMemorySize(buffer_name,max_data_size_mb);
    ifo_header = (daq_multi_cycle_header_t *)ifo;
    ifo_data = (char *)ifo + sizeof(daq_multi_cycle_header_t);
    // Following line breaks daqd for some reason
    // max_data_size *= 1024 * 1024;
    cycle_data_size = (max_data_size - sizeof(daq_multi_cycle_header_t)) / DAQ_NUM_DATA_BLOCKS_PER_SECOND;
    cycle_data_size -= (cycle_data_size % 8);
    fprintf (stderr,"cycle data size = %d\t%d\n",cycle_data_size, max_data_size_mb);
    sleep(3);
    ifo_header->cycleDataSize = cycle_data_size;
    ifo_header->maxCycle = DAQ_NUM_DATA_BLOCKS_PER_SECOND;

    if(xmitData) {
        // Connect to Dolphin
        error = dolphin_init();
        fprintf(stderr,"Read = 0x%lx \n Write = 0x%lx \n",(long)readAddr,(long)writeAddr);

        // Set pointer to xmit header in Dolphin xmit data area.
        mywriteaddr = (char *)writeAddr;
        for(ii=0;ii<IX_BLOCK_COUNT;ii++) {
            xmitHeader[ii] = (daq_multi_cycle_header_t *)mywriteaddr;
            mywriteaddr += IX_BLOCK_SIZE;
            xmitDataOffset[ii] = IX_BLOCK_SIZE * ii + sizeof(struct daq_multi_cycle_header_t);
            fprintf(stderr,"Dolphin at 0x%lx and 0x%lx",(long)xmitHeader[ii],(long)xmitDataOffset[ii]);
        }
    }

    fprintf(stderr,"nsys = %d\n",nsys);

    // Make 0MQ socket connections
    for(ii=0;ii<nsys;ii++) {
        // Create a thread to receive data from each data server
        thread_index[ii].index = ii % tpn;
        thread_index[ii].match_val = MATCH_VAL_MAIN;
        thread_index[ii].tpn = tpn;
        thread_index[ii].bid = ii/tpn;
        pthread_create(&thread_id[ii],NULL,rcvr_thread,(void *)&thread_index[ii]);

    }
    int nextCycle = 0;
    start_acq = 1;
    int64_t mytime = 0;
    int64_t mylasttime = 0;
    int64_t myptime = 0;
    int64_t n_cycle_time = 0;
    int mytotaldcu = 0;
    char *zbuffer;
    size_t zbuffer_remaining = 0;
    int dc_datablock_size = 0;
    int datablock_size_running = 0;
    int datablock_size_mb_s = 0;
    static const int header_size = sizeof(daq_multi_dcu_header_t);
    char dcstatus[4096];
    char dcs[48];
    int edcuid[10];
    int estatus[10];
    int edbs[10];
    unsigned long ets = 0;
    int timeout = 0;
    int threads_rdy;
    int any_rdy = 0;
    int jj,kk;
    int sendLength = 0;

    int min_cycle_time = 1 << 30;
    int pv_min_cycle_time = 0;
    int max_cycle_time = 0;
    int pv_max_cycle_time = 0;
    int mean_cycle_time = 0;
    int pv_mean_cycle_time = 0;
    int pv_dcu_count = 0;
    int pv_total_datablock_size = 0;
        int pv_datablock_size_mb_s = 0;
    int uptime = 0;
    int pv_uptime = 0;
    int gps_time = 0;
    int pv_gps_time = 0;
    // int endpoint_min_count = 1 << 30;
    // int endpoint_max_count = 0;
    // int endpoint_mean_count = 0;
    int cur_endpoint_ready_count;
    // char endpoints_missed_buffer[256];
    // int endpoints_missed_remaining;
    int missed_flag = 0;
    int missed_nsys[MAX_FE_COMPUTERS];
    int64_t recv_time[MAX_FE_COMPUTERS];
    int64_t min_recv_time = 0;
    int64_t cur_ref_time = 0;
    int recv_buckets[(MAX_DELAY_MS/5)+2];
    // int entry_binned = 0;
    int festatus = 0;
    int pv_festatus = 0;
    int ix_xmit_stop = 0;
    SimplePV pvs[] = {
            {
                    "RECV_MIN_MS",
                    SIMPLE_PV_INT,
                    &pv_min_cycle_time,

                    80,
                    45,
                    70,
                    54,
            },
            {
                    "RECV_MAX_MS",
                    SIMPLE_PV_INT,
                    &pv_max_cycle_time,

                    80,
                    45,
                    70,
                    54,
            },
            {
                    "RECV_MEAN_MS",
                    SIMPLE_PV_INT,
                    &pv_mean_cycle_time,

                    80,
                    45,
                    70,
                    54,
            },
            {
                    "DCU_COUNT",
                    SIMPLE_PV_INT,
                    &pv_dcu_count,

                    120,
                    0,
                    115,
                    0,
            },
            {
                    "DATA_SIZE",
                    SIMPLE_PV_INT,
                    &pv_total_datablock_size,

                    100*1024*1024,
                    0,
                    90*1024*1024,
                    1*1024*1024,
            },
            {
                    "DATA_RATE",
                    SIMPLE_PV_INT,
                    &pv_datablock_size_mb_s,

                    100*1024*1024,
                    0,
                    90*1024*1024,
                    1000000,
            },
            {
                    "UPTIME_SECONDS",
                    SIMPLE_PV_INT,
                    &pv_uptime,

                    100*1024*1024,
                    0,
                    90*1024*1024,

                    1,
            },
            {
                    "RCV_STATUS",
                    SIMPLE_PV_INT,
                    &pv_festatus,

                    // 100*1024*1024,
                    0xffffffff,
                    0,
                    // 90*1024*1024,
                    0xfffffffe,

                    1,
            },
            {
                    "GPS",
                    SIMPLE_PV_INT,
                    &pv_gps_time,

                    // 100*1024*1024,
                    0xfffffff,
                    0,
                    // 90*1024*1024,
                    0xfffffffe,

                    1,
            },

    };
    if (pv_debug_pipe_name)
    {
        pv_debug_pipe = open(pv_debug_pipe_name, O_NONBLOCK | O_RDWR, 0);
        if (pv_debug_pipe < 0) {
            fprintf(stderr, "Unable to open %s for writting (pv status)\n", pv_debug_pipe_name);
            exit(1);
        }
    }

    missed_flag = 1;
    memset(&missed_nsys[0], 0, sizeof(missed_nsys));
    memset(recv_buckets, 0, sizeof(recv_buckets));
    do {
        // Reset counters
        timeout = 0;
        for(ii=0;ii<nsys;ii++) dataRdy[ii] = 0;
        for(ii=0;ii<nsys;ii++) thread_cycle[ii] = 50;
        threads_rdy = 0;
        any_rdy = 0;

        // Wait up to 100ms until received data from at least 1 FE or timeout
        do {
            usleep(2000);
            for(ii=0;ii<nsys;ii++) {
                if(nextCycle == thread_cycle[ii]) any_rdy = 1;
            }
            timeout += 1;
        }while(!any_rdy && timeout < 50);

        // Wait up to delay_ms ms in 1/10ms intervals until data received from everyone or timeout
        timeout = 0;
        do {
            usleep(100);
            for(ii=0;ii<nsys;ii++) {
                if(nextCycle == thread_cycle[ii] && !dataRdy[ii]) threads_rdy ++;
                if(nextCycle == thread_cycle[ii]) dataRdy[ii] = 1;
            }
            timeout += 1;
        }while(threads_rdy < nsys && timeout < delay_cycles);
        if(timeout >= 100) rcv_errors += (nsys - threads_rdy);

        if(any_rdy) {
            int tbsize = 0;
            // Timing diagnostics
            mytime = s_clock();
            myptime = mytime - mylasttime;
            mylasttime = mytime;
            if (myptime < min_cycle_time) {
                min_cycle_time = myptime;
            }
            if (myptime > max_cycle_time) {
                max_cycle_time = myptime;
            }
            mean_cycle_time += myptime;
            ++n_cycle_time;

            // Reset total DCU counter
            mytotaldcu = 0;
            // Reset total DC data size counter
            dc_datablock_size = 0;
            // Get pointer to next data block in shared memory
            nextData = (char *)ifo_data;
            nextData += cycle_data_size * nextCycle;
            ifoDataBlock = (daq_multi_dcu_data_t *)nextData;
            zbuffer = (char *)nextData + header_size;
            zbuffer_remaining = cycle_data_size - header_size;

            cur_endpoint_ready_count = 0;
            // endpoints_missed_remaining = sizeof(endpoints_missed_buffer);
            // endpoints_missed_buffer[0] = '\0';
            min_recv_time = 0x7fffffffffffffff;
            festatus = 0;
            // Loop over all data buffers received from FE computers
            for(ii=0;ii<nsys;ii++) {
                recv_time[ii] = -1;
                if(dataRdy[ii]) {
                    festatus += (1 << ii);
                    recv_time[ii] = dataRecvTime[ii];
                    if (recv_time[ii] < min_recv_time) {
                        min_recv_time = recv_time[ii];
                    }
                    if (do_verbose && nextCycle == 0) {
                        fprintf(stderr, "+++%d\n", ii);
                    }
                    ++cur_endpoint_ready_count;
                    int myc = mxDataBlockSingle[ii].header.dcuTotalModels;
                    // For each model, copy over data header information
                    for(jj=0;jj<myc;jj++) {
                        // Copy data header information
                        ifoDataBlock->header.dcuheader[mytotaldcu].dcuId = mxDataBlockSingle[ii].header.dcuheader[jj].dcuId;
                        ifoDataBlock->header.dcuheader[mytotaldcu].fileCrc = mxDataBlockSingle[ii].header.dcuheader[jj].fileCrc;
                        ifoDataBlock->header.dcuheader[mytotaldcu].status = mxDataBlockSingle[ii].header.dcuheader[jj].status;
                        ifoDataBlock->header.dcuheader[mytotaldcu].cycle = mxDataBlockSingle[ii].header.dcuheader[jj].cycle;
                        if(!ix_xmit_stop) {
                        ifoDataBlock->header.dcuheader[mytotaldcu].timeSec = mxDataBlockSingle[ii].header.dcuheader[jj].timeSec;
                        ifoDataBlock->header.dcuheader[mytotaldcu].timeNSec = mxDataBlockSingle[ii].header.dcuheader[jj].timeNSec;
                        }
                        ifoDataBlock->header.dcuheader[mytotaldcu].dataCrc = mxDataBlockSingle[ii].header.dcuheader[jj].dataCrc;
                        ifoDataBlock->header.dcuheader[mytotaldcu].dataBlockSize = mxDataBlockSingle[ii].header.dcuheader[jj].dataBlockSize;
                        ifoDataBlock->header.dcuheader[mytotaldcu].tpBlockSize = mxDataBlockSingle[ii].header.dcuheader[jj].tpBlockSize;
                        ifoDataBlock->header.dcuheader[mytotaldcu].tpCount = mxDataBlockSingle[ii].header.dcuheader[jj].tpCount;
                        if(mxDataBlockSingle[ii].header.dcuheader[jj].dcuId == 52 && mxDataBlockSingle[ii].header.dcuheader[jj].tpCount == dc_number)
                        {
                            fprintf(stderr,"Got a DAQ Reset REQUEST %u\n",mxDataBlockSingle[ii].header.dcuheader[jj].timeSec);
                            ifoDataBlock->header.dcuheader[mytotaldcu].tpCount = 0;
                            ix_xmit_stop = 16 * IX_STOP_SEC;
                        }
                        for(kk=0;kk<DAQ_GDS_MAX_TP_NUM ;kk++)   
                            ifoDataBlock->header.dcuheader[mytotaldcu].tpNum[kk] = mxDataBlockSingle[ii].header.dcuheader[jj].tpNum[kk];
                        edbs[mytotaldcu] = mxDataBlockSingle[ii].header.dcuheader[jj].tpBlockSize + mxDataBlockSingle[ii].header.dcuheader[jj].dataBlockSize;
                        // Get some diags
                        if(ifoDataBlock->header.dcuheader[mytotaldcu].status != 0xbad)
                            ets = mxDataBlockSingle[ii].header.dcuheader[jj].timeSec;
                        estatus[mytotaldcu] = ifoDataBlock->header.dcuheader[mytotaldcu].status;
                        edcuid[mytotaldcu] = ifoDataBlock->header.dcuheader[mytotaldcu].dcuId;
                        // Increment total DCU count
                        mytotaldcu ++;
                    }
                    // Get the size of the data to transfer
                    int mydbs = mxDataBlockSingle[ii].header.fullDataBlockSize;
                    // Get pointer to data in receive data block
                    char *mbuffer = (char *)&mxDataBlockSingle[ii].dataBlock[0];
                    if (mydbs > zbuffer_remaining)
                    {
                        fprintf(stderr,"Buffer overflow found.  Attempting to write %d bytes to zbuffer which has %d bytes remainging\n",
                                (int)mydbs, (int)zbuffer_remaining);
                        abort();
                    }
                    // Copy data from receive buffer to shared memory
                    memcpy(zbuffer,mbuffer,mydbs);
                    // Increment shared memory data buffer pointer for next data set
                    zbuffer += mydbs;
                    // Calc total size of data block for this cycle
                    dc_datablock_size += mydbs;
                    tbsize += mydbs;
                } else {
                    missed_nsys[ii] |= missed_flag;
                    if (do_verbose && nextCycle == 0) {
                        fprintf(stderr, "---%d\n", ii);
                    } 
                }
            }
            /*
            if (cur_endpoint_ready_count < endpoint_min_count) {
                endpoint_min_count = cur_endpoint_ready_count;
            }
            if (cur_endpoint_ready_count > endpoint_max_count) {
                endpoint_max_count = cur_endpoint_ready_count;
            }
            endpoint_mean_count += cur_endpoint_ready_count;
            */
            // Write total data block size to shared memory header
            ifoDataBlock->header.fullDataBlockSize = dc_datablock_size;
            // Write total dcu count to shared memory header
            ifoDataBlock->header.dcuTotalModels = mytotaldcu;
            // Set multi_cycle head cycle to indicate data ready for this cycle
            ifo_header->curCycle = nextCycle;
            xmitBlockNum = nextCycle % IX_BLOCK_COUNT;

            // Calc IX message size
            sendLength = header_size + ifoDataBlock->header.fullDataBlockSize;
            /*
            if (nextCycle == 0) {
                memset(recv_buckets, 0, sizeof(recv_buckets));
            }
            */
            for (ii = 0; ii < nsys; ++ii) {
                cur_ref_time = 0;
                recv_time[ii] -= min_recv_time;
                /*
                entry_binned = 0;
                for (jj = 0; jj < sizeof(recv_buckets)/sizeof(recv_buckets[0]); ++jj) {
                    if (recv_time[ii] < cur_ref_time) {
                        ++recv_buckets[jj];
                        entry_binned = 1;
                        break;
                    }
                    cur_ref_time += 5;
                }
                if (!entry_binned) {
                    ++recv_buckets[sizeof(recv_buckets)/sizeof(recv_buckets[0])-1];
                }
                */
            }
            datablock_size_running += dc_datablock_size;
            if (nextCycle == 0) {
                datablock_size_mb_s = datablock_size_running / 1024 ;
                pv_datablock_size_mb_s = datablock_size_mb_s;
                uptime ++;
                pv_uptime = uptime;
                gps_time = ifoDataBlock->header.dcuheader[0].timeSec;
                pv_gps_time = gps_time;
                pv_dcu_count = mytotaldcu;
                pv_festatus = festatus;
                // pv_total_datablock_size = dc_datablock_size;
                mean_cycle_time = (n_cycle_time > 0 ? mean_cycle_time / n_cycle_time : 1 << 31);
                /*
                endpoint_mean_count = (n_cycle_time > 0 ? endpoint_mean_count/n_cycle_time :  1<<31);

                endpoints_missed_remaining = sizeof(endpoints_missed_buffer)-1;
                endpoints_missed_buffer[0] = '\0';
                for (ii = 0; ii < sizeof(missed_nsys)/sizeof(missed_nsys[0]) && endpoints_missed_remaining > 0; ++ii) {
                    if (missed_nsys[ii]) {
                        char tmp[80];
                        int count = snprintf(tmp, sizeof(tmp), "%s(%x) ", sname[ii], missed_nsys[ii]);
                        if (count < sizeof(tmp)) {
                            strncat(endpoints_missed_buffer, tmp, count);
                            endpoints_missed_remaining -= count;
                        }       
                    }
                    missed_nsys[ii] = 0;
                }
                */
                pv_mean_cycle_time = mean_cycle_time;
                pv_max_cycle_time = max_cycle_time;
                pv_min_cycle_time = min_cycle_time;
                send_pv_update(pv_debug_pipe, pv_prefix, pvs, sizeof(pvs)/sizeof(pvs[0]));

                if (do_verbose) {
                    fprintf(stderr,"\nData rdy for cycle = %d\t\tTime Interval = %ld msec\n", nextCycle, myptime);
                    fprintf(stderr,"Min/Max/Mean cylce time %d/%d/%d msec over %ld cycles\n", min_cycle_time, max_cycle_time,
                        mean_cycle_time, n_cycle_time);
                    fprintf(stderr,"Total DCU = %d\t\t\tBlockSize = %d\n", mytotaldcu, dc_datablock_size);
                    print_diags(mytotaldcu, nextCycle, sendLength, ifoDataBlock, edbs);
                }
                n_cycle_time = 0;
                min_cycle_time = 1 << 30;
                max_cycle_time = 0;
                mean_cycle_time = 0;

                /*
                endpoint_min_count = nsys;
                endpoint_max_count = 0;
                endpoint_mean_count = 0;
                */

                missed_flag = 1;
                datablock_size_running = 0;
            } else {
                missed_flag <<= 1;
            }
            if(xmitData && !ix_xmit_stop) {
                if (sendLength > IX_BLOCK_SIZE)
                {
                    fprintf(stderr, "Buffer overflow.  Sending %d bytes into a dolphin block that holds %d\n",
                    (int)sendLength, (int)IX_BLOCK_SIZE);
                    abort();
                }
                // WRITEDATA to Dolphin Network
                SCIMemCpy(sequence,nextData, remoteMap,xmitDataOffset[xmitBlockNum],sendLength,memcpyFlag,&error);
                error = SCI_ERR_OK;
                if (error != SCI_ERR_OK) {
                    fprintf(stderr,"SCIMemCpy failed - Error code 0x%x\n",error);
                    fprintf(stderr,"For reference the expected error codes are:\n");
                    fprintf(stderr,"SCI_ERR_OUT_OF_RANGE = 0x%x\n", SCI_ERR_OUT_OF_RANGE);
                    fprintf(stderr,"SCI_ERR_SIZE_ALIGNMENT = 0x%x\n", SCI_ERR_SIZE_ALIGNMENT);
                    fprintf(stderr,"SCI_ERR_OFFSET_ALIGNMENT = 0x%x\n", SCI_ERR_OFFSET_ALIGNMENT);
                    fprintf(stderr,"SCI_ERR_TRANSFER_FAILED = 0x%x\n", SCI_ERR_TRANSFER_FAILED);
                    return error;
                }
                // Set data header information
                unsigned int maxCycle = ifo_header->maxCycle;
                unsigned int curCycle = ifo_header->curCycle;
                xmitHeader[xmitBlockNum]->maxCycle = maxCycle;
                // xmitHeader->cycleDataSize = ifo_header->cycleDataSize;
                xmitHeader[xmitBlockNum]->cycleDataSize = sendLength;;
                // xmitHeader->msgcrc = myCrc;
                // Send cycle last as indication of data ready for receivers
                xmitHeader[xmitBlockNum]->curCycle = curCycle;
                // Have to flush the buffers to make data go onto Dolphin network
                SCIFlush(sequence,SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY);
            }
            if(ix_xmit_stop)  {
                ix_xmit_stop --;
                if(ix_xmit_stop == 0) fprintf(stderr,"Restarting Dolphin Xmit\n");
            }

        }
        sprintf(dcstatus,"%ld ",ets);
        for(ii=0;ii<mytotaldcu;ii++) {
            sprintf(dcs,"%d %d %d ",edcuid[ii],estatus[ii],edbs[ii]);
            strcat(dcstatus,dcs);
        }


        // Increment cycle count
        nextCycle ++;
        nextCycle %= 16;
    }while (keepRunning);   // End of infinite loop

    // Stop Rcv Threads
    fprintf(stderr,"stopping threads %d \n",nsys);
    stop_working_threads = 1;

    // Wait for threads to stop
    sleep(5);
    fprintf(stderr,"closing out MX\n");
    mx_finalize();
    if(xmitData) {
        fprintf(stderr,"closing out ix\n");
        // Cleanup the Dolphin connections
        error = dolphin_closeout();
    }
  
    exit(0);
}

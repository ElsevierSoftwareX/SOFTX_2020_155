#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include "zmq_dc_recv.h"

#include <zmq.h>
#include <string.h>

#define MAX_ENDPOINTS 32
#define MAX_CONN_STR 256

static const int multi_dcu_header_size = sizeof(daq_multi_dcu_header_t);

typedef struct thread_info_data {
    int index;
    char conn_str[MAX_CONN_STR];
    zmq_dc_receiver_p dc_receiver;
} thread_info_data;

typedef struct data_block {
    daq_dc_data_t *full_data_block;
    int send_length;
} data_block;

typedef struct zmq_dc_receiver {
    void *context;
    volatile unsigned int tstatus[16];
    thread_info_data thread_info[MAX_ENDPOINTS];
    int data_mask;
    volatile int run_threads;
    volatile int start_acq;
    int nsys;

    int resync;
    int loop;
    int64_t mylasttime;
    /* debug info */
    int verbose;

    daq_dc_data_t* mxDataBlockFull;
    daq_multi_dcu_data_t* mxDataBlockG[MAX_ENDPOINTS];
} zmq_dc_receiver;

static int64_t
s_clock ()
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/**
 * Count the number of strings in a null terminated char**
 * @param strings input char **
 * @return The number of strings
 */
static int _count_strings(char **strings)
{
    int count = 0;

    if (strings)
    {
        for(; strings[count]; ++count) {}
    }
    return count;
}

static void _clear_status(zmq_dc_receiver_p rcv)
{
    for (int i = 0; i < 16; ++i) {
        rcv->tstatus[i] = 0;
    }
}

static void _clear_status1(zmq_dc_receiver_p rcv, int segment)
{
    rcv->tstatus[segment] = 0;
}

static unsigned int _get_status(zmq_dc_receiver_p rcv, int segment)
{
    return rcv->tstatus[segment];
}

/**
 * Run the recieving thread
 * @param arg input argument (a thread_info_data *)
 * @return NULL
 */
static void *_rcvr_thread(void *arg)
{
    thread_info_data *my_thread_info = (thread_info_data*)arg;
    zmq_dc_receiver_p rcv = my_thread_info->dc_receiver;
    int mt = my_thread_info->index;
    int ii = 0;
    int cycle = 0;
    int acquire = 0;
    daq_multi_dcu_data_t mxDataBlock;
    void *zsocket = 0;
    zmq_msg_t msg;

    zsocket = zmq_socket(rcv->context, ZMQ_SUB);
    if (!zsocket) goto cleanup;
    if (zmq_setsockopt(zsocket, ZMQ_SUBSCRIBE, "", 0) != 0) goto cleanup;
    printf("reader thread %d connecting to %s\n", mt, my_thread_info->conn_str);
    if (zmq_connect(zsocket, my_thread_info->conn_str) != 0) goto cleanup;


    printf("thread %d entering main loop\n", mt);
    do {
        if (zmq_msg_init(&msg) != 0) goto cleanup;

        /* Get data when message size > 0 */
        int msg_size = zmq_msg_recv(&msg, zsocket, 0);

        assert(zmq_msg_size(&msg) >= 0 && msg_size == zmq_msg_size(&msg));
        // Get pointer to message data
        char *string = (char*)zmq_msg_data(&msg);
        char *daqbuffer = (char *) &mxDataBlock;
        // Copy data out of 0MQ message buffer to local memory buffer
        memcpy(daqbuffer, string, msg_size);


        //printf("Received block of %d on %d\n", size, mt);
        for (ii = 0; ii < mxDataBlock.header.dcuTotalModels; ii++) {
            cycle = mxDataBlock.header.dcuheader[ii].cycle;
            // Copy data to global buffer
            char *localbuff = (char *) &(rcv->mxDataBlockG[mt][cycle]);
            memcpy(localbuff, daqbuffer, msg_size);
        }
        // Always start on cycle 0 after told to start by main thread
        if (cycle == 0 && rcv->start_acq) {
            if (acquire != 1) {
                printf("thread %d starting to acquire\n", mt);
            }
            acquire = 1;
        }
        // Set the cycle data ready bit
        if (acquire) {
            rcv->tstatus[cycle] |= (1 << mt);
            // std::cout << "." << _tstatus[cycle] << std::endl;
        }
        // if (acquire && cycle == 0)
        //    std::cout << "thread " << mt << " received " << message.size() << " bytes" << std::endl;
        // Run until told to stop by main thread
        zmq_msg_close(&msg);
    } while (rcv->run_threads);
cleanup:
    zmq_msg_close(&msg);
    if (zsocket) zmq_close(zsocket);

    printf("Stopping thread %d\n", mt);
    usleep(200000);
    return NULL;
}


static int _create_subscriber_threads(zmq_dc_receiver_p rcv, char **sname)
{
    int i = 0;
    pthread_t thread_id[MAX_ENDPOINTS];

    for (i = 0; i < rcv->nsys; ++i)
    {
        if (snprintf(rcv->thread_info[i].conn_str, MAX_CONN_STR, "tcp://%s:%d", sname[i], DAQ_DATA_PORT) >= MAX_CONN_STR)
        {
            return 0;
        }
        rcv->thread_info[i].dc_receiver = rcv;
    }

    rcv->data_mask = 0;

    for (i = 0; i < rcv->nsys; ++i)
    {
        rcv->thread_info[i].index = i;
        pthread_create(&thread_id[i], NULL, _rcvr_thread, (void *)(&rcv->thread_info[i]));
        rcv->data_mask |= (1 << i);
    }
    return 1;
}



zmq_dc_receiver_p zmq_dc_receiver_create(char **sname)
{
    int i = 0;
    int nsys = _count_strings(sname);

    if (nsys == 0 || nsys > MAX_ENDPOINTS)
        return NULL;

    zmq_dc_receiver_p rcv = calloc(sizeof(zmq_dc_receiver), 1);
    if (rcv == NULL)
        return NULL;
    /* Any failure after this point requires clean-up */
    /* Things are zero initialized, so only touch those things which need to not be 0 */
    rcv->context = zmq_ctx_new();
    if (!rcv->context) goto cleanup;
    zmq_ctx_set(rcv->context, ZMQ_IO_THREADS, nsys);
    rcv->run_threads = 1;
    rcv->nsys = nsys;
    rcv->resync = 1;

    rcv->mxDataBlockFull = malloc(sizeof(daq_dc_data_t) * 16);
    if (!rcv->mxDataBlockFull) goto cleanup;
    for (i = 0; i < MAX_ENDPOINTS; ++i)
    {
        rcv->mxDataBlockG[i] = malloc(sizeof(daq_multi_dcu_data_t) * 16);
        if (!rcv->mxDataBlockG[i]) goto cleanup;
    }
    if (!_create_subscriber_threads(rcv, sname)) goto cleanup;
    return rcv;
cleanup:
    zmq_dc_receiver_destroy(rcv);
    return NULL;
}

void zmq_dc_receiver_destroy(zmq_dc_receiver_p rcv)
{
    int i = 0;

    if (!rcv)
        return;
    /* stop the threads first here */

    if (!rcv->context)
        zmq_ctx_term(rcv->context);
    if (rcv->mxDataBlockFull)
        free(rcv->mxDataBlockFull);
    for (i = 0; i < MAX_ENDPOINTS; ++i)
    {
        if (rcv->mxDataBlockG[i])
            free(rcv->mxDataBlockG[i]);
    }
    free(rcv);
}


int zmq_dc_receiver_data_mask(const zmq_dc_receiver_p rcv)
{
    if (!rcv) return 0;
    return rcv->data_mask;
}

void zmq_dc_receiver_stop_threads(zmq_dc_receiver_p rcv)
{
    if (!rcv) return;
    rcv->run_threads = 0;
}

void zmq_dc_receiver_begin_acquiring(zmq_dc_receiver_p rcv)
{
    if (!rcv) return;
    rcv->start_acq = 1;
}

void zmq_dc_receiver_receive_data(zmq_dc_receiver_p rcv, zmq_data_block* dest)
{

    int timeout = 0;

    if (!rcv || !dest) return;

    // Wait until received data from at least 1 FE
    do {
        if (rcv->resync) {
            rcv->loop = 0;
            rcv->resync = 0;
            _clear_status(rcv);
            timeout = 0;
        }

        do {
            usleep(2000);
            timeout += 1;
        } while (_get_status(rcv, rcv->loop) == 0 && timeout < 50);
        // std::cout << get_status(rcv->loop) << ":" << data_mask() << " (" << timeout << ")" << std::endl;
        // If timeout, not getting data from anyone.
        if (timeout >= 50) rcv->resync = 1;
    } while (rcv->resync);

    // Wait until data received from everyone
    timeout = 0;
    do {
        usleep(1000);
        timeout += 1;
    } while (_get_status(rcv, rcv->loop) != rcv->data_mask && timeout < 5);
    // If timeout, not getting data from everyone.
    // TODO: MARK MISSING FE DATA AS BAD

    //if (timeout >= 5)
    //    std::cout << "TTT" << std::endl;
    //else
    //    std::cout << "###" << std::endl;

    // Clear thread rdy for this cycle
    _clear_status1(rcv, rcv->loop);

    // Timing diagnostics
    int64_t mytime = s_clock();
    int64_t myptime = mytime - rcv->mylasttime;
    rcv->mylasttime = mytime;
    // printf("Data rday for cycle = %d\t%ld\n",rcv->loop,myptime);
    // Reset total DCU counter
    int mytotaldcu = 0;
    // Set pointer to start of DC data block
    char *zbuffer = (char*)(&rcv->mxDataBlockFull[rcv->loop].dataBlock[0]);
    // Reset total DC data size counter
    int dc_datablock_size = 0;

    rcv->mxDataBlockFull[rcv->loop].header.dataBlockSize = 0;
    // Loop over all data buffers received from FE computers
    for (int ii = 0; ii < rcv->nsys; ii++) {
        int cur_sys_dcu_count = rcv->mxDataBlockG[ii][rcv->loop].header.dcuTotalModels;
        // printf("\tModel %d = %d\n",ii,cur_sys_dcu_count);
        char* mbuffer = (char*) &rcv->mxDataBlockG[ii][rcv->loop].dataBlock[0];

        for (int jj = 0; jj < cur_sys_dcu_count; jj++) {
            // Copy data header information
            rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].dcuId = rcv->mxDataBlockG[ii][rcv->loop].header.dcuheader[jj].dcuId;
            int cur_dcuid = rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].dcuId;
            rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].fileCrc = rcv->mxDataBlockG[ii][rcv->loop].header.dcuheader[jj].fileCrc;
            rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].dataCrc = rcv->mxDataBlockG[ii][rcv->loop].header.dcuheader[jj].dataCrc;
            rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].status = rcv->mxDataBlockG[ii][rcv->loop].header.dcuheader[jj].status;
            rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].status;
            if (rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].status == 0xbad)
                printf("Fault on dcuid %d\n", rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].dcuId);
            rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].cycle = rcv->mxDataBlockG[ii][rcv->loop].header.dcuheader[jj].cycle;
            rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].timeSec = rcv->mxDataBlockG[ii][rcv->loop].header.dcuheader[jj].timeSec;
            rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].timeNSec = rcv->mxDataBlockG[ii][rcv->loop].header.dcuheader[jj].timeNSec;
            int mydbs = rcv->mxDataBlockG[ii][rcv->loop].header.dcuheader[jj].dataBlockSize;

            //if (rcv->loop == 0 && do_verbose)
            //    printf("\t\tdcuid = %d ; data size= %d\n", cur_dcuid, mydbs);

            rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].dataBlockSize = mydbs;

            int mytpbs = rcv->mxDataBlockG[ii][rcv->loop].header.dcuheader[jj].tpBlockSize;
            int used_tpbs = mytpbs;
            rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].tpBlockSize = mytpbs;
            rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].tpCount = rcv->mxDataBlockG[ii][rcv->loop].header.dcuheader[jj].tpCount;
            {
                unsigned int *tp_table = &rcv->mxDataBlockG[ii][rcv->loop].header.dcuheader[jj].tpNum[0];
                unsigned int *tp_dest = &rcv->mxDataBlockFull[rcv->loop].header.dcuheader[mytotaldcu].tpNum[0];
                memcpy(tp_dest, tp_table, DAQ_GDS_MAX_TP_NUM*sizeof(*tp_table));
            }

            // Copy data to DC buffer
            memcpy(zbuffer, mbuffer, mydbs + used_tpbs);
            // Increment DC data buffer pointer for next data set
            zbuffer += mydbs + used_tpbs;
            dc_datablock_size += mydbs + used_tpbs;
            // increment mbuffer by the source tp size not the used tp size
            mbuffer += mydbs + mytpbs;
            // increment the count of bytes sent
            rcv->mxDataBlockFull[rcv->loop].header.dataBlockSize += (unsigned int)(mydbs + used_tpbs);
            mytotaldcu++;
        }
    }

    // printf("\tTotal DCU = %d\tSize = %d\n",mytotaldcu,dc_datablock_size);
    rcv->mxDataBlockFull[rcv->loop].header.dcuTotalModels = mytotaldcu;

    if (/* rcv->loop == 0 && */ rcv->verbose) {
        printf("Recieved %d bytes from %d dcuids\n", dc_datablock_size, mytotaldcu);
        for (int jj = 0; jj < mytotaldcu; ++jj) {
            printf("dcuid: %d config crc: %x data crc: %x\n",
                   rcv->mxDataBlockFull[rcv->loop].header.dcuheader[jj].dcuId,
                   rcv->mxDataBlockFull[rcv->loop].header.dcuheader[jj].fileCrc,
                   rcv->mxDataBlockFull[rcv->loop].header.dcuheader[jj].dataCrc
            );
        }
    }

    dest->send_length = multi_dcu_header_size + dc_datablock_size;
    dest->full_data_block = &rcv->mxDataBlockFull[rcv->loop];

    ++rcv->loop;
    rcv->loop %= 16;
}


int zmq_dc_receiver_get_verbose(const zmq_dc_receiver_p rcv)
{
    if (!rcv) return 0;
    return rcv->verbose;
}

void zmq_dc_receiver_set_verbose(zmq_dc_receiver_p rcv, int verbose)
{
    if (!rcv) return;
    rcv->verbose = verbose;
}


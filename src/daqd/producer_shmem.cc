
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
#include <cctype> // old <ctype.h>
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

#include "checksum_crc32.hh"

#include "epics_pvs.hh"

#include "raii.hh"
#include "conv.hh"
#include "circ.h"
#include "daq_core.h"
#include "shmem_receiver.hh"

extern daqd_c       daqd;
extern int          shutdown_server( );
extern unsigned int crctab[ 256 ];

extern long int altzone;

struct ToLower
{
    char
    operator( )( char c ) const
    {
        return std::tolower( c );
    }
};

/* GM and shared memory communication area */

/* This may still be needed for test points */
// struct rmIpcStr gmDaqIpc[DCU_COUNT];
/// DMA memory area pointers
int controller_cycle = 0;

/// Pointer to GDS TP tables
struct cdsDaqNetGdsTpNum* gdsTpNum[ 2 ][ DCU_COUNT ];

/// The main data movement thread (the producer)
void*
producer::frame_writer( )
{
    unsigned char* read_dest;
    // circ_buffer_block_prop_t prop;

    unsigned long prev_gps, prev_frac;
    unsigned long gps, frac;

    checksum_crc32 crc_obj;

    // last status value
    std::array< bool, DCU_COUNT > dcuSeenLastCycle{};
    std::fill( dcuSeenLastCycle.begin( ), dcuSeenLastCycle.end( ), false );

    // error message buffer
    char          errmsgbuf[ 80 ];
    unsigned long stat_cycles = 0;
    stats         stat_full, stat_recv, stat_crc, stat_transfer;

    // Set thread parameters
    daqd_c::set_thread_priority(
        "Producer", "dqprod", PROD_THREAD_PRIORITY, PROD_CPUAFFINITY );

    work_queue_ = std::unique_ptr< work_queue_t >( new work_queue_t );
    for ( int i = 0; i < PRODUCER_WORK_QUEUE_BUF_COUNT; ++i )
    {
        std::unique_ptr< producer_buf > buf_( new producer_buf );

        buf_->move_buf = nullptr;
        buf_->vmic_pv_len = 0;

        daqd.initialize_vmpic( &( buf_->move_buf ),
                               &( buf_->vmic_pv_len ),
                               buf_->vmic_pv,
                               &( buf_->dcu_move_addresses ) );
        raii::array_ptr< unsigned char > mbuf_( buf_->move_buf );

        work_queue_->add_to_queue( PRODUCER_WORK_QUEUE_START, buf_.get( ) );
        buf_.release( );
        mbuf_.release( );
    }

    // start up CRC and transfer thread
    {
        DEBUG( 4, cerr << "Starting producer CRC thread" << endl );
        pthread_attr_t attr;
        pthread_attr_init( &attr );
        pthread_attr_setstacksize( &attr, daqd.thread_stack_size );
        pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );

        raii::lock_guard< pthread_mutex_t > crc_sync( prod_crc_mutex );
        int                                 err =
            pthread_create( &crc_tid,
                            &attr,
                            (void* (*)(void*))this->frame_writer_crc_static,
                            (void*)this );
        if ( err )
        {
            pthread_attr_destroy( &attr );
            system_log( 1,
                        "pthread_create() err=%d while creating producer debug "
                        "crc thread",
                        err );
            exit( 1 );
        }
        pthread_attr_destroy( &attr );

        pthread_cond_wait( &prod_crc_cond, &prod_crc_mutex );
        DEBUG( 5, cerr << "producer threads synced" << endl );
    }

    //    unsigned char*                      move_buf = nullptr;
    //    int                                 vmic_pv_len = 0;
    //    raii::array_ptr< struct put_dpvec > _vmic_pv(
    //        new struct put_dpvec[ MAX_CHANNELS ] );
    //    struct put_dpvec* vmic_pv = _vmic_pv.get( );
    //
    //
    //    // use the offsets calculated by initialize_vmpic
    //    // for the start of the dcu's
    //    dcu_move_address dcu_move_addresses;
    //
    //    // FIXME: move_buf could leak on errors (but we would probably die
    //    anyways. daqd.initialize_vmpic(
    //        &move_buf, &vmic_pv_len, vmic_pv, &dcu_move_addresses );
    //    raii::array_ptr< unsigned char > _move_buf( move_buf );

    // Allocate local test point tables
    static struct cdsDaqNetGdsTpNum gds_tp_table[ 2 ][ DCU_COUNT ];

    for ( int ifo = 0; ifo < daqd.data_feeds; ifo++ )
    {
        for ( int j = DCU_ID_EDCU; j < DCU_COUNT; j++ )
        {
            if ( daqd.dcuSize[ ifo ][ j ] == 0 )
                continue; // skip unconfigured DCU nodes
            if ( IS_MYRINET_DCU( j ) )
            {
                gdsTpNum[ ifo ][ j ] = gds_tp_table[ ifo ] + j;
            }
            else
            {
                gdsTpNum[ ifo ][ j ] = 0;
            }
        }
    }

    for ( int j = 0; j < DCU_COUNT; j++ )
    {
        class stats s;
        rcvr_stats.push_back( s );
    }

    ShMemReceiver shmem_receiver(
        daqd.parameters( ).get( "shmem_input", "" ),
        daqd.parameters( ).get< size_t >( "shmem_size", 21041152 ) );

    sleep( 1 );

    stat_full.sample( );

    // No waiting here if compiled as broadcasts receiver

    int cycle_delay = daqd.cycle_delay;

    // Wait until a second ends, so that the next data sould
    // come in on cycle 0
    // use the data as the clock here
    int sync_tries = 0;
    while ( true )
    {
        const int max_sync_tries = 10 * DATA_BLOCKS;

        daq_dc_data_t* block = shmem_receiver.receive_data( );
        ++sync_tries;
        if ( block->header.dcuTotalModels == 0 )
            continue;
        gps = block->header.dcuheader[ 0 ].timeSec;
        frac = block->header.dcuheader[ 0 ].timeNSec;
        std::cerr << block->header.dcuheader[ 0 ].timeNSec << std::endl;
        // as of 8 Nov 2017 zmq_multi_stream sends the gps nanoseconds as a
        // cycle number
        if ( frac == DATA_BLOCKS - 1 || frac >= 937500000 )
            break;
        if ( sync_tries > max_sync_tries )
        {
            std::cerr << "Unable to sync up to front ends after " << sync_tries
                      << " attempts" << std::endl;
            exit( 1 );
        }
    }
    prev_gps = gps;
    prev_frac = frac;

    PV::set_pv( PV::PV_UPTIME_SECONDS, 0 );
    PV::set_pv( PV::PV_GPS, 0 );

    int prev_controller_cycle = -1;
    int dcu_cycle = 0;
    int resync = 0;

    if ( daqd.dcu_status_check & 4 )
        resync = 1;

    std::array< int, DCU_COUNT >          dcu_to_zmq_lookup{};
    std::array< char*, DCU_COUNT >        dcu_data_from_zmq{};
    std::array< unsigned int, DCU_COUNT > dcu_data_crc{};
    std::array< unsigned int, DCU_COUNT > dcu_data_gps{};

    for ( unsigned long i = 0;; i++ )
    { // timing
        tick( ); // measure statistics

        // DEBUG(6, printf("Timing %d gps=%d frac=%d\n", i, gps, frac));

        std::fill( dcu_to_zmq_lookup.begin( ), dcu_to_zmq_lookup.end( ), -1 );
        std::fill(
            dcu_data_from_zmq.begin( ), dcu_data_from_zmq.end( ), (char*)0 );
        std::fill( dcu_data_crc.begin( ), dcu_data_crc.end( ), 0 );
        std::fill( dcu_data_gps.begin( ), dcu_data_gps.end( ), 0 );
        // retreive 1/16s of data from zmq

        stat_recv.sample( );
        daq_dc_data_t* data_block = shmem_receiver.receive_data( );
        stat_recv.tick( );

        producer_buf* cur_buffer =
            work_queue_->get_from_queue( RECV_THREAD_INPUT );
        circ_buffer_block_prop_t* cur_prop = &cur_buffer->prop;

        if ( data_block->header.dcuTotalModels > 0 )
        {
            gps = data_block->header.dcuheader[ 0 ].timeSec;
            frac = data_block->header.dcuheader[ 0 ].timeNSec;

            {
                for ( int i = 0; i < data_block->header.dcuTotalModels; ++i )
                {
                    if ( data_block->header.dcuheader[ i ].dataBlockSize == 0 )
                        std::cerr << "block " << i << " has 0 bytes"
                                  << std::endl;
                }
            }

            bool new_sec = ( i % 16 ) == 0;
            bool is_good = false;
            if ( new_sec )
            {
                is_good = ( gps == prev_gps + 1 && frac == 0 );
                for ( int i = 0; i < data_block->header.dcuTotalModels; ++i )
                {
                    int dcuid = data_block->header.dcuheader[ i ].dcuId;
                    gds_tp_table[ 0 ][ dcuid ].count =
                        data_block->header.dcuheader[ i ].tpCount;
                    std::copy( &data_block->header.dcuheader[ i ].tpNum[ 0 ],
                               &data_block->header.dcuheader[ i ].tpNum[ 256 ],
                               &gds_tp_table[ 0 ][ dcuid ].tpNum[ 0 ] );
                }
            }
            else
            {
                const unsigned int step = 1000000000 / 16;
                is_good = ( gps == prev_gps &&
                            ( ( frac == prev_frac + 1 ) ||
                              ( frac == prev_frac + step ) ) );
            }
            if ( !is_good )
            {
                std::cerr << "###################################\n\n\nGlitch "
                             "in receive\n"
                          << "prev " << prev_gps << ":" << prev_frac
                          << "    cur " << gps << ":" << frac
                          << " new_sec = " << new_sec << " i%16 = " << i % 16
                          << std::endl;
            }
        }
        if ( data_block->header.dcuTotalModels == 0 || ( gps > prev_gps + 1 ) )
        {
            fprintf( stderr,
                     "Dropped data from shmem or received 0 dcus; gps now = "
                     "%d, %d; was = %d, %d; dcu count = %d\n",
                     gps,
                     frac,
                     (int)prev_gps,
                     (int)prev_frac,
                     (int)( data_block->header.dcuTotalModels ) );
            exit( 1 );
        }

        // map out the order of the dcuids in the zmq data, this could change
        // with each data block
        {
            int   total_zmq_models = data_block->header.dcuTotalModels;
            char* cur_dcu_zmq_ptr = data_block->dataBlock;
            for ( int cur_block = 0; cur_block < total_zmq_models; ++cur_block )
            {
                unsigned int cur_dcuid =
                    data_block->header.dcuheader[ cur_block ].dcuId;
                dcu_to_zmq_lookup[ cur_dcuid ] = cur_block;
                dcu_data_from_zmq[ cur_dcuid ] = cur_dcu_zmq_ptr;
                dcu_data_crc[ cur_dcuid ] =
                    data_block->header.dcuheader[ cur_block ].dataCrc;
                dcu_data_gps[ cur_dcuid ] =
                    data_block->header.dcuheader[ cur_block ].timeSec;
                cur_dcu_zmq_ptr +=
                    data_block->header.dcuheader[ cur_block ].dataBlockSize +
                    data_block->header.dcuheader[ cur_block ].tpBlockSize;
            }
        }

        stat_crc.sample( );

        read_dest = cur_buffer->move_buf;
        for ( int j = DCU_ID_EDCU; j < DCU_COUNT; j++ )
        {

            // printf("DCU %d is %d bytes long\n", j, daqd.dcuSize[0][j]);
            if ( daqd.dcuSize[ 0 ][ j ] == 0 || dcu_to_zmq_lookup[ j ] < 0 ||
                 dcu_data_from_zmq[ j ] == (void*)0 )
            {
                bool seenLastCycle = dcuSeenLastCycle[ j ];
                if ( seenLastCycle )
                {
                    daqd.dcuCrcErrCntPerSecond[ 0 ][ j ]++;
                    daqd.dcuCrcErrCntPerSecondRunning[ 0 ][ j ]++;
                    daqd.dcuCrcErrCnt[ 0 ][ j ]++;
                }
                dcuSeenLastCycle[ j ] = false;
                daqd.dcuStatus[ 0 ][ j ] = 0xbad;
                continue; // skip unconfigured DCU nodes
            }
            dcuSeenLastCycle[ j ] = true;
            read_dest = cur_buffer->dcu_move_addresses.start[ j ];
            long read_size = daqd.dcuDAQsize[ 0 ][ j ];
            if ( IS_MYRINET_DCU( j ) )
            {
                dcu_cycle = i % DAQ_NUM_DATA_BLOCKS;

                // dcu_cycle = gmDaqIpc[j].cycle;
                // printf("cycl=%d ctrl=%d dcu=%d\n", gmDaqIpc[j].cycle,
                // controller_cycle, j);
                // Get the data from myrinet
                // Get the data from the buffers returned by the zmq receiver
                int               zmq_index = dcu_to_zmq_lookup[ j ];
                daq_msg_header_t& cur_dcu =
                    data_block->header.dcuheader[ zmq_index ];
                if ( read_size != cur_dcu.dataBlockSize )
                {
                    std::cerr << "read_size = " << read_size << " cur dcu size "
                              << cur_dcu.dataBlockSize << std::endl;
                }
                // assert(read_size == cur_dcu.dataBlockSize);
                memcpy( (void*)read_dest,
                        dcu_data_from_zmq[ j ],
                        cur_dcu.dataBlockSize );
                // copy test points over
                int max_tp_size =
                    2 * DAQ_DCU_BLOCK_SIZE - cur_dcu.dataBlockSize;
                int tp_size =
                    ( cur_dcu.tpBlockSize <= max_tp_size ? cur_dcu.tpBlockSize
                                                         : max_tp_size );
                memcpy( (void*)( read_dest + cur_dcu.dataBlockSize ),
                        (void*)( ( (char*)dcu_data_from_zmq[ j ] ) +
                                 cur_dcu.dataBlockSize ),
                        tp_size );
                int tp_off = reinterpret_cast< char* >( cur_buffer->move_buf ) -
                    reinterpret_cast< char* >( read_dest ) +
                    cur_dcu.dataBlockSize;

                int              cblk1 = ( i + 1 ) % DAQ_NUM_DATA_BLOCKS;
                static const int ifo = 0; // For now

                // Calculate DCU status, if needed
                if ( daqd.dcu_status_check & ( 1 << ifo ) )
                {
                    if ( cblk1 % 16 == 0 )
                    {
                        int lastStatus = dcuStatus[ ifo ][ j ];

                        /* Check if DCU running at all */
                        if ( dcuStatCycle[ ifo ][ j ] == 0 )
                            dcuStatus[ ifo ][ j ] = DAQ_STATE_SYNC_ERR;
                        else
                            dcuStatus[ ifo ][ j ] = DAQ_STATE_RUN;

                        // dcuCycleStatus shows how many matches of cycle number
                        // we got
                        DEBUG( 4,
                               cerr << "dcuid=" << j << " dcuCycleStatus="
                                    << dcuCycleStatus[ ifo ][ j ]
                                    << " dcuStatCycle="
                                    << dcuStatCycle[ ifo ][ j ] << endl );

                        /* Check if DCU running and in sync */
                        if ( ( dcuCycleStatus[ ifo ][ j ] > 3 || j < 5 ) &&
                             dcuStatCycle[ ifo ][ j ] > 4 )
                        {
                            dcuStatus[ ifo ][ j ] = DAQ_STATE_RUN;
                        }

                        if ( /* (lastStatus == DAQ_STATE_RUN) && */ (
                            dcuStatus[ ifo ][ j ] != DAQ_STATE_RUN ) )
                        {
                            DEBUG( 4,
                                   cerr << "Lost " << daqd.dcuName[ j ]
                                        << "(ifo " << ifo << "; dcu " << j
                                        << "); status "
                                        << dcuCycleStatus[ ifo ][ j ]
                                        << dcuStatCycle[ ifo ][ j ] << endl );
                            std::cout << "Lost " << daqd.dcuName[ j ] << "(ifo "
                                      << ifo << "; dcu " << j << "); status "
                                      << dcuCycleStatus[ ifo ][ j ]
                                      << dcuStatCycle[ ifo ][ j ] << endl;
                            cur_dcu.status = DAQ_STATE_FAULT;
                        }

                        if ((dcuStatus[ifo][j] ==
                             DAQ_STATE_RUN) /* && (lastStatus != DAQ_STATE_RUN) */)
                        {
                            DEBUG( 4,
                                   cerr << "New " << daqd.dcuName[ j ]
                                        << " (dcu " << j << ")" << endl );
                            cur_dcu.status = DAQ_STATE_RUN;
                        }

                        dcuCycleStatus[ ifo ][ j ] = 0;
                        dcuStatCycle[ ifo ][ j ] = 0;
                        cur_dcu.status = cur_dcu.status;
                    }

                    {
                        int intCycle = cur_dcu.cycle % DAQ_NUM_DATA_BLOCKS;
                        if ( intCycle != dcuLastCycle[ ifo ][ j ] )
                            dcuStatCycle[ ifo ][ j ]++;
                        dcuLastCycle[ ifo ][ j ] = intCycle;
                    }
                }

                // Update DCU status
                int newStatus = cur_dcu.status != DAQ_STATE_RUN ? 0xbad : 0;
                DEBUG( 4,
                       std::cout << "newStatus = " << ( hex ) << newStatus
                                 << " cur_dcu.status = " << ( dec )
                                 << cur_dcu.status;
                       std::cout << " gps = " << cur_dcu.timeSec << " gps_n = "
                                 << cur_dcu.timeNSec << std::endl; );
                int newCrc = cur_dcu.fileCrc;

                // printf("%x\n", *((int *)read_dest));
                if ( !IS_EXC_DCU( j ) )
                {
                    if ( newCrc != daqd.dcuConfigCRC[ 0 ][ j ] )
                    {
                        newStatus |= 0x2000;
                        DEBUG( 4,
                               std::cout << "config crc mismatch expecting "
                                         << std::hex
                                         << daqd.dcuConfigCRC[ 0 ][ j ]
                                         << " got " << std::hex << newCrc
                                         << std::endl; );
                    }
                }
                if ( newStatus != daqd.dcuStatus[ 0 ][ j ] )
                {
                    // system_log(1, "DCU %d IFO %d (%s) %s", j, 0,
                    // daqd.dcuName[j], newStatus? "fault": "running");
                    if ( newStatus & 0x2000 )
                    {
                        // system_log(1, "DCU %d IFO %d (%s) reconfigured (crc
                        // 0x%x rfm 0x%x)", j, 0, daqd.dcuName[j],
                        // daqd.dcuConfigCRC[0][j], newCrc);
                    }
                }
                daqd.dcuStatus[ 0 ][ j ] = newStatus;

                daqd.dcuCycle[ 0 ][ j ] = cur_dcu.cycle;

                /* Check DCU data checksum */
                unsigned long  bytes = read_size;
                unsigned char* cp = (unsigned char*)read_dest;

                crc_obj.add( cp, bytes );
                auto crc = crc_obj.result( );
                crc_obj.reset( );

                int cblk = i % 16;
                // Reset CRC/second variable for this DCU
                if ( cblk == 0 )
                {
                    daqd.dcuCrcErrCntPerSecond[ 0 ][ j ] =
                        daqd.dcuCrcErrCntPerSecondRunning[ 0 ][ j ];
                    daqd.dcuCrcErrCntPerSecondRunning[ 0 ][ j ] = 0;
                }

                read_dest += 2 * DAQ_DCU_BLOCK_SIZE; // cur_dcu.dataBlockSize;

                if ( j >= DCU_ID_ADCU_1 && ( !IS_TP_DCU( j ) ) &&
                     daqd.dcuStatus[ 0 ][ j ] == 0 )
                {

                    unsigned int rfm_crc =
                        dcu_data_crc[ j ]; // gmDaqIpc[j].bp[cblk].crc;
                    unsigned int dcu_gps =
                        dcu_data_gps[ j ]; // gmDaqIpc[j].bp[cblk].timeSec;

                    // system_log(5, "dcu %d block %d cycle %d  gps %d symm
                    // %d\n", j, cblk, gmDaqIpc[j].bp[cblk].cycle,  dcu_gps,
                    // gps);
                    unsigned long mygps = gps;
                    // if (cblk > (15 - cycle_delay))
                    //    mygps--;

                    if ( dcu_gps != mygps )
                    {
                        daqd.dcuStatus[ 0 ][ j ] |= 0x4000;
                        system_log(
                            5,
                            "GPS MISS dcu %d (%s); dcu_gps=%d gps=%ld\n",
                            j,
                            daqd.dcuName[ j ],
                            dcu_gps,
                            mygps );
                    }

                    if ( rfm_crc != crc )
                    {
                        system_log(
                            5,
                            "MISS dcu %d (%s); crc[%d]=%x; computed crc=%lx\n",
                            j,
                            daqd.dcuName[ j ],
                            cblk,
                            rfm_crc,
                            crc );

                        /* Set DCU status to BAD, all data will be marked as BAD
                           because of the CRC mismatch */
                        daqd.dcuStatus[ 0 ][ j ] |= 0x1000;
                    }
                    else
                    {
                        system_log( 10,
                                    " MATCH dcu %d (%s); crc[%d]=%x; "
                                    "computed crc=%lx\n",
                                    j,
                                    daqd.dcuName[ j ],
                                    cblk,
                                    rfm_crc,
                                    crc );
                    }
                    if ( daqd.dcuStatus[ 0 ][ j ] )
                    {
                        daqd.dcuCrcErrCnt[ 0 ][ j ]++;
                        daqd.dcuCrcErrCntPerSecondRunning[ 0 ][ j ]++;
                    }
                }
            }
        }

        stat_crc.tick( );

        int cblk = i % 16;

        // Assign per-DCU data we need to broadcast out
        //
        for ( int ifo = 0; ifo < daqd.data_feeds; ifo++ )
        {
            for ( int j = DCU_ID_EDCU; j < DCU_COUNT; j++ )
            {
                if ( IS_TP_DCU( j ) )
                    continue; // Skip TP and EXC DCUs
                if ( daqd.dcuSize[ ifo ][ j ] == 0 )
                    continue; // Skip unconfigured DCUs
                cur_prop->dcu_data[ j + ifo * DCU_COUNT ].cycle =
                    daqd.dcuCycle[ ifo ][ j ];
                volatile struct rmIpcStr* ipc = daqd.dcuIpc[ ifo ][ j ];

                // Do not support Myrinet DCUs on H2
                if ( IS_MYRINET_DCU( j ) && ifo == 0 )
                {

                    cur_prop->dcu_data[ j ].crc =
                        dcu_data_crc[ j ]; // gmDaqIpc[j].bp[cblk].crc;

                    // printf("dcu %d crc=0x%x\n", j, prop.dcu_data[j].crc);
                    // Remove 0x8000 status from propagating to the broadcast
                    // receivers
                    cur_prop->dcu_data[ j ].status =
                        daqd.dcuStatus[ 0 /* IFO */ ][ j ] & ~0x8000;
                }
                else
                {
                    cur_prop->dcu_data[ j + ifo * DCU_COUNT ].crc =
                        ipc->bp[ cblk ].crc;
                    cur_prop->dcu_data[ j + ifo * DCU_COUNT ].status =
                        daqd.dcuStatus[ ifo ][ j ];
                }
            }
        }

        // prop.gps = time(0) - 315964819 + 33;

        cur_prop->gps = gps;
        // if (cblk > (15 - cycle_delay))
        //    prop.gps--;

        cur_prop->gps_n = 1000000000 / 16 * ( i % 16 );
        // printf("before put %d %d %d\n", prop.gps, prop.gps_n, frac);
        cur_prop->leap_seconds = daqd.gps_leap_seconds( gps );

        // std::cout << "about to call put16th_dpscattered with " << vmic_pv_len
        // << " entries. prop.gps = " << prop.gps << " prop.gps_n = " <<
        // prop.gps_n << "\n"; for (int ii = 0; ii < vmic_pv_len; ++ii)
        //    std::cout << " " << *vmic_pv[ii].src_status_addr;
        // std::cout << std::endl;

        work_queue_->add_to_queue( RECV_THREAD_OUTPUT, cur_buffer );
        cur_buffer = nullptr;

        PV::set_pv( PV::PV_CYCLE, i );
        PV::set_pv( PV::PV_GPS, gps );
        // DEBUG1(cerr << "gps=" << PV::pv(PV::PV_GPS) << endl);
        if ( i % 16 == 0 )
        {
            // Count how many seconds we were acquiring data
            PV::pv( PV::PV_UPTIME_SECONDS )++;

            {
                extern unsigned long dmt_retransmit_count;
                extern unsigned long dmt_failed_retransmit_count;
                // Display DMT retransmit channels every second
                PV::set_pv( PV::PV_BCAST_RETR, dmt_retransmit_count );
                PV::set_pv( PV::PV_BCAST_FAILED_RETR,
                            dmt_failed_retransmit_count );
                dmt_retransmit_count = 0;
                dmt_failed_retransmit_count = 0;
            }
        }

        stat_full.tick( );

        ++stat_cycles;
        if ( stat_cycles >= 16 )
        {
            PV::set_pv( PV::PV_PRDCR_TIME_FULL_MIN_MS,
                        conv::s_to_ms_int( stat_full.getMin( ) ) );
            PV::set_pv( PV::PV_PRDCR_TIME_FULL_MAX_MS,
                        conv::s_to_ms_int( stat_full.getMax( ) ) );
            PV::set_pv( PV::PV_PRDCR_TIME_FULL_MEAN_MS,
                        conv::s_to_ms_int( stat_full.getMean( ) ) );

            PV::set_pv( PV::PV_PRDCR_TIME_RECV_MIN_MS,
                        conv::s_to_ms_int( stat_recv.getMin( ) ) );
            PV::set_pv( PV::PV_PRDCR_TIME_RECV_MAX_MS,
                        conv::s_to_ms_int( stat_recv.getMax( ) ) );
            PV::set_pv( PV::PV_PRDCR_TIME_RECV_MEAN_MS,
                        conv::s_to_ms_int( stat_recv.getMean( ) ) );

            PV::set_pv( PV::PV_PRDCR_CRC_TIME_CRC_MIN_MS,
                        conv::s_to_ms_int( stat_crc.getMin( ) ) );
            PV::set_pv( PV::PV_PRDCR_CRC_TIME_CRC_MAX_MS,
                        conv::s_to_ms_int( stat_crc.getMax( ) ) );
            PV::set_pv( PV::PV_PRDCR_CRC_TIME_CRC_MEAN_MS,
                        conv::s_to_ms_int( stat_crc.getMean( ) ) );

            stat_full.clearStats( );
            stat_crc.clearStats( );
            stat_recv.clearStats( );
            stat_cycles = 0;
        }

        stat_full.sample( );

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
void*
producer::frame_writer_debug_crc( )
{
    // not implemented
    return (void*)NULL;
}

/// A main loop for a producer that does crc  and data transfer
/// in a seperate thread.
void*
producer::frame_writer_crc( )
{
    int stat_cycles = 0;

    stats stat_transfer;

    //    PV::set_pv(PV::PV_UPTIME_SECONDS, 0);
    //    PV::set_pv(PV::PV_GPS, 0);

    // Set thread parameters
    daqd_c::set_thread_priority( "Producer crc",
                                 "dqprodcrc",
                                 PROD_CRC_THREAD_PRIORITY,
                                 PROD_CRC_CPUAFFINITY );

    // checksum_crc32 crc_obj;

    {
        raii::lock_guard< pthread_mutex_t > lock_{ prod_crc_mutex };
        pthread_cond_signal( &prod_crc_cond );
    }

    for ( unsigned long i = 0;; ++i )
    {
        unsigned int gps, gps_n;

        producer_buf* cur_buffer =
            work_queue_->get_from_queue( CRC_THREAD_INPUT );
        circ_buffer_block_prop_t* cur_prop = &( cur_buffer->prop );
        gps = cur_prop->gps;
        gps_n = cur_prop->gps_n;
        //        unsigned char *move_buf = cur_buffer->move_buf;

        //        stat_full.sample();
        //        stat_crc.sample();
        //        // Parse received broadcast transmission header and
        //        // check config file CRCs and data CRCs, check DCU size and
        //        number
        //        // Assign DCU status and cycle.
        //        unsigned int *header =
        //            (unsigned int *)(((char *)move_buf) -
        //            BROADCAST_HEADER_SIZE);
        //        int ndcu = ntohl(*header++);
        //        // printf("ndcu = %d\n", ndcu);
        //        if (ndcu > 0 && ndcu <= MAX_BROADCAST_DCU_NUM) {
        //            int data_offs = 0; // Offset to the current DCU data
        //            for (int j = 0; j < ndcu; j++) {
        //                unsigned int dcu_number;
        //                unsigned int dcu_size;   // Data size for this DCU
        //                unsigned int config_crc; // Configuration file CRC
        //                unsigned int dcu_crc;    // Data CRC
        //                unsigned int status; // DCU status word bits (0-ok,
        //                0xbad-out of
        //                // sync, 0x1000-trasm error
        //                // 0x2000 - configuration mismatch).
        //                unsigned int cycle;  // DCU cycle
        //                dcu_number = ntohl(*header++);
        //                dcu_size = ntohl(*header++);
        //                config_crc = ntohl(*header++);
        //                dcu_crc = ntohl(*header++);
        //                status = ntohl(*header++);
        //                cycle = ntohl(*header++);
        //                int ifo = 0;
        //                if (dcu_number > DCU_COUNT) {
        //                    ifo = 1;
        //                    dcu_number -= DCU_COUNT;
        //                }
        //                // printf("dcu=%d size=%d config_crc=0x%x crc=0x%x
        //                status=0x%x
        //                // cycle=%d\n",
        //                // dcu_number, dcu_size, config_crc, dcu_crc, status,
        //                cycle); if (daqd.dcuSize[ifo][dcu_number]) { // Don't
        //                do anything if
        //                    // this DCU is not
        //                    // configured
        //                    daqd.dcuStatus[ifo][dcu_number] = status;
        //                    daqd.dcuCycle[ifo][dcu_number] = cycle;
        //                    if (status ==
        //                        0) { // If the DCU status is OK from the
        //                        concentrator
        //                        // Check for local configuration and data
        //                        mismatch if (config_crc !=
        //                        daqd.dcuConfigCRC[ifo][dcu_number]) {
        //                            // Detected local configuration mismach
        //                            daqd.dcuStatus[ifo][dcu_number] |= 0x2000;
        //                        }
        //                        unsigned char *cp =
        //                            move_buf + data_offs; // Start of data
        //                        unsigned int bytes = dcu_size; // DCU data
        //                        size crc_obj.add(cp, bytes);
        ////                        unsigned int crc = 0;
        ////                        // Calculate DCU data CRC
        ////                        while (bytes--) {
        ////                            crc = (crc << 8) ^
        ////                                  crctab[((crc >> 24) ^ *(cp++)) &
        ///0xFF]; /                        } /                        bytes =
        ///dcu_size; /                        while (bytes > 0) { / crc = (crc
        ///<< 8) ^ /                                  crctab[((crc >> 24) ^
        ///bytes) & 0xFF]; /                            bytes >>= 8; / } / crc =
        ///~crc & 0xFFFFFFFF;
        //                        unsigned int crc = crc_obj.result();
        //                        if (crc != dcu_crc) {
        //                            // Detected data corruption !!!
        //                            daqd.dcuStatus[ifo][dcu_number] |= 0x1000;
        //                            DEBUG1(printf(
        //                                "ifo=%d dcu=%d calc_crc=0x%x
        //                                data_crc=0x%x\n", ifo, dcu_number,
        //                                crc, dcu_crc));
        //                        }
        //                        crc_obj.reset();
        //                    }
        //                }
        //                data_offs += dcu_size;
        //            }
        //        }
        //        stat_crc.tick();
        //
        //        // :TODO: make sure all DCUs configuration matches; restart
        //        when the
        //        // mismatch detected
        //
        //        prop.gps = gps;
        //        prop.gps_n = gps_n;
        //
        //        prop.leap_seconds = daqd.gps_leap_seconds(prop.gps);

        stat_transfer.sample( );
        int nbi = daqd.b1->put16th_dpscattered(
            cur_buffer->vmic_pv, cur_buffer->vmic_pv_len, cur_prop );
        stat_transfer.tick( );

        DEBUG( 4,
               std::cout << "put16th_dpscattered returned " << nbi << std::endl;
               std::cout << "drops: " << daqd.b1->drops( )
                         << " blocks: " << daqd.b1->blocks( )
                         << " puts: " << daqd.b1->num_puts( ) << " consumers: "
                         << daqd.b1->get_cons_num( ) << std::endl; );
        //  printf("%d %d\n", prop.gps, prop.gps_n);
        // DEBUG1(cerr << "producer " << i << endl);

        work_queue_->add_to_queue( CRC_THREAD_OUTPUT, cur_buffer );
        cur_buffer = nullptr;

        //  printf("%d %d\n", prop.gps, prop.gps_n);
        // DEBUG1(cerr << "producer " << i << endl);

        ++stat_cycles;
        if ( stat_cycles >= 16 )
        {
            //            PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MIN_MS,
            //            conv::s_to_ms_int(stat_crc.getMin()));
            //            PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MAX_MS,
            //            conv::s_to_ms_int(stat_crc.getMax()));
            //            PV::set_pv(PV::PV_PRDCR_CRC_TIME_CRC_MEAN_MS,
            //            conv::s_to_ms_int(stat_crc.getMean()));

            PV::set_pv( PV::PV_PRDCR_CRC_TIME_XFER_MIN_MS,
                        conv::s_to_ms_int( stat_transfer.getMin( ) ) );
            PV::set_pv( PV::PV_PRDCR_CRC_TIME_XFER_MAX_MS,
                        conv::s_to_ms_int( stat_transfer.getMax( ) ) );
            PV::set_pv( PV::PV_PRDCR_CRC_TIME_XFER_MEAN_MS,
                        conv::s_to_ms_int( stat_transfer.getMean( ) ) );

            //            stat_crc.clearStats();
            stat_transfer.clearStats( );
            stat_cycles = 0;
        }
    }

    return nullptr;
}
